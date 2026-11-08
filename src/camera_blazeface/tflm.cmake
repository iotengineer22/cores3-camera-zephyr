# TensorFlow Lite Micro for Zephyr on ESP32-S3 (with ESP-NN Xtensa PIE Acceleration)

target_compile_definitions(app PRIVATE
    TF_LITE_STATIC_MEMORY
    ESP_NN=1
    CONFIG_NN_OPTIMIZED=1
    CONFIG_IDF_TARGET_ESP32S3=1
    CONFIG_IDF_TARGET_ARCH_XTENSA=1
    ESP_NN_DUAL_CORE_SUPPORTED=0
)

target_include_directories(app SYSTEM PRIVATE
    C:/Users/ioten/zephyr-sdk-1.0.1/gnu/xtensa-espressif_esp32s3_zephyr-elf/xtensa-espressif_esp32s3_zephyr-elf/include/c++/14.3.0/xtensa-espressif_esp32s3_zephyr-elf
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/flatbuffers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/gemmlowp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/ruy
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/fully_connected
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/softmax
)

set(TFLM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai/debug_log.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/compiler/mlir/lite/core/api/error_reporter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/compiler/mlir/lite/schema/schema_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/array.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/core/api/flatbuffer_conversions.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/core/api/tensor_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/core/c/common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/kernels/internal/common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/kernels/internal/portable_tensor_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/kernels/internal/quantization_util.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/kernels/internal/reference/comparisons.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/kernels/internal/reference/portable_tensor_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/kernels/internal/runtime_shape.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/kernels/internal/tensor_ctypes.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/kernels/internal/tensor_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/kernels/kernel_util.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/arena_allocator/non_persistent_arena_buffer_allocator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/arena_allocator/persistent_arena_buffer_allocator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/arena_allocator/recording_single_arena_buffer_allocator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/arena_allocator/single_arena_buffer_allocator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/flatbuffer_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/memory_helpers.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/memory_planner/greedy_memory_planner.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/memory_planner/linear_memory_planner.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/memory_planner/non_persistent_buffer_planner_shim.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_allocation_info.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_allocator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_context.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_interpreter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_interpreter_context.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_interpreter_graph.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_log.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_op_resolver.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_profiler.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_resource_variable.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/micro_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/recording_micro_allocator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/tflite_bridge/flatbuffer_conversions_bridge.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/tflite_bridge/micro_error_reporter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/kernel_util.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/micro_tensor_utils.cc

    # ESP-NN accelerated kernels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/conv.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/conv_common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/depthwise_conv.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/depthwise_conv_common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/add.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/add_common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/pooling.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/pooling_common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/pad.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/reshape.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/reshape_common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/quantize.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/quantize_common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/dequantize.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/dequantize_common.cc
)

set(ESP_NN_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/activation_functions/esp_nn_relu_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/activation_functions/esp_nn_hard_swish_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/common/esp_nn_mean_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/basic_math/esp_nn_add_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/basic_math/esp_nn_mul_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_opt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_opt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/fully_connected/esp_nn_fully_connected_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/softmax/esp_nn_softmax_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/softmax/esp_nn_softmax_opt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/logistic/esp_nn_logistic_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/pooling/esp_nn_avg_pool_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/pooling/esp_nn_max_pool_ansi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/common/esp_nn_multicore.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_mt_split.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/basic_math/esp_nn_mul_mt.c

    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/common/esp_nn_common_functions_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/common/esp_nn_dot_s8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/common/esp_nn_multiply_by_quantized_mult_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_s8_kx1_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/common/esp_nn_multiply_by_quantized_mult_ver1_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/activation_functions/esp_nn_relu_s8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/activation_functions/esp_nn_hard_swish_s8_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/common/esp_nn_mean_s8_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/basic_math/esp_nn_add_s8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/basic_math/esp_nn_add_mt_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/basic_math/esp_nn_mul_s8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/basic_math/esp_nn_mul_broadcast_s8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_s8_1x1_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_s8_3x3_opt_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_s8_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_s16_mult8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_s8_mult8_1x1_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_s16_mult4_1x1_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_conv_s8_filter_aligned_input_padded_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_s8_mult1_3x3_padded_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_s16_mult1_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_s16_mult1_3x3_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_s16_mult1_3x3_no_pad_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_s16_mult8_3x3_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_s16_mult4_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/convolution/esp_nn_depthwise_conv_s16_mult8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/fully_connected/esp_nn_fully_connected_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/fully_connected/esp_nn_fc_batch_conv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/fully_connected/esp_nn_fc_s8_mac16_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/fully_connected/esp_nn_fully_connected_s8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/fully_connected/esp_nn_fully_connected_per_ch_s8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/pooling/esp_nn_max_pool_s8_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/pooling/esp_nn_max_pool_s8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/pooling/esp_nn_avg_pool_s8_esp32s3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/pooling/esp_nn_avg_pool_s8_esp32s3.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/esp-nn/src/softmax/esp_nn_softmax_s8_esp32s3.c
)

target_sources(app PRIVATE ${TFLM_SRCS} ${ESP_NN_SRCS})

# Compile performance-critical math kernels with -O3
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/conv.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/depthwise_conv.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/add.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/pooling.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm_libs/tflite-micro/tensorflow/lite/micro/kernels/esp_nn/quantize.cc
    ${ESP_NN_SRCS}
    PROPERTIES COMPILE_OPTIONS "-O3"
)

target_compile_options(app PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
    -Wno-unused-parameter
    -Wno-missing-declarations
    -Wno-conversion
    -Wno-sign-conversion
    -Wno-sign-compare
    -Wno-type-limits
    -Wno-format
    -Wno-stringop-overflow
    -Wno-strict-aliasing
    -Wno-unused-function
)
