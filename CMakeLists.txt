set(BOARD, nrf52_pca10040)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(wobble)

target_compile_definitions(app PRIVATE 
    NRFX_TIMER_ENABLED=1 
    NRFX_TIMER0_ENABLED=1
    NRFX_TIMER1_ENABLED=1
    __FPU_PRESENT=1
    ARM_MATH_CM4
    )

target_sources(app PRIVATE 
    app/src/main.c
    app/src/imu.c
    app/src/temperature.c
    app/src/ui.c
    app/src/bt.c
    app/src/hog_parser.c
    app/src/messages.c
    drivers/sensor/mpu9250/mpu9250.c
    drivers/oled/display_ssd1306.c
    drivers/drv8825/drv8825.c
    $ENV{ZEPHYR_BASE}/ext/hal/nordic/nrfx/drivers/src/nrfx_timer.c
    )

target_include_directories(app PUBLIC
    drivers/oled
    drivers/sensor/mpu9250
    drivers/drv8825
)

# Link against the hardware floating point math library
#target_link_libraries(app $ENV{GCCARMEMB_TOOLCHAIN_PATH}/arm-none-eabi/lib/hard/libm.a)
find_library(ARM_MATH_LIB 
    NAMES arm_cortexM4lf_math
    PATHS lib)

#target_link_libraries(app ${ARM_MATH_LIB})
target_link_libraries(app -mthumb)
target_link_libraries(app -mcpu=cortex-m4)
target_link_libraries(app -mfloat-abi=hard)
target_link_libraries(app -mfpu=fpv4-sp-d16)
target_link_libraries(app -specs=nosys.specs)
target_link_libraries(app m c g gcc nosys)
