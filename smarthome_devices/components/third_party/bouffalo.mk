# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS += aes \
							 cJson \
							 huytv_button \

## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS :=

## This component's src
COMPONENT_SRCS := aes/aes_cbc128.c \
                  aes/aes.c \
                  aes/base64.c \
				  cJson/cJSON.c \
				  huytv_button/app_btn.c \
				  
COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS :=  	aes \
						cJson \
						huytv_button \
					 


