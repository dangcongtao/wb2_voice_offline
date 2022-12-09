# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/components/middle $(PROJECT_PATH)/components/middle/include $(PROJECT_PATH)/components/middle/database $(PROJECT_PATH)/components/middle/gpio $(PROJECT_PATH)/components/middle/mqtt $(PROJECT_PATH)/components/middle/wifi
COMPONENT_LDFLAGS +=  -L$(BUILD_DIR_BASE)/middle -lmiddle 
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += middle
component-middle-build: 
