# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS += 	database \
								gpio\
								mqtt\
								wifi \
								ota

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
				  mqtt/m_mqtt.c \
				  wifi/m_wifi.c \
				  wifi/m_ap_config.c \
				  wifi/m_soft_ap.c \
				  wifi/m_http_server.c \
				  wifi/m_smartconfig.c \
				  ota/m_ota.c \

				  
COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS := 	database \
						gpio \
						mqtt \
						wifi \
						ota \
					 


