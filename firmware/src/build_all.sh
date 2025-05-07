mkdir build_bins
rm build_bins/*
west build -p -b arduino_nano_33_ble &&
cp ./build/zephyr/*.bin build_bins/ &&
west build -p -b arduino_nano_33_ble -- -DBOARD_REV2=y &&
cp ./build/zephyr/*.bin build_bins/ &&
west build -p -b xiao_ble/nrf52840 &&
cp ./build/zephyr/*.uf2 build_bins/ &&
west build -p -b xiao_ble/nrf52840/sense &&
cp ./build/zephyr/*.uf2 build_bins/ &&
west build -p -b esp32c3_devkitm &&
cp ./build/zephyr/*.bin build_bins/ &&
west build -p -b m5stickc_plus/esp32/procpu &&
cp ./build/zephyr/*.bin build_bins/ &&
west build -p -b rpi_pico/rp2040/w &&
cp ./build/zephyr/*.uf2 build_bins/ &&
west build -p -b dtqsys_ht &&
cp ./build/zephyr/*.bin build_bins/ &&
west build -p -b licardo_ht &&
cp ./build/zephyr/*.bin build_bins/