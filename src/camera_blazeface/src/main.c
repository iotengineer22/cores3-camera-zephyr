/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>
#include <zephyr/video/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/version.h>
#include <soc/lcd_cam_struct.h>
#include <soc/gpio_struct.h>
#include <string.h>

#include "blazeface_detector.h"

LOG_MODULE_REGISTER(camera_blazeface, LOG_LEVEL_INF);

#define LCD_WIDTH   320
#define LCD_HEIGHT  240
#define FRAME_SIZE  (LCD_WIDTH * LCD_HEIGHT * 2) /* 153,600 bytes for RGB565 */

/* Color constants in SPI byte-swapped format (Big Endian over SPI) */
#define COLOR_GREEN 0xE007 /* 0x07E0 swapped */
#define COLOR_RED   0x00F8 /* 0xF800 swapped */
#define COLOR_CYAN  0xFF07 /* 0x07FF swapped */

/* Line buffer for screen clearing */
static uint16_t line_buf[LCD_WIDTH];

/* Snapshot buffer in 8MB PSRAM for asynchronous AI inference */
static uint16_t s_ai_frame[LCD_WIDTH * LCD_HEIGHT] __attribute__((section(".ext_ram.bss")));

/* Thread synchronization */
static K_SEM_DEFINE(s_ai_sem, 0, 1);
static K_MUTEX_DEFINE(s_box_mutex);

static volatile bool s_ai_busy = false;
static blazeface_box_t s_detected_boxes[MAX_DETECTED_FACES];
static int s_detected_box_count = 0;
static uint32_t s_last_infer_time_ms = 0;

/* AI Worker Thread Definition */
#define AI_THREAD_STACK_SIZE 8192
#define AI_THREAD_PRIORITY   11
K_THREAD_STACK_DEFINE(s_ai_stack, AI_THREAD_STACK_SIZE);
static struct k_thread s_ai_thread_data;

static void clear_screen_color(const struct device *display_dev, uint16_t rgb565_color)
{
	uint16_t be_color = (rgb565_color >> 8) | ((rgb565_color & 0xff) << 8);
	for (size_t i = 0; i < LCD_WIDTH; i++) {
		line_buf[i] = be_color;
	}

	struct display_buffer_descriptor desc = {
		.buf_size = sizeof(line_buf),
		.width = LCD_WIDTH,
		.height = 1,
		.pitch = LCD_WIDTH,
	};

	for (uint16_t y = 0; y < LCD_HEIGHT; y++) {
		display_write(display_dev, 0, y, &desc, line_buf);
	}
}

static void draw_box(uint16_t *buf, int x1, int y1, int x2, int y2, uint16_t color, int thickness)
{
	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	if (x2 >= LCD_WIDTH) x2 = LCD_WIDTH - 1;
	if (y2 >= LCD_HEIGHT) y2 = LCD_HEIGHT - 1;
	if (x1 >= x2 || y1 >= y2) return;

	/* Horizontal top and bottom lines */
	for (int t = 0; t < thickness; t++) {
		int top = y1 + t;
		int bot = y2 - t;
		if (top < LCD_HEIGHT) {
			for (int x = x1; x <= x2; x++) buf[top * LCD_WIDTH + x] = color;
		}
		if (bot >= 0) {
			for (int x = x1; x <= x2; x++) buf[bot * LCD_WIDTH + x] = color;
		}
	}

	/* Vertical left and right lines */
	for (int t = 0; t < thickness; t++) {
		int left = x1 + t;
		int right = x2 - t;
		if (left < LCD_WIDTH) {
			for (int y = y1; y <= y2; y++) buf[y * LCD_WIDTH + left] = color;
		}
		if (right >= 0) {
			for (int y = y1; y <= y2; y++) buf[y * LCD_WIDTH + right] = color;
		}
	}
}

/* AI Inference Worker Thread */
static void ai_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("[AI_THREAD] Background AI inference thread started");

	blazeface_box_t local_boxes[MAX_DETECTED_FACES];

	while (1) {
		k_sem_take(&s_ai_sem, K_FOREVER);

		uint32_t infer_ms = 0;
		int num_faces = blazeface_detector_run(s_ai_frame, local_boxes, MAX_DETECTED_FACES, &infer_ms);

		k_mutex_lock(&s_box_mutex, K_FOREVER);
		s_detected_box_count = (num_faces > 0) ? num_faces : 0;
		for (int i = 0; i < s_detected_box_count; i++) {
			s_detected_boxes[i] = local_boxes[i];
		}
		s_last_infer_time_ms = infer_ms;
		k_mutex_unlock(&s_box_mutex);

		s_ai_busy = false;
	}
}

