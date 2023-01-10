
include $(BL60X_SDK_PATH)/components/network/ble/ble_common.mk

# ifeq ($(CONFIG_ENABLE_PSM_RAM),1)
# CPPFLAGS += -DCONF_USER_ENABLE_PSRAM
# endif

# ifeq ($(CONFIG_ENABLE_CAMERA),1)
# CPPFLAGS += -DCONF_USER_ENABLE_CAMERA
# endif

# ifeq ($(CONFIG_ENABLE_BLSYNC),1)
# CPPFLAGS += -DCONF_USER_ENABLE_BLSYNC
# endif

# ifeq ($(CONFIG_ENABLE_VFS_SPI),1)
# CPPFLAGS += -DCONF_USER_ENABLE_VFS_SPI
# endif

# ifeq ($(CONFIG_ENABLE_VFS_ROMFS),1)
# CPPFLAGS += -DCONF_USER_ENABLE_VFS_ROMFS
# endif


# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS += 	database \
								gpio\
								mqtt\
								wifi \
								ota \
								ble_slave \

## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS :=

## This component's src
COMPONENT_SRCS := database/m_database.c \
				  database/flash_api.c \
				  database/easyflash_common.c \
                  gpio/m_button.c \
                  gpio/m_led_stt.c \
				  gpio/m_switch_event.c \
				  gpio/m_switch.c \
				  mqtt/m_mqtt_client_props.c \
				  mqtt/m_mqtt_client_method.c \
				  ota/m_ota.c \
				  wifi/tcp_server.c \
				  wifi/m_http_server.c \
				  wifi/m_http_services.c \
				  wifi/m_ap_config.c \
				  wifi/m_smartconfig.c \
				  wifi/m_wifi_ap.c \
				  wifi/m_wifi_station.c \
				  wifi/m_wifi_event.c \
				  wifi/m_wifi.c \
				  ble_slave/m_ble.c \
				  ble_slave/my_ble.c \
				  
COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS := 	database \
						gpio \
						mqtt \
						wifi \
						ota \
						ble_slave \
					 


