# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS += ./

## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS :=

## This component's src
COMPONENT_SRCS := process_data.c \
                  app_cwsl.c \
                  app_scene.c \
                  app_volume.c \
                  app_voice.c \
                  app_button.c \
                  app_fan.c

COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS := $(COMPONENT_ADD_INCLUDEDIRS)
					 


