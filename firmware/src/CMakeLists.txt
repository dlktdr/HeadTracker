cmake_minimum_required(VERSION 3.20.0)

# Get the Git Revision and Most recent version tag
execute_process(
  COMMAND git rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_REV
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(ret EQUAL "1")
  no_git()
endif()
execute_process(
  COMMAND git describe --tags --abbrev=0
  OUTPUT_VARIABLE GIT_TAG_RAW
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(ret EQUAL "1")
  no_git()
endif()
string(REPLACE "v" "" GIT_TAG "${GIT_TAG_RAW}")
function(no_git)
  message( FATAL_ERROR "Unable to fetch the project GIT status, be sure GIT is installed and in your path")
endfunction()
message(STATUS "Git Revision ${GIT_REV}")
message(STATUS "Git Tag ${GIT_TAG}")

# Rebuild all the the settings for the project
message(STATUS "Building the settings files (buildsettings.py) from ../settings.csv")
execute_process(
  COMMAND ../settings/python buildsettings.py
)

message(STATUS "Board=${BOARD}")
set (BOARD_HW_REV "")

# NRF52 Boards
if((${BOARD} STREQUAL "arduino_nano_33_ble") OR
   (${BOARD} STREQUAL "xiao_ble/nrf52840/sense") OR
   (${BOARD} STREQUAL "xiao_ble/nrf52840") OR
   (${BOARD} STREQUAL "dtqsys_ht"))
  message(STATUS "  Board is a NRF52 based board")

  # Pick the correct project file for this architecture
  set (CONF_FILE zephyr/nrf_prj.conf)

  # The sense revision 2 uses the same board file as the original
  # but a few different defines in the code.
  if(DEFINED BOARD_REV2)
    set(ignoreMe "${BOARD_REV2}")
    set (BOARD_HW_REV "2")
    message(STATUS "  Board is a Revision 2 Nano 33 BLE")
    set (BOARD "arduino_nano_33_ble")
    # SEE BELOW for defined passed to compiler
  endif()

  #DTQ Board, use local board file from /zephyr/boards
  if((${BOARD} STREQUAL "dtqsys_ht"))
    set (BOARD_ROOT "${CMAKE_SOURCE_DIR}/zephyr")
  endif()

#ESP32C3 Boards
elseif(${BOARD} STREQUAL "esp32c3_devkitm")
  message(STATUS "  Board is a ESP32C3 based board")
  set (CONF_FILE zephyr/esp32c3_prj.conf)

# M5Stick CPLUS1
elseif((${BOARD} STREQUAL "m5stickc_plus/esp32/procpu") OR
      (${BOARD} STREQUAL "m5stickc_plus"))
  message(STATUS "  Board is a ESP32 PICO based board")
  set(CONF_FILE zephyr/m5stickc_plus_prj.conf)

# Raspberry Pi Pico
elseif((${BOARD} STREQUAL "rpi_pico/rp2040/w") OR
      (${BOARD} STREQUAL "rpi_pico"))
  message(STATUS "  Board is a Raspberry Pi 2040 Pico")
  set(CONF_FILE zephyr/rpi_pico.conf)

else()
  message(FATAL_ERROR "ERROR: Board is not supported")
endif()

# Set the output filename in release mode
string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
if(NOT build_type STREQUAL "debug")
  message(STATUS "Building in Release mode")
  set(output_name "${BOARD}")
  if(NOT BOARD_HW_REV STREQUAL "")
    set(output_name "${output_name}_rev${BOARD_HW_REV}")
  endif()
  string(REPLACE "." "_" output_name "${output_name}-${GIT_TAG}-${GIT_REV}")
  string(REPLACE "/" "_" output_name "${output_name}")
  set(CONFIG_KERNEL_BIN_NAME "\"${output_name}\"" CACHE INTERNAL "")
  message(STATUS "Output File Name=${output_name}")
# Set KConfig Option to disable some optimizations in debug mode
# do this here so it's easy in VScode with a task to set the build type
else()
  message(STATUS "Building in Debug mode")
  set(CONFIG_DEBUG_OPTIMIZATIONS "y" CACHE INTERNAL "")
  set(NO_BUILD_TYPE_WARNING "y")
endif()



find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(HeadTracker DESCRIPTION "HeadTracker")

# Set optimization level to -O1 for debug builds -o0 casuses stack issues
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O1")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1")

# -DOPENOCD=/openocd-esp32/bin -DOPENOCD_DEFAULT_PATH=/openocd-esp32/share/openocd/scripts
if(${BOARD} MATCHES "esp32c3")
  target_compile_definitions(app PUBLIC "OPENOCD=\"c:\\openocd-esp32\\bin\"")
  target_compile_definitions(app PUBLIC "OPENOCD_DEFAULT_PATH=\"c:\\openocd-esp32\\share\\openocd\\scripts\"")
endif()

# Arduino Nano Sense Rev 2
if(DEFINED BOARD_REV2)
  target_compile_definitions(app PUBLIC BOARD_REV2=1)
endif()

target_compile_definitions(app PUBLIC FW_GIT_REV=${GIT_REV})
target_compile_definitions(app PUBLIC FW_VER_TAG=${GIT_TAG})
target_include_directories(app PRIVATE src/include)
target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE third-party/ArduinoJson/src)
target_include_directories(app PRIVATE third-party/BMI270_SensorAPI)

FILE(GLOB_RECURSE app_sources src/*.c*)
set(app_sources ${app_sources} third-party/BMI270_SensorAPI/bmi2.c)
set(app_sources ${app_sources} third-party/BMI270_SensorAPI/bmi270.c)
target_sources(app PRIVATE ${app_sources})
