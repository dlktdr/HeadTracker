# Copyright (c) 2020 Jefferson Lee.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_ARDUINO_NANO_33_BLE_INIT)
  zephyr_library()
  zephyr_library_sources("${CMAKE_CURRENT_SOURCE_DIR}/src/init.c")
endif()
