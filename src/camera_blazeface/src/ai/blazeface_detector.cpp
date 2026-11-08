/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "blazeface_detector.h"
#include "model_data.h"
#include "blazeface_anchors.h"

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#define BLAZEFACE_INPUT_W 128
#define BLAZEFACE_INPUT_H 128
#define BLAZEFACE_INPUT_CH 3

// Detection thresholds (0.30 confidence for real-time responsiveness)
#define SCORE_THRESHOLD 0.30f
#define SIGMOID_THRESHOLD -0.847298f // ln(0.30 / (1 - 0.30))
#define IOU_THRESHOLD 0.30f
#define MAX_CANDIDATES 32

// 1 MB Tensor Arena placed in external 8MB PSRAM (.ext_ram.bss) to save internal SRAM
#define TENSOR_ARENA_SIZE (1024 * 1024)
alignas(16) static uint8_t s_tensor_arena[TENSOR_ARENA_SIZE] __attribute__((section(".ext_ram.bss")));

// TFLM static structures
static const tflite::Model* s_model = nullptr;
static tflite::MicroInterpreter* s_interpreter = nullptr;
static tflite::MicroMutableOpResolver<10> s_resolver;

// Internal candidate structure for NMS
typedef struct {
    float x1, y1, x2, y2;
    float score;
} candidate_box_t;

static candidate_box_t s_candidates[MAX_CANDIDATES];
static int s_candidate_count = 0;

int blazeface_detector_init(void)
{
    printk("[BLAZEFACE] Initializing BlazeFace Face Detector...\n");

    s_model = tflite::GetModel(g_blazeface_model_data);
    if (s_model->version() != TFLITE_SCHEMA_VERSION) {
        printk("[BLAZEFACE] Error: Model schema version %u != supported %d!\n",
               (unsigned int)s_model->version(), TFLITE_SCHEMA_VERSION);
        return -EINVAL;
    }

    // Register only operators required by BlazeFace
    s_resolver.AddConv2D();
    s_resolver.AddDepthwiseConv2D();
    s_resolver.AddAdd();
    s_resolver.AddMaxPool2D();
    s_resolver.AddPad();
    s_resolver.AddReshape();
    s_resolver.AddQuantize();
    s_resolver.AddDequantize();

    static tflite::MicroInterpreter static_interpreter(
        s_model, s_resolver, s_tensor_arena, TENSOR_ARENA_SIZE);
    s_interpreter = &static_interpreter;

    TfLiteStatus alloc_status = s_interpreter->AllocateTensors();
    if (alloc_status != kTfLiteOk) {
        printk("[BLAZEFACE] Error: AllocateTensors() failed: %d!\n", alloc_status);
        return -ENOMEM;
    }

    TfLiteTensor *input = s_interpreter->input(0);
    printk("[BLAZEFACE] TFLM Model initialized successfully!\n");
    printk("[BLAZEFACE] Input tensor: dims=[%d,%d,%d,%d] type=%d bytes=%zu\n",
           input->dims->data[0], input->dims->data[1],
           input->dims->data[2], input->dims->data[3],
           input->type, input->bytes);
    printk("[BLAZEFACE] Arena used: %zu / %d bytes\n",
           s_interpreter->arena_used_bytes(), TENSOR_ARENA_SIZE);

    for (size_t i = 0; i < s_interpreter->outputs_size(); i++) {
        TfLiteTensor *out = s_interpreter->output(i);
        printk("[BLAZEFACE] Output %u: dims=[", (unsigned int)i);
        for (int d = 0; d < out->dims->size; d++) {
            printk("%d%s", out->dims->data[d], (d < out->dims->size - 1) ? "," : "");
        }
        printk("] type=%d bytes=%u scale_x1000=%d zp=%d\n",
               out->type, (unsigned int)out->bytes,
               (int)(out->params.scale * 1000.0f), out->params.zero_point);
    }

    return 0;
}

static inline float calculate_iou(const candidate_box_t *a, const candidate_box_t *b)
{
    float x1 = (a->x1 > b->x1) ? a->x1 : b->x1;
    float y1 = (a->y1 > b->y1) ? a->y1 : b->y1;
    float x2 = (a->x2 < b->x2) ? a->x2 : b->x2;
    float y2 = (a->y2 < b->y2) ? a->y2 : b->y2;

    float inter_w = x2 - x1;
    float inter_h = y2 - y1;
    if (inter_w <= 0.0f || inter_h <= 0.0f) {
        return 0.0f;
    }
    float inter_area = inter_w * inter_h;
    float area_a = (a->x2 - a->x1) * (a->y2 - a->y1);
    float area_b = (b->x2 - b->x1) * (b->y2 - b->y1);
    float union_area = area_a + area_b - inter_area;
    return (union_area > 0.0f) ? (inter_area / union_area) : 0.0f;
}

