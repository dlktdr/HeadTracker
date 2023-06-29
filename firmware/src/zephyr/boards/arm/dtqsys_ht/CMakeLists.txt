if(CONFIG_BOARD_DTQSYS_INIT)
  zephyr_library()
  zephyr_library_sources("${CMAKE_CURRENT_SOURCE_DIR}/src/init.c")
endif()
