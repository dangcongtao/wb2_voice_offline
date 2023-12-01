#include "ir_Funiki.h"
#include "system_typedef.h"


const uint16_t kFunikiAcBitMark = 4400;
const uint16_t kFunikiAcHdrSpace = 4400;
const uint16_t kFunikiHdr1Mark = 1100;             ///< uSeconds.
const uint16_t kFunikiHdr1Space = 110;            ///< uSeconds.
const uint16_t kFunikiHdr2Mark = 300;            ///< uSeconds.

const uint16_t kSamsungAcSectionMark = 600;

bool decodeFunikiAC(decode_results *results, uint16_t offset, const uint16_t nbits, const bool strict) 
{

    os_trace("decodeFunikiAC() offset: %d, nbits: %d, kFunikiBits: %d\r\n", offset, nbits, kFunikiBits);

    if (results->rawlen == 200 && nbits != kFunikiAcBits) {
        return false;
    }

    if (results->rawlen == 300 && nbits != kFunikiAcExtendedBits) {
        return false;
    }

    if (results->rawlen != 200 && results->rawlen != 300) {
        os_trace("funiki len invalid\r\n");
        return false;
    }

    for (uint16_t pos = 0; pos <= (nbits / 8) - kFunikiAcSectionLength;
       pos += kFunikiAcSectionLength) {
        uint16_t used;
        // Section Header + Section Data (7 bytes) + Section Footer
        used = matchGenericUint8(results->rawbuf + offset, (uint8_t*)(&results->state[pos]),
                            results->rawlen - offset, kFunikiAcSectionLength * 8,
                            5050, 4450,
                            600, 1500,
                            600, 460,
                            600, 4300,
                            pos + kFunikiAcSectionLength >= nbits / 8,
                            50, 0, false);

        if (used == 0){
            os_trace("step 2 not used failed\r\n");
            return false;
        }
        offset += used;

    }

    results->decode_type = FUNIKI;
    results->bits = nbits;

    // don set it to 0, It states will be 0
    // results->command = 0;
    // results->address = 0;


    os_trace("decode Funiki success\r\n");
    return true;
}

static uint8_t get_temp_funiki(uint8_t *states)
{
    uint8_t temp = 0;
    switch (states[11] & 0xF)
    {
    case 0xF:
        temp = 17;
        break;
    
    case 0x7:
        temp = 18;
        break;
    
    case 0x3:
        temp = 19;
        break;
    
    case 0xb:
        temp = 20;
        break;
    
    case 0x9:
        temp = 21;
        break;
    
    case 0x1:
        temp = 22;
        break;
    
    case 0x5:
        temp = 23;
        break;
    
    case 0xd:
        temp = 24;
        break;
    
    case 0xc:
        temp = 25;
        break;
    
    case 0x4:
        temp = 26;
        break;
    
    case 0x6:
        temp = 27;
        break;
    
    case 0xe:
        temp = 28;
        break;
    
    case 0xa:
        temp = 29;
        break;
    
    case 0x2:
        temp = 30;
        break;
    
    default:
        break;
    }
    return temp;
}

static void get_mode_funiki(uint8_t *states, char* str)
{
    memset(str, 0, 20);
    switch ((states[11] >> 4) & 0xF)
    {
    case 0xF:
        sprintf(str, "COOL");
        break;

    case 0xE:
        sprintf(str, "AUTO");
        break;
    
    case 0xC:
        sprintf(str, "HEAT");
        break;
    
    case 0xD:
        if ((states[11] & 0xF) == 0x8) {
            sprintf(str, "FAN");
        } else {
            sprintf(str, "DRY");
        }
        break;
    
    default:
        break;
    }
}

static void get_fan_funiki(uint8_t *states, char* str)
{
    memset(str, 0, 20);
    switch (states[9] & 0xF)
    {
    case 0x3:
        sprintf(str, "HIGH");
        break;

    case 0x6:
        sprintf(str, "LOW");
        break;
    
    case 0x5:
        sprintf(str, "MEDIUM");
        break;
    
    case 0x2:
        sprintf(str, "AUTO");
        break;

    case 0x7:
        sprintf(str, "AUTO WITCH LOCKED");
        break;

    default:
        break;
    }
}

static uint8_t is_power_on(uint8_t* states) 
{
    if ((states[9] >> 4) & 0xF) {
        return 0;
    }
    return 1;
}

static FunikiProtocol _;

void Funiki_setRaw(const uint8_t* new_code, const decode_type_t protocol) 
{
    memcpy(_.data.raw, new_code, kFunikiAcStateLength);
    // _.data.raw = new_code;
}

void Funiki_getString(void)
{
    char mode_str[20], fan_str[20];
    get_mode_funiki(_.data.raw, mode_str);
    get_fan_funiki(_.data.raw, fan_str);
    os_trace("power: %u, mode: %s, temp: %u, fan: %s\r\n", is_power_on(_.data.raw), mode_str, get_temp_funiki(_.data.raw), fan_str);
}