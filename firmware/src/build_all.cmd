mkdir build_bins
del /q build_bins\*.*
west build -p -b arduino_nano_33_ble || exit /b
copy .\build\zephyr\*.bin build_bins\

west build -p -b arduino_nano_33_ble -- -DBOARD_REV_SR2=y || exit /b
copy .\build\zephyr\*.bin build_bins\

west build -p -b xiao_ble/nrf52840/sense || exit /b
copy .\build\zephyr\xiao*.uf2 build_bins\

west build -p -b esp32c3_devkitm || exit /b
copy .\build\zephyr\*.bin build_bins\

west build -p -b m5stickc_plus/esp32/procpu || exit /b
copy .\build\zephyr\*.bin build_bins\

west build -p -b rpi_pico/rp2040/w || exit /b
copy .\build\zephyr\rpi*.uf2 build_bins\

west build -p -b dtqsys_ht || exit /b
copy .\build\zephyr\*.bin build_bins\