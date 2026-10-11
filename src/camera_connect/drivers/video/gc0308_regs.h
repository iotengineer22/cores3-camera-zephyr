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

#define GC0308_REG_OUTPUT_FMT       0x24
#define GC0308_REG_PCLK_DIV         0x28
#define GC0308_REG_CISCTL_MODE1     0x14

#endif /* ZEPHYR_DRIVERS_VIDEO_GC0308_REGS_H_ */
