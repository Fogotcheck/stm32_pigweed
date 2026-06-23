cmake_minimum_required(VERSION 3.22)

link_libraries(${PROJECT_NAME}_CHIP_INTERFACE)

set(pigweed_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pigweed)

pw_set_backend(pw_third_party.freertos freertos_kernel)
pw_set_backend(pw_thread.sleep pw_thread_freertos.sleep)

add_subdirectory(${pigweed_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/pigweed-build)

add_library(${PROJECT_NAME}_PIGWEED_INTERFACE INTERFACE)

target_link_libraries(${PROJECT_NAME}_PIGWEED_INTERFACE
    INTERFACE
    pw_alignment
    pw_log_basic
    pw_log
    pw_containers
    pw_string
    pw_unit_test
    pw_unit_test.logging_main
    pw_thread_freertos.thread
    pw_thread_freertos.sleep
    pw_sync_freertos.interrupt_spin_lock
    pw_chrono_freertos.system_clock

)
