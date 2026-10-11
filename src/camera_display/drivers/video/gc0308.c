/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT galaxycore_gc0308

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

#include "video_common.h"
#include "gc0308_regs.h"
#include "gc0308_settings.h"

LOG_MODULE_REGISTER(video_gc0308, CONFIG_VIDEO_LOG_LEVEL);

struct gc0308_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn_gpio;
#endif
};

struct gc0308_data {
	const struct device *dev;
	struct video_format fmt;
	bool streaming;
};

static int gc0308_read_reg(const struct device *dev, uint8_t reg_addr, uint8_t *val)
{
	const struct gc0308_config *cfg = dev->config;

	return i2c_write_read_dt(&cfg->i2c, &reg_addr, sizeof(reg_addr), val, sizeof(*val));
}

static int gc0308_write_reg(const struct device *dev, uint8_t reg_addr, uint8_t val)
{
	const struct gc0308_config *cfg = dev->config;
	uint8_t buf[2] = {reg_addr, val};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int gc0308_write_regs(const struct device *dev, const uint8_t (*regs)[2], size_t size)
{
	int ret;

	for (size_t i = 0; i < size; i++) {
		if (regs[i][0] == REG_DLY) {
			k_msleep(regs[i][1]);
		} else {
			ret = gc0308_write_reg(dev, regs[i][0], regs[i][1]);
			if (ret < 0) {
				LOG_ERR("Failed to write reg 0x%02x: %d", regs[i][0], ret);
				return ret;
			}
		}
	}
	return 0;
}

static int gc0308_hw_reset(const struct device *dev)
{
	const struct gc0308_config *cfg = dev->config;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (cfg->reset_gpio.port != NULL) {
		LOG_DBG("Asserting HW reset via GPIO");
		gpio_pin_set_dt(&cfg->reset_gpio, 1); /* Active (Low) */
		k_msleep(20);
		gpio_pin_set_dt(&cfg->reset_gpio, 0); /* Inactive (High) */
		k_msleep(30);
	}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	if (cfg->pwdn_gpio.port != NULL) {
		/* Power on (Active High means 0 is power on) */
		gpio_pin_set_dt(&cfg->pwdn_gpio, 0);
		k_msleep(10);
	}
#endif

	return 0;
}

static int gc0308_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct gc0308_data *data = dev->data;
	int ret = 0;

	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565 &&
	    fmt->pixelformat != VIDEO_PIX_FMT_YUYV) {
		LOG_ERR("Unsupported pixel format: 0x%x", fmt->pixelformat);
		return -ENOTSUP;
	}

	/* Switch to Page 0 */
	ret |= gc0308_write_reg(dev, GC0308_REG_RESET_RELATED, GC0308_PAGE_0);

	if (fmt->pixelformat == VIDEO_PIX_FMT_RGB565) {
		/* RGB565: OUTPUT_FMT 0xa6 per esp32-camera driver */
		ret |= gc0308_write_reg(dev, GC0308_REG_OUTPUT_FMT, 0xa6);
	} else {
		/* YUV422: OUTPUT_FMT Y Cb Y Cr (0xa2) */
		ret |= gc0308_write_reg(dev, GC0308_REG_OUTPUT_FMT, 0xa2);
	}

	/* Configure sensor output resolution */
	if (fmt->width == 320 && fmt->height == 240) {
		/* Page 0: set 640x480 full window (+ 8 for GC0308 sensor array border) */
		ret |= gc0308_write_reg(dev, GC0308_REG_RESET_RELATED, GC0308_PAGE_0);
		ret |= gc0308_write_reg(dev, GC0308_REG_ROW_START_H, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_ROW_START_L, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_COL_START_H, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_COL_START_L, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_WIN_HEIGHT_H, (480 + 8) >> 8);
		ret |= gc0308_write_reg(dev, GC0308_REG_WIN_HEIGHT_L, (480 + 8) & 0xff);
		ret |= gc0308_write_reg(dev, GC0308_REG_WIN_WIDTH_H, (640 + 8) >> 8);
		ret |= gc0308_write_reg(dev, GC0308_REG_WIN_WIDTH_L, (640 + 8) & 0xff);

		/* Page 1: enable 1/2 subsampling */
		ret |= gc0308_write_reg(dev, GC0308_REG_RESET_RELATED, GC0308_PAGE_1);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_EN, 0x80);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_EN2, 0x01);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_MODE, 0x22);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_Y0, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_Y1, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_UV0, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_UV1, 0x00);

		/* Page 0: Crop Window mode to crop to EXACTLY 320x240 */
		ret |= gc0308_write_reg(dev, GC0308_REG_RESET_RELATED, GC0308_PAGE_0);
		ret |= gc0308_write_reg(dev, 0x46, 0x80); /* Enable crop window mode */
		ret |= gc0308_write_reg(dev, 0x47, 0x00); /* Crop Y0 = 0 */
		ret |= gc0308_write_reg(dev, 0x48, 0x00); /* Crop X0 = 0 */
		ret |= gc0308_write_reg(dev, 0x49, 0x00); /* Crop Height H = 0 */
		ret |= gc0308_write_reg(dev, 0x4a, 0xf0); /* Crop Height L = 240 */
		ret |= gc0308_write_reg(dev, 0x4b, 0x01); /* Crop Width H = 1 */
		ret |= gc0308_write_reg(dev, 0x4c, 0x40); /* Crop Width L = 0x40 (0x0140 = 320) */

		/* Subsample measurement window for AEC / AWB statistics */
		ret |= gc0308_write_reg(dev, 0xf7, 0x02);
		ret |= gc0308_write_reg(dev, 0xf8, 0x02);
		ret |= gc0308_write_reg(dev, 0xf9, 0x4e); /* 78 (320/4 - 2) */
		ret |= gc0308_write_reg(dev, 0xfa, 0x3a); /* 58 (240/4 - 2) */

		LOG_INF("GC0308 hardware configured for exact 320x240 (subsample + crop window)");
	} else if (fmt->width == 640 && fmt->height == 480) {
		/* Page 0: set 640x480 full window */
		ret |= gc0308_write_reg(dev, GC0308_REG_RESET_RELATED, GC0308_PAGE_0);
		ret |= gc0308_write_reg(dev, 0x46, 0x00); /* Disable crop window mode */
		ret |= gc0308_write_reg(dev, GC0308_REG_ROW_START_H, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_ROW_START_L, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_COL_START_H, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_COL_START_L, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_WIN_HEIGHT_H, (480 + 8) >> 8);
		ret |= gc0308_write_reg(dev, GC0308_REG_WIN_HEIGHT_L, (480 + 8) & 0xff);
		ret |= gc0308_write_reg(dev, GC0308_REG_WIN_WIDTH_H, (640 + 8) >> 8);
		ret |= gc0308_write_reg(dev, GC0308_REG_WIN_WIDTH_L, (640 + 8) & 0xff);

		/* Page 1: disable subsampling */
		ret |= gc0308_write_reg(dev, GC0308_REG_RESET_RELATED, GC0308_PAGE_1);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_EN, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_EN2, 0x00);
		ret |= gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE_MODE, 0x11);

		/* Return to Page 0 */
		ret |= gc0308_write_reg(dev, GC0308_REG_RESET_RELATED, GC0308_PAGE_0);
		LOG_INF("GC0308 hardware configured for 640x480 (full)");
	} else {
		LOG_ERR("Resolution %ux%u not supported", fmt->width, fmt->height);
		return -ENOTSUP;
	}

	data->fmt = *fmt;
	return ret;
}

