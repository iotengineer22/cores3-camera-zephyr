#pragma once
#include <stdint.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int64_t esp_timer_get_time(void) {
    return k_ticks_to_us_near64(k_uptime_ticks());
}

#ifdef __cplusplus
}
#endif
