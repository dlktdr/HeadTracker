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

set (CONF_FILE zephyr/nrf_prj.conf)
message(STATUS "Board=${BOARD}")

# NRF52 Boards
if((${BOARD} STREQUAL "arduino_nano_33_ble") OR
    (${BOARD} STREQUAL "xiao_ble") OR
    (${BOARD} STREQUAL "dtqsys_ht"))
  message(STATUS "Board is a NRF52 based board")
  set (CONF_FILE zephyr/nrf_prj.conf)

  #DTQ Board, use local board file from /zephyr/boards
  if((${BOARD} STREQUAL "dtqsys_ht"))
    set (BOARD_ROOT "${CMAKE_SOURCE_DIR}/zephyr")
  endif()
#ESP32C3 Boards
elseif(${BOARD} MATCHES "esp32c3")
  message(STATUS "ESP Board Defined")
  set (CONF_FILE zephyr/esp32c3_prj.conf)
  message(STATUS "Board is a ESP32C3 based board")
else()
  message(FATAL_ERROR "ERROR: Board is not supported")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(HeadTracker DESCRIPTION "HeadTracker")

# -DOPENOCD=/openocd-esp32/bin -DOPENOCD_DEFAULT_PATH=/openocd-esp32/share/openocd/scripts
if(${BOARD} MATCHES "esp32c3")
target_compile_definitions(app PUBLIC "OPENOCD=\"c:\\openocd-esp32\\bin\"")
target_compile_definitions(app PUBLIC "OPENOCD_DEFAULT_PATH=\"c:\\openocd-esp32\\share\\openocd\\scripts\"")
endif()

target_compile_definitions(app PUBLIC FW_GIT_REV=${GIT_REV})
target_compile_definitions(app PUBLIC FW_VER_TAG=${GIT_TAG})
target_include_directories(app PRIVATE src/include)
target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE third-party/ArduinoJson/src)

FILE(GLOB_RECURSE app_sources src/*.c*)
target_sources(app PRIVATE ${app_sources})