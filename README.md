########################## project-wobble ##########################
This is a program designed to incorporate several hardware components to produce a self balancing robot.
Authors: Stuart Alldrit <skalldri>
         Kara Vaillancourt <kvaillancourt>

#################### HARDWARE ####################
Nordic nrf52 - Microcontroller, bluetooth radio 
    http://infocenter.nordicsemi.com/pdf/nRF52_DK_User_Guide_v1.2.pdf
    http://infocenter.nordicsemi.com/pdf/nRF52832_PS_v1.1.pdf
MPU-9250 - Inertial Measurement Unit
    https://www.invensense.com/wp-content/uploads/2015/02/PS-MPU-9250A-01-v1.1.pdf
DRV8825 - Stepper Motor Driver
    http://www.ti.com/lit/ds/symlink/drv8825.pdf
DIYmall OLED - screen

#################### DEPENDANCIES ####################
Chocolatey (Windows Only) - skip this section and run env.ps1 instead
Homebrew (MacOS only) - used to install software packages
git - code history
zephyr (http://docs.zephyrproject.org/) - Operating System used for this project
ninja - dependancy for zephyr
python - dependancy for ninja

ZEPHYR - install all dependancies. Follow this guide:
http://docs.zephyrproject.org/getting_started/getting_started.html

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

#################### SETUP ####################
Set Enviorment Variables: 
You must do this EVERY new terminal session. The command is:
    export <VAR_NAME> = <path/to/variable>

EPHYR_BASE=<project/folder>/project-wobble/zephyr
ZEPHYR_TOOLCHAIN_VARIANT=gccarmemb
GCCARMEMB_TOOLCHAIN_PATH=<gcc/folder>/gcc-arm-none-eabi-7-2018-q2-update

Run Project:
Cmake -GNinja -DBOARD=nrf52_pca10040 ../app/ 

Detect board for Debugging: 
JLinkGDBServer -if SWD -device NRF52832_XXAA

#################### DEBUG ####################

OS Print:
    Open telnet
    telnet> open localhost 19021

Download & Run Firmware:
    Open VSCode
    Debug symbol
    Play symbol
    (Debug breaks will ensue)
