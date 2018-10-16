
set(BOARD, nrf52_pca10040)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(wobble)
target_sources(app PRIVATE 
    app/src/main.c
    app/src/imu.c
    app/src/temperature.c
    app/src/ui.c
    app/src/bt.c
    drivers/sensor/mpu9250/mpu9250.c
    drivers/oled/display_ssd1306.c
    )

target_include_directories(app PUBLIC
    drivers/oled
    drivers/sensor/mpu9250
)