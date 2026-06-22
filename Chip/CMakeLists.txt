cmake_minimum_required(VERSION 3.22)

add_library(${PROJECT_NAME}_CHIP_INTERFACE INTERFACE)

add_subdirectory(STM32H743XX)



target_link_libraries(
    ${PROJECT_NAME}_CHIP_INTERFACE
    INTERFACE
    STM32H743ZITx_HAL
)
