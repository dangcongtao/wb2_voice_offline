
setup environment to work with Ai-WB02 chip:
-------------------------------------------------------------------
sudo apt install build-essential python3 python3-pip git screen
cd ~/ && git clone --recursive https://github.com/Ai-Thinker-Open/Ai-Thinker-WB2.git
export BL60X_SDK_PATH=~/Ai-Thinker-WB2/
export CONFIG_CHIP_NAME=BL602

cd ~/Ai-Thinker-WB2
git submodule init
git submodule update

# Firstly, to support OTA feature for 2M flash chip version, have to change content of partition table file by run following commands:
rm ~/Ai-Thinker-WB2/tools/flash_tool/chips/bl602/partition/partition_cfg_2M.toml
cp ~/Ai-Thinker-WB2/applications/peripherals/smarthome_devices/Flash_Partition_table/03-partition_cfg_2M-OTA.toml ~/Ai-Thinker-WB2/tools/flash_tool/chips/bl602/partition/partition_cfg_2M.toml

# command build and flash firmware
docker run -it --name "hunonic-wb2" -h dungnt98 --rm -v /home/dungnt98/:/home/dungnt98/ --device=/dev/ttyUSB0 ntdungembedded98/hunonic-esp8266
sudo chmod 777 /dev/ttyUSB0 && make -j16 && make flash b=921600 && picocom /dev/ttyUSB0 -b 115200 --logfile=log-$(date +%s).txt
# to exit screen serial log: press sequence keys as: Ctrl+A -> k -> y

IR:
trước khi gửi lệnh phải có hàm: 
vTaskEnterCritical();
sau khi gửi xong thì dùng hàm:
vTaskExitCritical();
để có thể gửi thành công

nếu khi gửi mà nhận đc data ở IRrecv thì cần sử dụng 2 hàm trên khi gửi

Casper là biến thể của WhirlpoolAC