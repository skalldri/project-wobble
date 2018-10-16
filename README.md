# project-wobble
This is a program designed to incorporate several hardware components to produce a self balancing robot.
Authors: Stuart Alldrit <skalldri>
         Kara Vaillancourt <kvaillancourt>

## HARDWARE
Nordic nrf52 - Microcontroller, bluetooth radio 

    http://infocenter.nordicsemi.com/pdf/nRF52_DK_User_Guide_v1.2.pdf
    http://infocenter.nordicsemi.com/pdf/nRF52832_PS_v1.1.pdf
    
MPU-9250 - Inertial Measurement Unit

    https://www.invensense.com/wp-content/uploads/2015/02/PS-MPU-9250A-01-v1.1.pdf
    
DRV8825 - Stepper Motor Driver

    http://www.ti.com/lit/ds/symlink/drv8825.pdf
    
DIYmall OLED - screen, based on SSD1306 Driver

    https://www.olimex.com/Products/Modules/LCD/MOD-OLED-128x64/resources/SSD1306.pdf

## DEPENDANCIES

Clone git submodules with `git submodule update --init`

### Windows
Install Chocolatey and then run `env.ps1 -init` in an admin prompt to install dependencies.

### MacOS
- Homebrew - used to install software packages
- git - code history
- ninja - dependancy for zephyr
- python - dependancy for ninja
- ZephyrOS - install all dependancies. Follow this guide: http://docs.zephyrproject.org/getting_started/getting_started.html

Ninja menu config:

go into build folder:

    > ninja menuconfig
    Device Drivers > Console Drivers > Use RTT Console * (output all the debug statements)
    Device Drivers > SPI Hardware Bus Drivers > SPI port 2 * (port to talk to the mpu9250)
    Device Drivers > SPI Hardware Bus Drivers > Port 2 interrupt priority (2)
    Device Drivers > SPI Hardware Bus Drivers > nRF SPI nrfx drivers > SPI Port 2 Driver type > SCK pin number (15)
    Device Drivers > SPI Hardware Bus Drivers > nRF SPI nrfx drivers > SPI Port 2 Driver type > MOSI pin number (14)
    Device Drivers > SPI Hardware Bus Drivers > nRF SPI nrfx drivers > SPI Port 2 Driver type > MISO pin number (13) 
    Device Drivers > I2C Drivers > Enable I2C Port 0
    Device Drivers > I2C Drivers > Enable I2C Port 1
    Device Drivers > I2C Drivers > nrf TWI nrfx drivers > I2C Port 0 Driver type (nerf TWI 0) > nrf TWI 0 (x)
    Device Drivers > I2C Drivers > nrf TWI nrfx drivers > I2C Port 1 Driver type (nerf TWI 1) > nrf TWI 1 (x)
    Device Drivers > Sensor Drivers > nrf5 Temperature Sensor > (TEMP 0) Driver Name
    Device Drivers > Sensor Drivers > nrf5 Temperature Sensor > (1) TEMP interrupt priority

> o (select 'prj.conf' file inside project wobble folder) (loads all files in the file)

## SETUP
Set Enviorment Variables: 
You must do this EVERY new terminal session. The command is:
    export <VAR_NAME> = <path/to/variable>

*/Users/kvaillancourt

ZEPHYR_BASE=<project/folder>/project-wobble/zephyr
ZEPHYR_TOOLCHAIN_VARIANT=gccarmemb
GCCARMEMB_TOOLCHAIN_PATH=<gcc/folder>/gcc-arm-none-eabi-7-2018-q2-update

Configure Project:
> Cmake -GNinja -DBOARD=nrf52_pca10040 ../ 

Run Project:
> ninja

## DEBUG

OS Print:
    Open telnet
    telnet> open localhost 19021

Download & Run Firmware:
    Open VSCode
    Debug symbol
    Play symbol
    (Debug breaks will ensue)

Alternate debug method via terminal: 
> JLinkGDBServer -if SWD -device NRF52832_XXAA