int main(void)
{
	int ret;
	const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	const struct device *const camera_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));

	LOG_INF("==================================================");
	LOG_INF(" M5Stack CoreS3 BlazeFace AI Face Detection       ");
	LOG_INF(" Zephyr OS Build: %s", KERNEL_VERSION_STRING);
	LOG_INF(" Model: BlazeFace INT8 (128x128 SSD Face Detector)");
	LOG_INF("==================================================");

	/* 1. Initialize Display (ILI9342C) */
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device (%s) not ready!", display_dev->name);
		return -ENODEV;
	}
	LOG_INF("Display ready: %s", display_dev->name);
	printk("[MAIN] Turning on display blanking off...\n");
	display_blanking_off(display_dev);
	printk("[MAIN] Clearing screen to blue...\n");
	clear_screen_color(display_dev, 0x001F); /* Blue */
	printk("[MAIN] Screen cleared. Sleeping 300ms...\n");
	k_msleep(300);

	/* 2. Initialize BlazeFace Detector */
	printk("[MAIN] Initializing BlazeFace Detector...\n");
	ret = blazeface_detector_init();
	printk("[MAIN] blazeface_detector_init returned: %d\n", ret);
	if (ret < 0) {
		LOG_ERR("Failed to initialize BlazeFace Detector: %d", ret);
		return ret;
	}

	/* 3. Start AI Background Worker Thread */
	k_thread_create(&s_ai_thread_data, s_ai_stack,
			K_THREAD_STACK_SIZEOF(s_ai_stack),
			ai_thread_entry, NULL, NULL, NULL,
			AI_THREAD_PRIORITY, 0, K_NO_WAIT);

	/* 4. Initialize Camera Interface (GC0308) */
	if (!device_is_ready(camera_dev)) {
		LOG_ERR("Camera device (%s) not ready!", camera_dev->name);
		return -ENODEV;
	}
	LOG_INF("Camera ready: %s", camera_dev->name);

	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width = LCD_WIDTH,
		.height = LCD_HEIGHT,
		.pitch = LCD_WIDTH * 2,
	};
	ret = video_set_format(camera_dev, &fmt);
	if (ret < 0) {
		LOG_ERR("Failed to set video format: %d", ret);
		return ret;
	}

	/* Allocate Video Buffer */
	struct video_buffer *vbuf = video_buffer_aligned_alloc(FRAME_SIZE, 32, K_NO_WAIT);
	if (vbuf == NULL) {
		LOG_ERR("Failed to allocate video buffer!");
		return -ENOMEM;
	}
	vbuf->type = VIDEO_BUF_TYPE_OUTPUT;
	vbuf->bytesused = FRAME_SIZE;
	ret = video_enqueue(camera_dev, vbuf);
	if (ret < 0) {
		LOG_ERR("Failed to enqueue buffer: %d", ret);
		return ret;
	}

	ret = video_stream_start(camera_dev, VIDEO_BUF_TYPE_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to start stream: %d", ret);
		return ret;
	}
	LOG_INF("Video stream started successfully! Starting live preview & AI loop...");
	LCD_CAM.lc_dma_int_ena.cam_vsync_int_ena = 1;

	/* 5. Live Preview & AI Trigger Loop */
	uint32_t frame_count = 0;
	uint32_t last_report_time = k_uptime_get_32();

	while (1) {
		struct video_buffer *rx_buf = NULL;
		ret = video_dequeue(camera_dev, &rx_buf, K_MSEC(1000));
		if (ret < 0) {
			continue;
		}

		uint16_t *frame_pixels = (uint16_t *)rx_buf->buffer;

		/* Asynchronously trigger AI thread if idle */
		if (!s_ai_busy) {
			memcpy(s_ai_frame, frame_pixels, FRAME_SIZE);
			s_ai_busy = true;
			k_sem_give(&s_ai_sem);
		}

		/* Overlay latest detected bounding boxes */
		k_mutex_lock(&s_box_mutex, K_FOREVER);
		int box_count = s_detected_box_count;
		for (int i = 0; i < box_count; i++) {
			blazeface_box_t *box = &s_detected_boxes[i];
			draw_box(frame_pixels, box->x1, box->y1, box->x2, box->y2, COLOR_GREEN, 3);
		}
		uint32_t infer_ms = s_last_infer_time_ms;
		k_mutex_unlock(&s_box_mutex);

		/* Render frame onto ILI9342C display */
		struct display_buffer_descriptor desc = {
			.buf_size = rx_buf->bytesused,
			.width = LCD_WIDTH,
			.height = LCD_HEIGHT,
			.pitch = LCD_WIDTH,
		};
		display_write(display_dev, 0, 0, &desc, rx_buf->buffer);

		/* Re-enqueue the buffer for next frame */
		video_enqueue(camera_dev, rx_buf);

		frame_count++;
		uint32_t now = k_uptime_get_32();
		if (now - last_report_time >= 2000) {
			uint32_t elapsed_ms = now - last_report_time;
			uint32_t fps_x10 = (frame_count * 10000) / elapsed_ms;
			printk("[APP] Preview: %u frames (~%u.%u FPS) | Faces: %d | AI Infer: %u ms\n",
				frame_count, fps_x10 / 10, fps_x10 % 10, box_count, infer_ms);
			frame_count = 0;
			last_report_time = now;
		}
	}

	return 0;
}
