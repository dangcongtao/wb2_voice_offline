#!/bin/sh
#export 
export BL60X_SDK_PATH=~/wb2/Ai-Thinker-WB2/
export CONFIG_CHIP_NAME=BL602

# build and flash
echo "************************************************************"
echo "Author: taodc, set path done, able to build and flash"
echo "************************************************************"
echo ""

# Generate a timestamped log file name
FILE_NAME="sdr_$(date +%Y-%m-%d_%H:%M:%S).log"

# Run make, flash, and picocom commands
sudo chmod 777 /dev/ttyUSB$1 && make -j16 && \
make flash p=/dev/ttyUSB$1 b=921600 && \
picocom /dev/ttyUSB$1 -b 115200 --logfile=$FILE_NAME

# sudo chmod 777 /dev/ttyUSB$1 && picocom /dev/ttyUSB$1 -b 115200 --logfile $FILE_NAME
# sudo chmod 777 /dev/ttyUSB$1 && make -j16 