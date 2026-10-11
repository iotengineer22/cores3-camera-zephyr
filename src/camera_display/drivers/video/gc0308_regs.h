/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_GC0308_REGS_H_
#define ZEPHYR_DRIVERS_VIDEO_GC0308_REGS_H_

#define GC0308_REG_CHIP_ID          0x00
#define GC0308_CHIP_ID_VAL          0x9b

#define GC0308_REG_RESET_RELATED    0xfe
#define GC0308_RESET_SW             0x80
#define GC0308_PAGE_0               0x00
#define GC0308_PAGE_1               0x01

/* Page 0 Registers */
#define GC0308_REG_ROW_START_H      0x05
#define GC0308_REG_ROW_START_L      0x06
#define GC0308_REG_COL_START_H      0x07
#define GC0308_REG_COL_START_L      0x08
#define GC0308_REG_WIN_HEIGHT_H     0x09
#define GC0308_REG_WIN_HEIGHT_L     0x0a
#define GC0308_REG_WIN_WIDTH_H      0x0b
#define GC0308_REG_WIN_WIDTH_L      0x0c

#define GC0308_REG_CISCTL_MODE1     0x14
#define GC0308_REG_OUTPUT_FMT       0x24
#define GC0308_REG_PCLK_DIV         0x28

/* Page 1 Registers (Subsample) */
#define GC0308_REG_SUBSAMPLE_EN     0x53
#define GC0308_REG_SUBSAMPLE_MODE   0x54
#define GC0308_REG_SUBSAMPLE_EN2    0x55
#define GC0308_REG_SUBSAMPLE_Y0     0x56
#define GC0308_REG_SUBSAMPLE_Y1     0x57
#define GC0308_REG_SUBSAMPLE_UV0    0x58
#define GC0308_REG_SUBSAMPLE_UV1    0x59

#endif /* ZEPHYR_DRIVERS_VIDEO_GC0308_REGS_H_ */
