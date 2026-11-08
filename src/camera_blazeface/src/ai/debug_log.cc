/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tensorflow/lite/micro/debug_log.h"
#include "tensorflow/lite/micro/micro_time.h"
#include "tensorflow/lite/micro/micro_common.h"
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void DebugLog(const char* format, va_list args)
{
#ifndef TF_LITE_STRIP_ERROR_STRINGS
    vprintf(format, args);
#endif
}

#ifndef TF_LITE_STRIP_ERROR_STRINGS
int DebugVsnprintf(char* buffer, size_t buf_size, const char* format, va_list vlist)
{
    return vsnprintf(buffer, buf_size, format, vlist);
}
#endif

#ifdef __cplusplus
}
#endif

namespace tflite {

uint32_t ticks_per_second()
{
    return CONFIG_SYS_CLOCK_TICKS_PER_SEC;
}

uint32_t GetCurrentTimeTicks()
{
    return (uint32_t)k_uptime_ticks();
}

namespace micro {
TFLMRegistration Register_CONV_2D();
TFLMRegistration Register_DEPTHWISE_CONV_2D();
TFLMRegistration Register_ADD();
TFLMRegistration Register_MAX_POOL_2D();
} // namespace micro

TFLMRegistration Register_CONV_2D()
{
    return micro::Register_CONV_2D();
}

TFLMRegistration Register_DEPTHWISE_CONV_2D()
{
    return micro::Register_DEPTHWISE_CONV_2D();
}

TFLMRegistration Register_ADD()
{
    return micro::Register_ADD();
}

TFLMRegistration Register_MAX_POOL_2D()
{
    return micro::Register_MAX_POOL_2D();
}

} // namespace tflite
