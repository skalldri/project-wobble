########################## project-wobble ##########################
This is a program designed to incorporate several hardware components to produce a self balancing robot.
Authors: Stuart Alldrit <skalldrit>
         Kara Vaillancourt <kvaillancourt>

#################### HARDWARE ####################
Nordic nrf52
MPU-9250

#################### DEPENDANCIES ####################
Chocolatey (Windows Only) - skip this section and run env.ps1 instead
Homebrew (MacOS only) - used to install software packages
git - code history
zephyr (http://docs.zephyrproject.org/) - Operating System used for this project
ninja - dependancy for zephyr
python - dependancy for ninja

ZEPHYR - install all dependancies. Follow this guide:
http://docs.zephyrproject.org/getting_started/getting_started.html

#################### SETUP ####################
Set Enviorment Variables: 
You must do this EVERY new terminal session. The command is:
    export <VAR_NAME> = <path/to/variable>

EPHYR_BASE=/Users/kvaillancourt/CLionProjects/project-wobble/zephyr
ZEPHYR_TOOLCHAIN_VARIANT=gccarmemb
GCCARMEMB_TOOLCHAIN_PATH=/Users/kvaillancourt/gcc-arm-none-eabi-7-2018-q2-update

Run Project:
Cmake -GNinja -DBOARD=nrf52_pca10040 ../app/ 

Detect board for Debugging: 
JLinkGDBServer -if SWD -device NRF52832_XXAA

Debug:
Open telnet
telnet> open localhost 19021