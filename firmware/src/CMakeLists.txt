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

set (BOARD_ROOT "${CMAKE_SOURCE_DIR}/zephyr") # Use the local board files first
set (CONF_FILE zephyr/prj.conf) # Set the project configuration file
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(HeadTracker DESCRIPTION "HeadTracker")

target_compile_definitions(app PUBLIC FW_GIT_REV=${GIT_REV})
target_compile_definitions(app PUBLIC FW_VER_TAG=${GIT_TAG})
target_include_directories(app PRIVATE src/include)
target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE third-party/ArduinoJson/src)

FILE(GLOB_RECURSE app_sources src/*.c*)
target_sources(app PRIVATE ${app_sources})
