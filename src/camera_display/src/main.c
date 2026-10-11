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

LOG_MODULE_REGISTER(camera_display, LOG_LEVEL_INF);

#define LCD_WIDTH   320
#define LCD_HEIGHT  240
#define FRAME_SIZE  (LCD_WIDTH * LCD_HEIGHT * 2) /* 153,600 bytes for RGB565 */

/* Line buffer for screen clearing to save ~153KB of precious SRAM */
static uint16_t line_buf[LCD_WIDTH];

static void clear_screen_color(const struct device *display_dev, uint16_t rgb565_color)
{
	/* Fill one line with Big-Endian RGB565 */
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

int main(void)
{
	int ret;
	const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	const struct device *const camera_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));

	LOG_INF("==============================================");
	LOG_INF(" M5Stack CoreS3 Camera LCD Preview Viewer     ");
	LOG_INF(" Zephyr OS Build: %s", KERNEL_VERSION_STRING);
	LOG_INF("==============================================");

	/* 1. Initialize and Test Built-in Display (ILI9342C) */
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device (%s) not ready!", display_dev->name);
		return -ENODEV;
	}
	LOG_INF("Display device ready: %s", display_dev->name);

	/* Turn on display and clear screen with Blue (0x001F) */
	display_blanking_off(display_dev);
	LOG_INF("Clearing display with Blue background...");
	clear_screen_color(display_dev, 0x001F);
	k_msleep(300);

	/* 2. Initialize Camera Interface */
	if (!device_is_ready(camera_dev)) {
		LOG_ERR("Camera device (%s) not ready!", camera_dev->name);
		return -ENODEV;
	}
	LOG_INF("Camera device ready: %s", camera_dev->name);

	/* Set video format to 320x240 RGB565 */
	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width = LCD_WIDTH,
		.height = LCD_HEIGHT,
		.pitch = LCD_WIDTH * 2,
	};
	printk("[MAIN] Calling video_set_format...\n");
	ret = video_set_format(camera_dev, &fmt);
	printk("[MAIN] video_set_format returned: %d\n", ret);
	if (ret < 0) {
		LOG_ERR("Failed to set video format: %d", ret);
		return ret;
	}

	/* 3. Allocate 2 Video Buffers from Pool for Seamless Double Buffering */
	printk("[MAIN] Allocating 2 video buffers...\n");
	for (int i = 0; i < 2; i++) {
		struct video_buffer *vbuf = video_buffer_aligned_alloc(FRAME_SIZE, 32, K_NO_WAIT);
		if (vbuf == NULL) {
			printk("[MAIN] ERROR: Failed to allocate buffer %d!\n", i);
			return -ENOMEM;
		}
		vbuf->type = VIDEO_BUF_TYPE_OUTPUT;
		vbuf->bytesused = FRAME_SIZE;
		ret = video_enqueue(camera_dev, vbuf);
		if (ret < 0) {
			printk("[MAIN] ERROR: Failed to enqueue buffer %d: %d\n", i, ret);
			return ret;
		}
		printk("[MAIN] Buffer %d enqueued at %p\n", i, vbuf);
	}

	/* 4. Start Camera Streaming */
	printk("[MAIN] Starting video stream...\n");
	ret = video_stream_start(camera_dev, VIDEO_BUF_TYPE_OUTPUT);
	if (ret < 0) {
		printk("[MAIN] ERROR: Failed to start stream: %d\n", ret);
		return ret;
	}
	printk("[MAIN] Video stream started successfully! Entering loop...\n");
	LCD_CAM.lc_dma_int_ena.cam_vsync_int_ena = 1;

	/* 5. Live Preview Loop */
	uint32_t frame_count = 0;
	uint32_t last_report_time = k_uptime_get_32();

	while (1) {
		struct video_buffer *rx_buf = NULL;
		uint32_t t0 = k_uptime_get_32();
		ret = video_dequeue(camera_dev, &rx_buf, K_MSEC(1000));
		uint32_t t1 = k_uptime_get_32();
		if (ret < 0) {
			printk("[MAIN] dequeue timeout (%d). INT_ST=0x%08x CTRL=0x%08x CTRL1=0x%08x\n",
				ret, LCD_CAM.lc_dma_int_st.val, LCD_CAM.cam_ctrl.val, LCD_CAM.cam_ctrl1.val);
			continue;
		}

		/* Render frame onto ILI9342C display */
		struct display_buffer_descriptor desc = {
			.buf_size = rx_buf->bytesused,
			.width = LCD_WIDTH,
			.height = LCD_HEIGHT,
			.pitch = LCD_WIDTH,
		};
		display_write(display_dev, 0, 0, &desc, rx_buf->buffer);
		uint32_t t2 = k_uptime_get_32();

		/* Re-enqueue the buffer to capture the next frame */
		video_enqueue(camera_dev, rx_buf);
		uint32_t t3 = k_uptime_get_32();

		frame_count++;
		uint32_t now = k_uptime_get_32();
		if (now - last_report_time >= 2000) {
			uint32_t elapsed_ms = now - last_report_time;
			uint32_t fps_x10 = (frame_count * 10000) / elapsed_ms;
			printk("[MAIN] Preview: %u frames (~%u.%u FPS) [deq=%ums, disp=%ums, enq=%ums] INT_RAW=0x%x G46=%d\n",
				frame_count, fps_x10 / 10, fps_x10 % 10,
				t1 - t0, t2 - t1, t3 - t2,
				LCD_CAM.lc_dma_int_raw.val, (int)((GPIO.in1.val >> 14) & 1));
			frame_count = 0;
			last_report_time = now;
		}
	}

	return 0;
}
