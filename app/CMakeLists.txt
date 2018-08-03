
set(BOARD, nrf52_pca10040)
set(CONF_FILE, )
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(wobble)
target_sources(app PRIVATE 
    src/main.c
    src/imu.c
    src/temperature.c)