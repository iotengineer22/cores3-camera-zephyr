cmake_minimum_required(VERSION 3.20.0)

list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(camera_connect)

target_include_directories(app PRIVATE
    ${ZEPHYR_BASE}/drivers/video
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/video
)

target_sources(app PRIVATE
    src/main.c
    drivers/video/gc0308.c
)