static int gc0308_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct gc0308_data *data = dev->data;

	*fmt = data->fmt;
	return 0;
}

static int gc0308_get_caps(const struct device *dev, struct video_caps *caps)
{
	static const struct video_format_cap fmts[] = {
		{
			.pixelformat = VIDEO_PIX_FMT_RGB565,
			.width_min = 160,
			.width_max = 640,
			.height_min = 120,
			.height_max = 480,
			.width_step = 16,
			.height_step = 8,
		},
		{
			.pixelformat = VIDEO_PIX_FMT_YUYV,
			.width_min = 160,
			.width_max = 640,
			.height_min = 120,
			.height_max = 480,
			.width_step = 16,
			.height_step = 8,
		},
		{0}
	};

	caps->format_caps = fmts;
	return 0;
}

static int gc0308_set_stream(const struct device *dev, bool enable,
			     enum video_buf_type type)
{
	ARG_UNUSED(type);
	struct gc0308_data *data = dev->data;

	data->streaming = enable;
	LOG_INF("Stream %s", enable ? "started" : "stopped");
	return 0;
}

static DEVICE_API(video, gc0308_driver_api) = {
	.set_format = gc0308_set_fmt,
	.get_format = gc0308_get_fmt,
	.get_caps = gc0308_get_caps,
	.set_stream = gc0308_set_stream,
};

