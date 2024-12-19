#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := switchv3
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
#CONFIG_TOOLPREFIX :=

-include ./proj_config.mk

# BL60X_SDK_PATH = /home/huy/Ai-Thinker-WB2
ifeq ($(origin BL60X_SDK_PATH), undefined)
BL60X_SDK_PATH_GUESS ?= $(shell pwd)
BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../../..
$(info ****** Please SET BL60X_SDK_PATH ******)
$(info ****** Trying SDK PATH [$(BL60X_SDK_PATH)])
endif

COMPONENTS_NETWORK := sntp dns_server
COMPONENTS_BLSYS   := bltime blfdt blmtd blota bloop loopadc looprt loopset
COMPONENTS_VFS     := romfs

INCLUDE_COMPONENTS += easyflash4
INCLUDE_COMPONENTS += freertos_riscv_ram
INCLUDE_COMPONENTS += bl602 bl602_std
INCLUDE_COMPONENTS += cli vfs yloop utils blog blog_testc newlibc 
#=============== DUNGNT98 ===============
# HTTP
INCLUDE_COMPONENTS += httpc mbedtls_lts lwip lwip_dhcpd axk_ota http-parser
# WIFI
INCLUDE_COMPONENTS += wifi wifi_manager wpa_supplicant wifi_hosal hosal bl602_os_adapter blcrypto_suite
INCLUDE_COMPONENTS +=  aws-iot       
INCLUDE_COMPONENTS +=  coredump mqtt mqtt_client
# MQTT
INCLUDE_COMPONENTS    += axk_common axk_log tcp_transport axk_tls axk_mqtt http-parser


# BLE
INCLUDE_COMPONENTS += freertos_riscv_ram bl602 ble602_hal bl602_std newlibc wifi wifi_manager wpa_supplicant bl602_os_adapter wifi_hosal hosal mbedtls_lts lwip lwip_dhcpd vfs yloop utils cli netutils blog blog_testc blcrypto_suite
INCLUDE_COMPONENTS += easyflash4 coredump
INCLUDE_COMPONENTS += rfparam_adapter_tmp
INCLUDE_COMPONENTS += bl_os_adapter cjson 
INCLUDE_COMPONENTS += blecontroller_602_std  blestack blecontroller

#=======================================
INCLUDE_COMPONENTS += dns_server netutils
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(PROJECT_NAME)

INCLUDE_COMPONENTS += app_config
INCLUDE_COMPONENTS += application
INCLUDE_COMPONENTS += hardware
INCLUDE_COMPONENTS += middle
INCLUDE_COMPONENTS += third_party

COMPONENT_ADD_INCLUDEDIRS += app_config
COMPONENT_ADD_INCLUDEDIRS += application
COMPONENT_ADD_INCLUDEDIRS += hardware
COMPONENT_ADD_INCLUDEDIRS += middle/mqtt
COMPONENT_ADD_INCLUDEDIRS += middle/gpio
COMPONENT_ADD_INCLUDEDIRS += middle/wifi
COMPONENT_ADD_INCLUDEDIRS += middle/ota
COMPONENT_ADD_INCLUDEDIRS += middle/ble_slave
COMPONENT_ADD_INCLUDEDIRS += middle/database
COMPONENT_ADD_INCLUDEDIRS += middle/ir
COMPONENT_ADD_INCLUDEDIRS += third_party/huytv_button
COMPONENT_ADD_INCLUDEDIRS += third_party/aes
COMPONENT_ADD_INCLUDEDIRS += third_party/cJson

LOG_ENABLED_COMPONENTS:=blog_testc hosal loopset looprt bloop

include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
