# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS += ./

## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS :=

## This component's src
COMPONENT_SRCS := hardware.c \
                  button.c \
                  led.c \
                  74hc595.c \
                  temp_10k.c \
                  hw_uart.c \
                  aiio_uart0_rx_log.c \
                  buzz.c \
                  lpf.c \
				  
COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS :=  	./
					 