int blazeface_detector_run(const uint16_t *frame_rgb565,
                           blazeface_box_t *out_boxes,
                           int max_boxes,
                           uint32_t *infer_time_ms)
{
    if (s_interpreter == nullptr) {
        return -EINVAL;
    }

    uint32_t t_start = k_uptime_get_32();

    // 1. Preprocess: Crop center 240x240 from 320x240 RGB565 and downsample to 128x128
    TfLiteTensor *input_tensor = s_interpreter->input(0);
    float *input_data = input_tensor->data.f;

    // Center crop: X: 40 to 280 (width 240), Y: 0 to 240 (height 240)
    for (int dy = 0; dy < BLAZEFACE_INPUT_H; dy++) {
        int sy = (dy * 240) / BLAZEFACE_INPUT_H; // [0..239]
        const uint16_t *line_src = &frame_rgb565[sy * 320];
        float *dst_line = &input_data[dy * BLAZEFACE_INPUT_W * 3];

        for (int dx = 0; dx < BLAZEFACE_INPUT_W; dx++) {
            int sx = 40 + (dx * 240) / BLAZEFACE_INPUT_W; // [40..279]
            const uint8_t *pix_bytes = (const uint8_t *)&line_src[sx];
            uint8_t b0 = pix_bytes[0];
            uint8_t b1 = pix_bytes[1];

            // GC0308 produces RGB565: b0=[R4..R0, G5..G3], b1=[G2..G0, B4..B0]
            uint8_t r = (uint8_t)(((b0 >> 3) & 0x1F) * 255 / 31);
            uint8_t g = (uint8_t)((((b0 & 0x07) << 3) | ((b1 >> 5) & 0x07)) * 255 / 63);
            uint8_t b = (uint8_t)((b1 & 0x1F) * 255 / 31);

            // Normalize to [0.0, 1.0] matching INT8 Quantization (scale = 1/255, zp = -128)
            dst_line[dx * 3 + 0] = (float)r / 255.0f;
            dst_line[dx * 3 + 1] = (float)g / 255.0f;
            dst_line[dx * 3 + 2] = (float)b / 255.0f;
        }
    }

    // 2. Invoke TFLM inference
    TfLiteStatus status = s_interpreter->Invoke();
    if (status != kTfLiteOk) {
        printk("[BLAZEFACE] Invoke failed: %d\n", status);
        return -EIO;
    }

    uint32_t t_end = k_uptime_get_32();
    if (infer_time_ms) {
        *infer_time_ms = (t_end - t_start);
    }

    // 3. Extract SSD Anchors and Filter by Score
    // Output 0: [1, 512, 16] (boxes layer 0)
    // Output 1: [1, 512, 1]  (scores layer 0)
    // Output 2: [1, 384, 1]  (scores layer 1)
    // Output 3: [1, 384, 16] (boxes layer 1)
    const float *out0_boxes  = s_interpreter->output(0)->data.f;
    const float *out1_scores = s_interpreter->output(1)->data.f;
    const float *out2_scores = s_interpreter->output(2)->data.f;
    const float *out3_boxes  = s_interpreter->output(3)->data.f;

    s_candidate_count = 0;
    float max_raw0 = -9999.0f, min_raw0 = 9999.0f;
    float max_raw1 = -9999.0f, min_raw1 = 9999.0f;

    // Check layer 0 (512 anchors)
    for (int i = 0; i < 512; i++) {
        float raw_score = out1_scores[i];
        if (raw_score > max_raw0) max_raw0 = raw_score;
        if (raw_score < min_raw0) min_raw0 = raw_score;

        if (raw_score > SIGMOID_THRESHOLD && s_candidate_count < MAX_CANDIDATES) {
            const blazeface_anchor_t *anchor = &g_blazeface_anchors[i];
            float sx = out0_boxes[i * 16 + 0];
            float sy = out0_boxes[i * 16 + 1];
            float w  = out0_boxes[i * 16 + 2];
            float h  = out0_boxes[i * 16 + 3];

            float cx = (sx + anchor->x_center * 128.0f) / 128.0f;
            float cy = (sy + anchor->y_center * 128.0f) / 128.0f;
            w /= 128.0f;
            h /= 128.0f;

            candidate_box_t *c = &s_candidates[s_candidate_count++];
            c->x1 = cx - w * 0.5f;
            c->y1 = cy - h * 0.5f;
            c->x2 = cx + w * 0.5f;
            c->y2 = cy + h * 0.5f;
            c->score = 1.0f / (1.0f + expf(-raw_score));
        }
    }

    // Check layer 1 (384 anchors)
    for (int j = 0; j < 384; j++) {
        float raw_score = out2_scores[j];
        if (raw_score > max_raw1) max_raw1 = raw_score;
        if (raw_score < min_raw1) min_raw1 = raw_score;

        if (raw_score > SIGMOID_THRESHOLD && s_candidate_count < MAX_CANDIDATES) {
            int anchor_idx = 512 + j;
            const blazeface_anchor_t *anchor = &g_blazeface_anchors[anchor_idx];
            float sx = out3_boxes[j * 16 + 0];
            float sy = out3_boxes[j * 16 + 1];
            float w  = out3_boxes[j * 16 + 2];
            float h  = out3_boxes[j * 16 + 3];

            float cx = (sx + anchor->x_center * 128.0f) / 128.0f;
            float cy = (sy + anchor->y_center * 128.0f) / 128.0f;
            w /= 128.0f;
            h /= 128.0f;

            candidate_box_t *c = &s_candidates[s_candidate_count++];
            c->x1 = cx - w * 0.5f;
            c->y1 = cy - h * 0.5f;
            c->x2 = cx + w * 0.5f;
            c->y2 = cy + h * 0.5f;
            c->score = 1.0f / (1.0f + expf(-raw_score));
        }
    }

    if (s_candidate_count > 0) {
        printk("[BLAZEFACE] Detected %d face candidate(s) (Scores: max0=%d, max1=%d x1000)\n",
               s_candidate_count, (int)(max_raw0 * 1000.0f), (int)(max_raw1 * 1000.0f));
    }

    if (s_candidate_count == 0) {
        return 0;
    }

    // 4. Sort candidates descending by score
    for (int i = 0; i < s_candidate_count - 1; i++) {
        for (int k = i + 1; k < s_candidate_count; k++) {
            if (s_candidates[k].score > s_candidates[i].score) {
                candidate_box_t tmp = s_candidates[i];
                s_candidates[i] = s_candidates[k];
                s_candidates[k] = tmp;
            }
        }
    }

    // 5. Non-Maximum Suppression (NMS)
    bool suppressed[MAX_CANDIDATES] = { false };
    int detected_faces = 0;

    for (int i = 0; i < s_candidate_count && detected_faces < max_boxes; i++) {
        if (suppressed[i]) continue;

        candidate_box_t *c = &s_candidates[i];

        // Map normalized crop coordinates to 320x240 screen coordinates
        // Crop was: X: 40 + x * 240, Y: y * 240
        int sx1 = 40 + (int)(c->x1 * 240.0f);
        int sy1 = (int)(c->y1 * 240.0f);
        int sx2 = 40 + (int)(c->x2 * 240.0f);
        int sy2 = (int)(c->y2 * 240.0f);

        // Ensure correct coordinate ordering
        if (sx1 > sx2) { int tmp = sx1; sx1 = sx2; sx2 = tmp; }
        if (sy1 > sy2) { int tmp = sy1; sy1 = sy2; sy2 = tmp; }

        // Clamping to screen boundaries [0..319, 0..239]
        if (sx1 < 0) sx1 = 0;
        if (sy1 < 0) sy1 = 0;
        if (sx2 > 319) sx2 = 319;
        if (sy2 > 239) sy2 = 239;

        out_boxes[detected_faces].x1 = sx1;
        out_boxes[detected_faces].y1 = sy1;
        out_boxes[detected_faces].x2 = sx2;
        out_boxes[detected_faces].y2 = sy2;
        out_boxes[detected_faces].score = c->score;
        detected_faces++;

        // Suppress overlapping candidates
        for (int k = i + 1; k < s_candidate_count; k++) {
            if (!suppressed[k] && calculate_iou(c, &s_candidates[k]) > IOU_THRESHOLD) {
                suppressed[k] = true;
            }
        }
    }

    if (detected_faces > 0) {
        printk("[BLAZEFACE] >>> DETECTED %d FACE(S)! Box: (%d,%d)-(%d,%d) Score: %d%% <<<\n",
               detected_faces, out_boxes[0].x1, out_boxes[0].y1,
               out_boxes[0].x2, out_boxes[0].y2, (int)(out_boxes[0].score * 100.0f));
    }

    return detected_faces;
}
