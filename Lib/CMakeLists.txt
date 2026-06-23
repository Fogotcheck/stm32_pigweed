cmake_minimum_required(VERSION 3.22)

add_library(${PROJECT_NAME}_LIB_INTERFACE INTERFACE)


add_subdirectory(FreeRTOS)
add_subdirectory(Pigweed)
add_subdirectory(logger)

target_link_libraries(
    ${PROJECT_NAME}_LIB_INTERFACE
    INTERFACE
    freertos_kernel
    ${PROJECT_NAME}_PIGWEED_INTERFACE
    ${PROJECT_NAME}_LOGGER
)
