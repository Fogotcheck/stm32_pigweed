cmake_minimum_required(VERSION 3.22)

set(CMAKE_SYSTEM_PROCESSOR "arm" CACHE STRING "")
set(CMAKE_SYSTEM_NAME "Generic" CACHE STRING "")
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
# Specify toolchain. NOTE When building from inside STM32CubeIDE the location of the toolchain is resolved by the "MCU Toolchain" project setting (via PATH).
set(TOOLCHAIN_PREFIX "arm-none-eabi-")

find_program(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc" HINTS ${TOOLCHAIN_PATH})
find_program(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc" HINTS ${TOOLCHAIN_PATH})
find_program(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++" HINTS ${TOOLCHAIN_PATH})

find_program(CMAKE_AR "${TOOLCHAIN_PREFIX}ar" HINTS ${TOOLCHAIN_PATH})
find_program(CMAKE_LINKER "${TOOLCHAIN_PREFIX}ld" HINTS ${TOOLCHAIN_PATH})
find_program(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}objcopy" HINTS ${TOOLCHAIN_PATH})
find_program(CMAKE_RANLIB "${TOOLCHAIN_PREFIX}ranlib" HINTS ${TOOLCHAIN_PATH})
find_program(CMAKE_STRIP "${TOOLCHAIN_PREFIX}ld" HINTS ${TOOLCHAIN_PATH})

set(CMAKE_ASM_FLAGS "-x assembler-with-cpp")

add_compile_options(
  -fdata-sections
  -ffunction-sections
  -Wall
)

add_link_options(
  --specs=nano.specs
  -Wl,-Map=output.map,--cref,--print-memory-usage
  -Wl,--gc-sections
  -static
  -Wl,--start-group
  -lc
  -lm
  -Wl,--end-group
)

# Железобетонный абсолютный путь относительно папки Cmake/Toolchain
set(pigweed_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../Lib/Pigweed/pigweed)

# Подключаем главный скрипт сборки через локальный путь
include(${pigweed_SOURCE_DIR}/pw_build/pigweed.cmake)

# Подключаем официальные файлы бэкендов через локальный путь субмодуля
include(${pigweed_SOURCE_DIR}/pw_assert/backend.cmake)
include(${pigweed_SOURCE_DIR}/pw_chrono/backend.cmake)
include(${pigweed_SOURCE_DIR}/pw_log/backend.cmake)
include(${pigweed_SOURCE_DIR}/pw_sync/backend.cmake)
include(${pigweed_SOURCE_DIR}/pw_sys_io/backend.cmake)
include(${pigweed_SOURCE_DIR}/pw_thread/backend.cmake)
include(${pigweed_SOURCE_DIR}/pw_interrupt/backend.cmake)
include(${pigweed_SOURCE_DIR}/pw_unit_test/backend.cmake)

# ==============================================================================
# НАСТРОЙКА EMBEDDED-БЭКЕНДОВ PIGWEED (FREE_RTOS + СТАНДАРТНЫЙ ВЫВОД)
# ==============================================================================

# 1. Ассерты и логи (оставляем твою рабочую схему, которая уходит в твой _write)
pw_set_backend(pw_assert.check pw_assert.print_and_abort_check_backend)
pw_set_backend(pw_assert.assert pw_assert.print_and_abort_assert_backend)
pw_set_backend(pw_log pw_log_basic)

# Потокобезопасный вывод Pigweed — пусть думает, что пишет в stdio,
# твой ретаргетинг в syscalls.c подхватит его мьютексом FreeRTOS!
pw_set_backend(pw_sys_io pw_sys_io_stdio)

# Настройка бэкенда для прерываний Cortex-M
pw_set_backend(pw_interrupt.context pw_interrupt_cortex_m.context)

# 2. СИНХРОНИЗАЦИЯ (ПЕРЕВОДИМ НА FREE_RTOS)
# Вместо pw_sync_stl подключаем нативные примитивы твоей ОСРВ
pw_set_backend(pw_sync.interrupt_spin_lock pw_sync_freertos.interrupt_spin_lock)
pw_set_backend(pw_sync.binary_semaphore pw_sync_freertos.binary_semaphore)      # Бинарный семафор FreeRTOS
pw_set_backend(pw_sync.counting_semaphore pw_sync_freertos.counting_semaphore)  # Счётный семафор FreeRTOS
pw_set_backend(pw_sync.mutex pw_sync_freertos.mutex)                            # Мьютекс FreeRTOS
pw_set_backend(pw_sync.timed_mutex pw_sync_freertos.timed_mutex)                  # Тайм-мьютекс FreeRTOS

# Быстрые уведомления задач (Task Notifications), о которых мы говорили — супер-легковесные
pw_set_backend(pw_sync.thread_notification pw_sync_freertos.thread_notification)
pw_set_backend(pw_sync.timed_thread_notification pw_sync_freertos.timed_thread_notification)

# 3. ВРЕМЯ И ЧАСЫ (ПЕРЕВОДИМ НА FREE_RTOS)
# Связываем pw::chrono с тиками планировщика FreeRTOS (xTaskGetTickCount)
pw_set_backend(pw_chrono.system_clock pw_chrono_freertos.system_clock)

# 4. УПРАВЛЕНИЕ ПОТОКАМИ / ТАСКАМИ (ПЕРЕВОДИМ НА FREE_RTOS)
# Привязываем создание тасок C++ к xTaskCreate / xTaskCreateStatic
pw_set_backend(pw_thread.id pw_thread_freertos.id)
pw_set_backend(pw_thread.yield pw_thread_freertos.yield)
pw_set_backend(pw_thread.sleep pw_thread_freertos.sleep)
pw_set_backend(pw_thread.thread pw_thread_freertos.thread)
pw_set_backend(pw_thread.creation pw_thread_freertos.thread_creation)
pw_set_backend(pw_unit_test.main pw_unit_test.logging_main)

# ==============================================================================
# ОТКЛЮЧЕНИЕ СЛОЖНОГО ЗАКОММЕНТИРОВАННОГО МУСОРА, ВЫЗЫВАВШЕГО ВАРНИНГИ
# ==============================================================================
# pw_set_backend(pw_rpc.system_server targets.host.system_rpc_server)
# pw_set_backend(pw_perf_test.TIMER_INTERFACE_BACKEND pw_perf_test.chrono_timer)
# pw_set_backend(pw_thread.test_thread_context pw_thread_stl.test_thread_context)
# pw_set_backend(pw_thread.thread_iteration pw_thread_stl.thread_iteration)
# pw_set_backend(pw_system.target_hooks pw_system.stl_target_hooks)
# pw_set_backend(pw_system.rpc_server pw_system.hdlc_rpc_server)
# pw_set_backend(pw_system.io pw_system.sys_io_target_io)
# pw_set_backend(pw_trace pw_trace_tokenized)
