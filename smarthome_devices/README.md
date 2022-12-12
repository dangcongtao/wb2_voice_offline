
setup environment to work with Ai-WB02 chip:
-------------------------------------------------------------------
sudo apt install build-essential python3 python3-pip git screen
cd ~/ && git clone --recursive https://github.com/Ai-Thinker-Open/Ai-Thinker-WB2.git
export BL60X_SDK_PATH=~/Ai-Thinker-WB2/
export CONFIG_CHIP_NAME=BL602


