add_library(accelerometer INTERFACE)

target_sources(accelerometer INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/accelerometer.h
    ${CMAKE_CURRENT_LIST_DIR}/accelerometer.c
)

target_include_directories(accelerometer INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(accelerometer INTERFACE
    pico_stdlib
    hardware_adc
    hardware_gpio
)