static int gc0308_init(const struct device *dev)
{
	const struct gc0308_config *cfg = dev->config;
	struct gc0308_data *data = dev->data;
	uint8_t chip_id = 0;
	int ret;

	data->dev = dev;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("Reset GPIO not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO: %d", ret);
			return ret;
		}
	}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	if (cfg->pwdn_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->pwdn_gpio)) {
			LOG_ERR("PWDN GPIO not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->pwdn_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure PWDN GPIO: %d", ret);
			return ret;
		}
	}
#endif

	/* Hardware reset sequence */
	gc0308_hw_reset(dev);

	/* Probe Chip ID */
	ret = gc0308_read_reg(dev, GC0308_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed to read Chip ID from GC0308 over I2C (addr 0x%02x): %d",
			cfg->i2c.addr, ret);
		return ret;
	}

	if (chip_id != GC0308_CHIP_ID_VAL) {
		LOG_WRN("GC0308 Chip ID mismatch! Read: 0x%02x (Expected: 0x%02x)",
			chip_id, GC0308_CHIP_ID_VAL);
	} else {
		LOG_INF("GC0308 camera sensor successfully detected! Chip ID: 0x%02x", chip_id);
	}

	/* Software Reset */
	ret = gc0308_write_reg(dev, GC0308_REG_RESET_RELATED, GC0308_RESET_SW);
	if (ret < 0) {
		LOG_ERR("Failed to issue software reset: %d", ret);
		return ret;
	}
	k_msleep(50);

	/* Load default sensor registers */
	ret = gc0308_write_regs(dev, gc0308_sensor_default_regs,
				ARRAY_SIZE(gc0308_sensor_default_regs));
	if (ret < 0) {
		LOG_ERR("Failed to write default sensor settings: %d", ret);
		return ret;
	}

	/* Apply default format: RGB565 320x240 (CoreS3 LCD native resolution) */
	struct video_format def_fmt = {
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width = 320,
		.height = 240,
		.pitch = 320 * 2,
	};
	ret = gc0308_set_fmt(dev, &def_fmt);
	if (ret < 0) {
		LOG_ERR("Failed to apply default format: %d", ret);
		return ret;
	}

	LOG_INF("GC0308 initialized successfully (Default: 320x240 RGB565)");
	return 0;
}

#define GC0308_INIT(inst)                                                                          \
	static const struct gc0308_config gc0308_cfg_##inst = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, reset_gpios),                               \
			   (.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),))              \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, pwdn_gpios),                                \
			   (.pwdn_gpio = GPIO_DT_SPEC_INST_GET(inst, pwdn_gpios),))                \
	};                                                                                         \
	static struct gc0308_data gc0308_data_##inst;                                             \
	DEVICE_DT_INST_DEFINE(inst, gc0308_init, NULL, &gc0308_data_##inst,                        \
			      &gc0308_cfg_##inst, POST_KERNEL,                                     \
			      90, &gc0308_driver_api);                                              \
	VIDEO_DEVICE_DEFINE(gc0308_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(GC0308_INIT)
