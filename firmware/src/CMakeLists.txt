cmake_minimum_required(VERSION 3.20.0)

include(FetchContent)
FetchContent_Declare(ArduinoJson
    GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
    GIT_TAG        v6.21.3
)
FetchContent_MakeAvailable(ArduinoJson)

set(BOARD arduino_nano_33_ble)
set(CONF_FILE zephyr/prj.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(HeadTracker)

target_compile_definitions(app PUBLIC PCB_NANO33BLE) # TODO: Use the devicetree board file to configure this
target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE src/include)
target_sources(app PRIVATE src/main.cpp)
target_sources(app PRIVATE src/nano33ble.cpp)
target_link_libraries(app ArduinoJson)
