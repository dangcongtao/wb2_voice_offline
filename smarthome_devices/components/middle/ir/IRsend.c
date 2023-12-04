#include "IRsend.h"
#include <stdio.h>
#include "hardware.h"

void IR_logCommon(state_ir_t ir_state)
{
    char result[256];
    memset(result, 0, sizeof(result));
    int seek = 0;

    seek += sprintf(&result[seek], "power: ");
    if (ir_state.power == 1) {
        seek += sprintf(&result[seek], "ON");
    } else {
        seek += sprintf(&result[seek], "OFF");
    }

    seek += sprintf(&result[seek], ", mode: ");
    switch (ir_state.mode)
    {
    case opmode_kCool:
        seek += sprintf(&result[seek], "COOL");
        break;
    
    case opmode_kDry:
        seek += sprintf(&result[seek], "DRY");
        break;
    
    case opmode_kFan:
        seek += sprintf(&result[seek], "FAN");
        break;
    
    case opmode_kHeat:
        seek += sprintf(&result[seek], "HEAT");
        break;
    
    case opmode_kOff:
        seek += sprintf(&result[seek], "OFF");
        break;
    
    default:
        seek += sprintf(&result[seek], "AUTO");
        break;
    }

    seek += sprintf(&result[seek], ", temp: %.01f, fan_speed: ", ir_state.degrees);

    switch (ir_state.fanspeed)
    {
    case fanspeed_kHigh:
        seek += sprintf(&result[seek], "HIGH");
        break;
    
    case fanspeed_kLow:
        seek += sprintf(&result[seek], "LOW");
        break;
    
    case fanspeed_kMax:
        seek += sprintf(&result[seek], "MAX");
        break;
    
    case fanspeed_kMedium:
        seek += sprintf(&result[seek], "MEDIUM");
        break;
    
    case fanspeed_kMediumHigh:
        seek += sprintf(&result[seek], "MEDIUM HIGH");
        break;
    
    case fanspeed_kMin:
        seek += sprintf(&result[seek], "MIN");
        break;
    
    default:
        seek += sprintf(&result[seek], "AUTO");
        break;
    }

    seek += sprintf(&result[seek], "\r\n");

    os_trace(result);
}