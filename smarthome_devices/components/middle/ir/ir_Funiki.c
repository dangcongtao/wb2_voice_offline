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

    // os_trace("decodeFunikiAC() offset: %d, nbits: %d, kFunikiBits: %d\r\n", offset, nbits, kFunikiBits);

    if (results->rawlen == 200 && nbits != kFunikiAcBits) {
        return false;
    }

    if (results->rawlen == 300 && nbits != kFunikiAcExtendedBits) {
        return false;
    }

    if (results->rawlen != 200 && results->rawlen != 300) {
        // os_trace("funiki len invalid\r\n");
        return false;
    }

    for (uint16_t pos = 0; pos <= (nbits / 8) - kFunikiAcSectionLength;
       pos += kFunikiAcSectionLength) {
        uint16_t used;
        // Section Header + Section Data (7 bytes) + Section Footer
        used = matchGenericUint8(results->rawbuf + offset, (uint8_t*)(&results->state[pos]),
                            results->rawlen - offset, kFunikiAcSectionLength * 8,
                            4400, 4400,
                            480, 1500,
                            480, 460,
                            500, 5300,
                            pos + kFunikiAcSectionLength >= nbits / 8,
                            50, 0, false);

        if (used == 0){
            os_trace("%s step 2 not used failed\r\n", __func__);
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

void sendFunikiAc(const uint8_t *data, uint16_t nbytes, uint16_t repeat)
{
    vTaskEnterCritical();
    uint16_t repeatHeaderMark = 0;
    uint8_t duty = kDutyDefault;

    enableIROut(38, 50);

    for (uint16_t offset = 0; offset < nbytes; offset += kFunikiAcSectionLength){
        sendGenericBytes(4400, 4400, 480, 1500, 480, 460, 500, 5300, data + offset, kFunikiAcSectionLength, 38, false, repeat, 50);
    }

    // mark(9000);
    // space(4500);
    
    // for (uint16_t offset = 0; offset < nbytes;) {

    //     sendData(502, 1650, 501, 600, *(data + offset), 8, false);

    //     offset++;
    //     if (offset == 6) {
    //         mark(480);
    //         space(8000);
    //     } else if (offset == 14) {
    //         mark(480);
    //         space(8000);
    //     }
    // }

    // mark(500);
    vTaskExitCritical();
}

static uint8_t get_temp_funiki(uint8_t *states)
{
    os_trace("temp_huy: %u\r\n", ((~(states[11])) & 0xF));
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

static opmode_t get_mode_funiki(uint8_t *states)
{
    // memset(str, 0, 20);
    switch ((states[11] >> 4) & 0xF)
    {
    case 0xF:
        // sprintf(str, "COOL");
        return opmode_kCool;
        break;

    case 0xE:
        // sprintf(str, "AUTO");
        return opmode_kAuto;
        break;
    
    case 0xC:
        // sprintf(str, "HEAT");
        return opmode_kHeat;
        break;
    
    case 0xD:
        if ((states[11] & 0xF) == 0x8) {
            // sprintf(str, "FAN");
            return opmode_kFan;
        } else {
            // sprintf(str, "DRY");
            return opmode_kDry;
        }
        break;
    
    default:
        return opmode_kAuto;
        break;
    }
}

static fanspeed_t get_fan_funiki(uint8_t *states)
{
    // memset(str, 0, 20);
    switch (states[9] & 0xF)
    {
    case 0x3:
        // sprintf(str, "HIGH");
        return fanspeed_kMax;
        break;

    case 0x6:
        // sprintf(str, "LOW");
        return fanspeed_kMin;
        break;
    
    case 0x5:
        // sprintf(str, "MEDIUM");
        return fanspeed_kMedium;
        break;
    
    case 0x2:
    //     sprintf(str, "AUTO");
    //     break;

    // case 0x7:

    default:
        return fanspeed_kAuto;
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

// static FunikiProtocol _;

static void Funiki_toCommon(const uint8_t* new_code, state_ir_t *ir_state)
{
    ir_state->mode = get_mode_funiki(new_code);
    ir_state->fanspeed = get_fan_funiki(new_code);
    ir_state->power = is_power_on(new_code);
    ir_state->degrees = get_temp_funiki(new_code);
}

void Funiki_setRaw(const uint8_t* new_code, const decode_type_t protocol, state_ir_t *ir_state) 
{
    // memcpy(_.data.raw, new_code, kFunikiAcStateLength);
    Funiki_toCommon(new_code, ir_state);
    FunikiProtocol _;
    memset(&_, 0, sizeof(FunikiProtocol));
    memcpy(&_.data.raw, new_code, 12);
    os_trace("fan: %u\r\n", _.data.fan);
    // _.data.raw = new_code;
}

static void hash_code(FunikiProtocol* data, state_ir_t state_ir)
{
    if (state_ir.power) {
        data->data.power = 0;
        switch ((uint8_t)state_ir.degrees)
        {
        case 17:
            data->data.temp = 0xF;
            break;
        
        case 18:
            data->data.temp = 0x7;
            break;
        
        case 19:
            data->data.temp = 0x3;
            break;
        
        case 20:
            data->data.temp = 0xb;
            break;
        
        case 21:
            data->data.temp = 0x9;
            break;
        
        case 22:
            data->data.temp = 0x1;
            break;
        
        case 23:
            data->data.temp = 0x5;
            break;
        
        case 24:
            data->data.temp = 0xD;
            break;
        
        case 25:
            data->data.temp = 0xC;
            break;
        
        case 26:
            data->data.temp = 0x4;
            break;
        
        case 27:
            data->data.temp = 0x6;
            break;
        
        case 28:
            data->data.temp = 0xE;
            break;
        
        case 29:
            data->data.temp = 0xA;
            break;
        
        case 30:
            data->data.temp = 0x2;
            break;
        
        default:
            break;
        }

        switch (state_ir.mode)
        {
        case opmode_kCool:
            data->data.mode = 0xF;
            break;

        case opmode_kAuto:
            data->data.mode = 0xE;
            break;
        
        case opmode_kHeat:
            data->data.mode = 0xC;
            break;
        
        case opmode_kFan:
            data->data.temp = 0x8;
        case opmode_kDry:
            data->data.mode = 0xD;
            break;
        
        default:
            break;
        }

        switch (state_ir.fanspeed)
        {
        case fanspeed_kMax:
            data->data.fan = 0x3;
            break;

        case fanspeed_kMin:
            data->data.fan = 0x6;
            break;
        
        case fanspeed_kMedium:
            data->data.fan = 0x5;
            break;
        
        case fanspeed_kAuto:
            data->data.fan = 0x2;
            break;

        default:
            break;
        }

        if (state_ir.mode == opmode_kAuto || state_ir.mode == opmode_kDry) {
            data->data.fan = 0x7;
        }
    } else {
        data->data.power = 1;
        data->data.temp = 0x8;
        data->data.fan = 0x1;
        data->data.mode = 0xF;
    }

    

    data->data.raw[0] = 0x4d;
    data->data.raw[1] = 0xb2;


    data->data.raw[2] = (~data->data.raw[3] & 0xFF);
    data->data.raw[4] = (~data->data.raw[5] & 0xFF);

    memcpy(&data->data.raw[6], &data->data.raw[0], 6);
}

void Funiki_send(state_ir_t state_ir)
{

    switch ((uint8_t)state_ir.mode)
    {
    case 1:
        state_ir.mode = opmode_kDry;
        break;
    
    case 0:
        state_ir.mode = opmode_kCool;
        break;
    
    case 2:
        state_ir.mode = opmode_kAuto;
        break;
    
    case 3:
        state_ir.mode = opmode_kHeat;
        break;
    
    case 4:
        state_ir.mode = opmode_kFan;
        break;
    
    default:
        break;
    }

    switch (state_ir.fanspeed)
    {
    case 5:
        state_ir.fanspeed = fanspeed_kAuto;
        break;
    
    case 4:
        state_ir.fanspeed = fanspeed_kMin;
        break;
    
    case 2:
        state_ir.fanspeed = fanspeed_kMedium;
        break;
    
    case 1:
        state_ir.fanspeed = fanspeed_kMax;
        break;
    
    default:
        break;
    }

    FunikiProtocol _;
    hash_code(&_, state_ir);

    // os_trace("FUNIKI_SEND: 0x");
    // for (uint8_t i = 0; i < 18; i++) {
    //     if (i == 6) {
    //         os_trace("\t|\t");
    //     } else if (i == 12) {
    //             break;
    //     }
    //     os_trace("%02x", _.data.raw[i]);
    // }
    // os_trace("\r\n");

    sendFunikiAc(_.data.raw, kFunikiAcStateLength, 0);
}

// void Funiki_getString(void)
// {
//     // char mode_str[20], fan_str[20];
//     // get_mode_funiki(_.data.raw);
//     // get_fan_funiki(_.data.raw);
//     // os_trace("power: %u, mode: %s, temp: %u, fan: %s\r\n", is_power_on(_.data.raw), mode_str, get_temp_funiki(_.data.raw), fan_str);
// }