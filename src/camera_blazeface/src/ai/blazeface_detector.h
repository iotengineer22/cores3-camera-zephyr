/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLAZEFACE_DETECTOR_H_
#define BLAZEFACE_DETECTOR_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int x1;
    int y1;
    int x2;
    int y2;
    float score;
} blazeface_box_t;

#define MAX_DETECTED_FACES 4

/**
 * @brief Initialize BlazeFace model, op resolver, and TFLM interpreter.
 * @return 0 on success, negative error code on failure.
 */
int blazeface_detector_init(void);

/**
 * @brief Run face detection on a 320x240 RGB565 frame.
 * Preprocesses center 240x240 region downscaled to 128x128.
 * Decodes SSD anchors, filters scores > threshold, applies NMS.
 *
 * @param frame_rgb565 Pointer to 320x240 RGB565 camera buffer.
 * @param out_boxes Array to store detected bounding boxes.
 * @param max_boxes Max elements in out_boxes.
 * @param infer_time_ms Output inference time in milliseconds.
 * @return Number of faces detected.
 */
int blazeface_detector_run(const uint16_t *frame_rgb565,
                           blazeface_box_t *out_boxes,
                           int max_boxes,
                           uint32_t *infer_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* BLAZEFACE_DETECTOR_H_ */
