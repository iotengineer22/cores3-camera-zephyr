/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define GC0308_NODE       DT_NODELABEL(gc0308)
#define LCD_CAM_DVP_NODE  DT_NODELABEL(lcd_cam_dvp)
#define I2C0_NODE         DT_NODELABEL(i2c0)

#define GC0308_I2C_ADDR   0x21
#define GC0308_REG_PID    0x00
#define GC0308_EXPECTED_PID 0x9b

int main(void)
{
	int ret;
	uint8_t chip_id = 0;

	LOG_INF("==============================================");
	LOG_INF(" M5Stack CoreS3 Camera Connection Test App   ");
	LOG_INF("==============================================");

	/* 1. Check I2C bus device */
	const struct device *i2c_dev = DEVICE_DT_GET(I2C0_NODE);
	if (!device_is_ready(i2c_dev)) {
		LOG_ERR("I2C0 controller is not ready!");
		return -1;
	}
	LOG_INF("I2C0 controller is ready.");

	/* 2. Direct I2C Probe for GC0308 Chip ID */
	LOG_INF("Probing GC0308 on I2C address 0x%02x...", GC0308_I2C_ADDR);
	uint8_t reg_addr = GC0308_REG_PID;
	ret = i2c_write_read(i2c_dev, GC0308_I2C_ADDR, &reg_addr, 1, &chip_id, 1);
	if (ret == 0) {
		LOG_INF(">>> Direct I2C Read Success: Reg[0x%02x] = 0x%02x", reg_addr, chip_id);
		if (chip_id == GC0308_EXPECTED_PID) {
			LOG_INF(">>> MATCH! GC0308 Image Sensor confirmed (PID: 0x%02x)!", chip_id);
		} else {
			LOG_WRN(">>> PID Mismatch! Expected 0x%02x, got 0x%02x",
				GC0308_EXPECTED_PID, chip_id);
		}
	} else {
		LOG_ERR("Direct I2C Read failed with error code: %d", ret);
	}

	/* 3. Check Zephyr Video Sensor Driver Device */
#if DT_NODE_HAS_STATUS_OKAY(GC0308_NODE)
	const struct device *cam_sensor_dev = DEVICE_DT_GET(GC0308_NODE);
	if (device_is_ready(cam_sensor_dev)) {
		LOG_INF("GC0308 Video Device is ready: %s", cam_sensor_dev->name);

		struct video_format fmt = {0};
		ret = video_get_format(cam_sensor_dev, &fmt);
		if (ret == 0) {
			char fourcc[5] = {
				(char)(fmt.pixelformat & 0xff),
				(char)((fmt.pixelformat >> 8) & 0xff),
				(char)((fmt.pixelformat >> 16) & 0xff),
				(char)((fmt.pixelformat >> 24) & 0xff),
				'\0'
			};
			LOG_INF("Current Camera Format: %ux%u (FourCC: %s)",
				fmt.width, fmt.height, fourcc);
		}
	} else {
		LOG_ERR("GC0308 Video Device is NOT ready!");
	}
#endif

	/* 4. Check Zephyr DVP Controller Device */
#if DT_NODE_HAS_STATUS_OKAY(LCD_CAM_DVP_NODE)
	const struct device *dvp_dev = DEVICE_DT_GET(LCD_CAM_DVP_NODE);
	if (device_is_ready(dvp_dev)) {
		LOG_INF("ESP32-S3 LCD_CAM DVP Interface is ready: %s", dvp_dev->name);
	} else {
		LOG_ERR("ESP32-S3 LCD_CAM DVP Interface is NOT ready!");
	}
#endif

	LOG_INF("Camera probe and verification sequence completed.");
	while (1) {
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
