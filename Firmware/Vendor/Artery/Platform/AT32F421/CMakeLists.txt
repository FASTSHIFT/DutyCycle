cmake_minimum_required(VERSION 3.13)

# Generate compile_commands.json for IDE/tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Toolchain must be set before project()
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      "${CMAKE_CURRENT_SOURCE_DIR}/arm-none-eabi.cmake"
      CACHE PATH "Toolchain file")
endif()

project(DutyCycle C CXX ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# ==============================================================================
# Directory aliases
# ==============================================================================
set(PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VENDOR_DIR ${PLATFORM_DIR}/../..)
set(PROJECT_ROOT ${VENDOR_DIR}/../..)
set(APP_DIR ${PROJECT_ROOT}/App)
set(EXTERNAL_DIR ${PROJECT_ROOT}/External)
set(FRAMEWORKS_DIR ${PROJECT_ROOT}/Frameworks)
set(FWLIBS_DIR ${PLATFORM_DIR}/FWLibs)
set(LIBRARIES_DIR ${VENDOR_DIR}/Libraries)
set(APPLICATION_DIR ${VENDOR_DIR}/Application)
set(ARDUINO_API_DIR ${VENDOR_DIR}/ArduinoAPI)
set(CORE_DIR ${PLATFORM_DIR}/Core)

# ==============================================================================
# Build type configuration
# ==============================================================================
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Build type (Debug/Release)" FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Release: size-optimized (62KB flash limit)
set(CMAKE_C_FLAGS_RELEASE
    "-Os -g -DNDEBUG"
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE
    "-Os -g -DNDEBUG"
    CACHE STRING "" FORCE)

# Debug configuration
set(CMAKE_C_FLAGS_DEBUG
    "-O0 -g3 -DDEBUG"
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG
    "-O0 -g3 -DDEBUG"
    CACHE STRING "" FORCE)

# ==============================================================================
# Compile definitions
# ==============================================================================
add_compile_definitions(
  AT32F421K8U7
  USE_STDPERIPH_DRIVER
  ARDUINO=111
  ARGPARSE_DISABLE_FLOAT
  PRINT_DISABLE_FLOAT)

# ==============================================================================
# Compile flags
# ==============================================================================
add_compile_options(
  -ffunction-sections
  -fdata-sections
  -fno-strict-aliasing
  -fno-exceptions
  -fno-unwind-tables
  -fno-asynchronous-unwind-tables
  -flto)

# C++ specific flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

# ==============================================================================
# Linker script
# ==============================================================================
set(LINKER_SCRIPT
    ${FWLIBS_DIR}/cmsis/cm4/device_support/startup/gcc/linker/AT32F421x8_FLASH.ld)
add_link_options(
  -T${LINKER_SCRIPT}
  -flto
  -Wl,--gc-sections
  -Wl,--print-memory-usage
  -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map,--cref)

# ==============================================================================
# Include directories
# ==============================================================================
include_directories(
  # Platform
  ${PLATFORM_DIR}/Config
  ${FWLIBS_DIR}/cmsis/cm4/core_support
  ${FWLIBS_DIR}/cmsis/cm4/device_support
  ${FWLIBS_DIR}/drivers/inc
  ${CORE_DIR}
  # ArduinoAPI
  ${ARDUINO_API_DIR}
  # Application
  ${APPLICATION_DIR}
  ${APPLICATION_DIR}/HAL
  ${APPLICATION_DIR}/flashdb_port
  ${APPLICATION_DIR}/umm_malloc_port
  # Project root (for "Frameworks/..." includes)
  ${PROJECT_ROOT}
  # App
  ${APP_DIR}
  # External
  ${EXTERNAL_DIR}/FlashDB/inc
  ${EXTERNAL_DIR}/FlashDB/port/fal/inc
  # Libraries
  ${LIBRARIES_DIR})

# ==============================================================================
# Sources: AT32 FWLibs (startup + system + peripheral drivers)
# ==============================================================================
set(FWLIBS_SOURCES
    ${FWLIBS_DIR}/cmsis/cm4/device_support/startup/gcc/startup_at32f421.s
    ${FWLIBS_DIR}/cmsis/cm4/device_support/system_at32f421.c)

# Only compile drivers that are actually used (saves flash)
set(FWLIBS_DRIVER_SOURCES
    ${FWLIBS_DIR}/drivers/src/at32f421_crm.c
    ${FWLIBS_DIR}/drivers/src/at32f421_gpio.c
    ${FWLIBS_DIR}/drivers/src/at32f421_misc.c
    ${FWLIBS_DIR}/drivers/src/at32f421_tmr.c
    ${FWLIBS_DIR}/drivers/src/at32f421_usart.c
    ${FWLIBS_DIR}/drivers/src/at32f421_flash.c
    ${FWLIBS_DIR}/drivers/src/at32f421_adc.c
    ${FWLIBS_DIR}/drivers/src/at32f421_dma.c
    ${FWLIBS_DIR}/drivers/src/at32f421_exint.c
    ${FWLIBS_DIR}/drivers/src/at32f421_pwc.c
    ${FWLIBS_DIR}/drivers/src/at32f421_scfg.c
    ${FWLIBS_DIR}/drivers/src/at32f421_wdt.c
    ${FWLIBS_DIR}/drivers/src/at32f421_ertc.c
    ${FWLIBS_DIR}/drivers/src/at32f421_debug.c)
list(APPEND FWLIBS_SOURCES ${FWLIBS_DRIVER_SOURCES})

# ==============================================================================
# Sources: Core (MCU platform drivers)
# ==============================================================================
file(GLOB CORE_SOURCES ${CORE_DIR}/*.c ${CORE_DIR}/*.cpp)

# ==============================================================================
# Sources: ArduinoAPI (only files actually used)
# ==============================================================================
set(ARDUINO_SOURCES
    ${ARDUINO_API_DIR}/Arduino.c
    ${ARDUINO_API_DIR}/dtostrf.c
    ${ARDUINO_API_DIR}/itoa.c
    ${ARDUINO_API_DIR}/Print.cpp
    ${ARDUINO_API_DIR}/Tone.cpp)

# ==============================================================================
# Sources: Application (HAL + ports)
# ==============================================================================
file(GLOB APPLICATION_HAL_SOURCES ${APPLICATION_DIR}/HAL/*.cpp
     ${APPLICATION_DIR}/HAL/*.c)
# Exclude rt_sys.cpp (Keil-specific retarget)
list(FILTER APPLICATION_HAL_SOURCES EXCLUDE REGEX ".*rt_sys\\.cpp$")

set(APPLICATION_PORT_SOURCES
    ${APPLICATION_DIR}/flashdb_port/fal_port.cpp
    ${APPLICATION_DIR}/umm_malloc_port/new.cpp
    ${APPLICATION_DIR}/umm_malloc_port/umm_malloc_port.c)

set(APPLICATION_MAIN_SOURCES ${APPLICATION_DIR}/main.cpp)

# ==============================================================================
# Sources: App
# ==============================================================================
set(APP_SOURCES
    ${APP_DIR}/App.cpp
    ${APP_DIR}/Service/DataProc/DataProc.cpp
    ${APP_DIR}/Service/DataProc/DP_Button.cpp
    ${APP_DIR}/Service/DataProc/DP_Power.cpp
    ${APP_DIR}/Service/DataProc/DP_Version.cpp
    ${APP_DIR}/Service/DataProc/DP_WatchDog.cpp
    ${APP_DIR}/Service/DataProc/DP_Ctrl.cpp
    ${APP_DIR}/Service/DataProc/DP_Clock.cpp
    ${APP_DIR}/Service/DataProc/DP_TimeMonitor.cpp
    ${APP_DIR}/Service/DataProc/DP_Audio.cpp
    ${APP_DIR}/Service/DataProc/DP_Shell.cpp
    ${APP_DIR}/Service/DataProc/DP_KVDB.cpp
    ${APP_DIR}/Service/DataProc/DP_Alarm.cpp
    ${APP_DIR}/Service/DataProc/_DP_Template.cpp
    ${APP_DIR}/Service/DataProc/Helper/_Template_Helper.cpp
    ${APP_DIR}/Service/DataProc/Helper/Power_Helper.cpp
    ${APP_DIR}/Service/DataProc/Helper/Audio_Helper.cpp
    ${APP_DIR}/Service/DataProc/Helper/KVDB_Helper.cpp
    ${APP_DIR}/Service/DataProc/Helper/MotorCtrl.cpp
    ${APP_DIR}/Service/HAL/HAL_Template.cpp
    ${APP_DIR}/Utils/ButtonEvent/ButtonEvent.cpp
    ${APP_DIR}/Utils/Shell/Shell.c
    ${APP_DIR}/Utils/easing/easing.c)

# ==============================================================================
# Sources: Frameworks
# ==============================================================================
file(GLOB FRAMEWORKS_SOURCES
     ${FRAMEWORKS_DIR}/DataBroker/*.cpp
     ${FRAMEWORKS_DIR}/DeviceManager/*.cpp)

# ==============================================================================
# Sources: External
# ==============================================================================
set(FLASHDB_SOURCES
    ${EXTERNAL_DIR}/FlashDB/src/fdb.c
    ${EXTERNAL_DIR}/FlashDB/src/fdb_kvdb.c
    ${EXTERNAL_DIR}/FlashDB/src/fdb_utils.c
    ${EXTERNAL_DIR}/FlashDB/port/fal/src/fal.c
    ${EXTERNAL_DIR}/FlashDB/port/fal/src/fal_flash.c
    ${EXTERNAL_DIR}/FlashDB/port/fal/src/fal_partition.c
    ${EXTERNAL_DIR}/FlashDB/port/fal/src/fal_rtt.c)

set(ARGPARSE_SOURCES ${EXTERNAL_DIR}/argparse/argparse.c)

set(UMM_MALLOC_SOURCES
    ${EXTERNAL_DIR}/umm_malloc/src/umm_info.c
    ${EXTERNAL_DIR}/umm_malloc/src/umm_integrity.c
    ${EXTERNAL_DIR}/umm_malloc/src/umm_malloc.c
    ${EXTERNAL_DIR}/umm_malloc/src/umm_poison.c)

# ==============================================================================
# Sources: Libraries
# ==============================================================================
set(LIB_SOURCES
    ${LIBRARIES_DIR}/StackInfo/StackInfo.c
    ${LIBRARIES_DIR}/cm_backtrace/cm_backtrace.c)

# ==============================================================================
# Executable
# ==============================================================================
add_executable(
  ${PROJECT_NAME}
  # Startup and drivers
  ${FWLIBS_SOURCES}
  ${CORE_SOURCES}
  ${ARDUINO_SOURCES}
  # Application
  ${APPLICATION_HAL_SOURCES}
  ${APPLICATION_PORT_SOURCES}
  ${APPLICATION_MAIN_SOURCES}
  # App logic
  ${APP_SOURCES}
  ${FRAMEWORKS_SOURCES}
  # External libraries
  ${FLASHDB_SOURCES}
  ${ARGPARSE_SOURCES}
  ${UMM_MALLOC_SOURCES}
  # Libraries
  ${LIB_SOURCES})

# ==============================================================================
# Warning flags per source type
# ==============================================================================
# Project code: strict warnings
set(PROJECT_WARNING_FLAGS
    "-Wall;-Wextra;-Wno-unused-parameter;-Wno-missing-field-initializers")
set_source_files_properties(
  ${CORE_SOURCES} ${APPLICATION_HAL_SOURCES} ${APPLICATION_PORT_SOURCES}
  ${APPLICATION_MAIN_SOURCES} ${APP_SOURCES} ${FRAMEWORKS_SOURCES}
  PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNING_FLAGS}")

# Third-party: suppress warnings
set_source_files_properties(
  ${FWLIBS_SOURCES} ${ARDUINO_SOURCES} ${FLASHDB_SOURCES} ${ARGPARSE_SOURCES}
  ${UMM_MALLOC_SOURCES} ${LIB_SOURCES}
  PROPERTIES COMPILE_OPTIONS "-w")

# Special: HAL_FaultHandle.cpp needs -fno-lto
file(GLOB _FAULT_HANDLE ${APPLICATION_DIR}/HAL/HAL_FaultHandle.cpp)
if(_FAULT_HANDLE)
  set_source_files_properties(${_FAULT_HANDLE}
                              PROPERTIES COMPILE_OPTIONS "-fno-lto;-Wall")
endif()

# ==============================================================================
# Post-build: generate .bin and .hex, print size
# ==============================================================================
set_target_properties(
  ${PROJECT_NAME} PROPERTIES SUFFIX ".elf" RUNTIME_OUTPUT_DIRECTORY
                                           ${CMAKE_BINARY_DIR})

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}>
          ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
  COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}>
          ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
  COMMENT "Generating .bin and .hex, size report:")

# ==============================================================================
# Build info
# ==============================================================================
message(STATUS "========================================")
message(STATUS "DutyCycle - AT32F421K8U7 CMake Build")
message(STATUS "========================================")
message(STATUS "Flash limit: 62KB (2KB reserved for KVDB)")
message(STATUS "RAM: 16KB")
message(STATUS "Linker Script: ${LINKER_SCRIPT}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
