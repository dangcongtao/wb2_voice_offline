
setup environment to work with Ai-WB02 chip:
-------------------------------------------------------------------
sudo apt install build-essential python3 python3-pip git screen
cd ~/ && git clone --recursive https://github.com/Ai-Thinker-Open/Ai-Thinker-WB2.git
export BL60X_SDK_PATH=~/Ai-Thinker-WB2/
export CONFIG_CHIP_NAME=BL602


# command build and flash firmware
docker run -it --name "hunonic-wb2" -h dungnt98 --rm -v /home/dungnt98/:/home/dungnt98/ --device=/dev/ttyUSB0 ntdungembedded98/hunonic-esp8266
sudo chmod 777 /dev/ttyUSB0 && make -j16 && make flash b=921600 && sudo screen /dev/ttyUSB0 115200
# to exit screen serial log: press sequence keys as: Ctrl+A -> k -> y