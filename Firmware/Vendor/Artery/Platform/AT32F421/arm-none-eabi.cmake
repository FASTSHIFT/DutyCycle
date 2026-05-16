set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Cortex-M4 flags (AT32F421 has no FPU, use soft float)
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=soft")

set(CMAKE_C_FLAGS_INIT "${MCU_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${MCU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${MCU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${MCU_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections")
