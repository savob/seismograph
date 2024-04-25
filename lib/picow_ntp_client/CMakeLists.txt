add_library(picow_ntp_client INTERFACE)

target_sources(picow_ntp_client INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/picow_ntp_client.h
    ${CMAKE_CURRENT_LIST_DIR}/picow_ntp_client.c
    ${CMAKE_CURRENT_LIST_DIR}/lwipopts.h
)

target_include_directories(picow_ntp_client INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(picow_ntp_client INTERFACE
    pico_cyw43_arch_lwip_threadsafe_background
    pico_stdlib
)