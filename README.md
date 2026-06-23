# **[Проект](https://github.com/Fogotcheck/stm32_pigweed)**

## *Клонирование репозитория*

Клонировать репозиторий вы можете следующим образом:

```console
git clone --recurse-submodules "https://github.com/Fogotcheck/stm32_pigweed.git"
```

## *Структура проекта*

```
├── App
│   ├── inc
│   │    ├── app.hpp
│   │    ├── boot.hpp
│   │    └── bsp.hpp
│   │
│   └── src
|        └── main.cpp
│
├── Chip
│   └── STM32H743XX
│
├── Cmake
│   ├── Opts
│   ├── Toolchain
│   └── Utils
│
├── Lib
│   ├── FreeRTOS
│   ├── Pigweed
│   └── logger
│
├── Services
│
└── README.md

```

## *Сборка проекта*

### Настройка сборки проекта
Сборка проекта основана на [Cmake](https://cmake.org/). Минимальные требования для настройки сборки проекта - являются указание toolchain:

```console
CMAKE_TOOLCHAIN_FILE=./Cmake/Toolchain/Arm-none-eabi-toolchain.cmake
```
Таким образом настройка проекта для сборки arm-none-eabi будет выглядеть:

```console
cmake -DCMAKE_TOOLCHAIN_FILE=./Cmake/Toolchain/Arm-none-eabi-toolchain.cmake -B ./build
```

### Дополнительные утилиты настройки проекта

Утилита настройки [pre-commit](https://pre-commit.com)

```console
cmake --build ./build --target UTILS_BuildTest_VIRTUAL_ENV
```

Утилита настройки редактора [vscode](https://code.visualstudio.com/)
```console
cmake --build ./build --target UTILS_BuildTest_VSC_CONFIGURE
```
### Сборка проекта

Сборка осуществляется командой:

```console
cmake --build ./build
```

## *Дополнительные ссылки*

* [Трэкер задач](https://github.com/Fogotcheck/stm32_pigweed/issues/new/choose)
