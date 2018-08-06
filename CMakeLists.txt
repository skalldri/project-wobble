
set(BOARD, nrf52_pca10040)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(wobble)
target_sources(app PRIVATE 
    app/src/main.c
    app/src/imu.c
    app/src/temperature.c)