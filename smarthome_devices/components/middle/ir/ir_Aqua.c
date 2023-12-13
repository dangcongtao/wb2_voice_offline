#include "ir_Aqua.h"
#include "system_typedef.h"
#include "string.h"


const uint16_t kAquaHdrMark = 3050;
const uint16_t kAquaSpaceMark = 3040;
const uint16_t kAquaHdrSpace = 4430;

const uint16_t kAquaBitOneMark =   601;
const uint16_t kAquaOneSpace =  1650;
const uint16_t kAquaBitZeroMark =   602;
const uint16_t kAquaZeroSpace = 550;

static AquaProtocol _;

static uint8_t getTemp(const uint8_t *states)
{
    // AquaProtocol _;
    uint8_t res = 0;
    uint8_t num = _.data.reverse_temp;
    for (int i = 0; i < 4; i++) {
        res = (res << 1) | (num & 1);
        num >>= 1;
    }
    // state_ir->celsius = res + 16;
    return res + 16; 
}

static fanspeed_t get_fan_aqua(uint8_t *states)
{
    // memset(str, 0, 20);
    switch (states[5] & 0xF)
    {
    case 0x4:
        return fanspeed_kMax;
        // sprintf(str, "HIGH");
        break;

    case 0x6:
        // sprintf(str, "LOW");
        return fanspeed_kMin;
        break;
    
    case 0x2:
        // sprintf(str, "MEDIUM");
        return fanspeed_kMedium;
        break;
    
    case 0x5:
        // sprintf(str, "AUTO");
        return fanspeed_kAuto;
        break;
    default:
        return fanspeed_kAuto;
        break;
    }
}

static opmode_t get_mode_aqua(uint8_t *states)
{
    switch (states[7] & 0xF)
    {
    case 0x4:
        // sprintf(str, "COOL");
        return opmode_kCool;
        break;

    case 0x2:
        // sprintf(str, "DRY");
        return opmode_kDry;
        break;
    
    default:
        return opmode_kAuto;
        break;
    }
}

static uint8_t is_power_on(uint8_t* states) 
{
    if (states[4] & 0xF) {
        return 1;
    }
    return 0;
}

void AQUA_toCommon(const uint8_t* new_code, state_ir_t* state_ir)
{
    state_ir->degrees = (float)getTemp(new_code);
    state_ir->mode = get_mode_aqua(new_code);
    state_ir->fanspeed = get_fan_aqua(new_code);
    state_ir->power = is_power_on(new_code);
}

void AQUA_setRaw(const uint8_t* new_code, const decode_type_t protocol, state_ir_t* state_ir)
{
    memcpy(_.data.raw, new_code, kAquaAcStateLength);
    AQUA_toCommon(new_code, state_ir);
    os_trace("power: %u, fan: %u, mode: %u, temp: %u\r\n", _.data.power, _.data.fan, _.data.mode, _.data.reverse_temp);
    // _.data.raw = new_code;
}

void aqua_getString(void)
{
    // char mode_str[20], fan_str[20];
    // get_mode_aqua(_.data.raw, mode_str);
    // get_fan_aqua(_.data.raw, fan_str);
    // os_trace("power: %u, mode: %s, temp: %u, fan: %s\r\n", is_power_on(_.data.raw), mode_str, getTemp(), fan_str);
}

bool decodeAquaAC(decode_results *results, uint16_t offset, const uint16_t nbits, const bool strict) 
{

    // os_trace("%s() offset: %d, nbits: %d, kLg32Bits: %d\r\n", __func__, offset, nbits, kFunikiBits);

    if (results->rawlen != 230) {
        // os_trace("aqua len invalid\r\n");
        return false;
    }

    if (!matchMark(results->rawbuf[offset++], kAquaHdrMark, kUseDefTol, kMarkExcess)) {
        // os_trace("%s mark 1 failed\r\n", __func__);
        return false;
    }

    if (!matchSpace(results->rawbuf[offset++], kAquaSpaceMark, kUseDefTol, kMarkExcess)) {
        // os_trace("%s space 2 failed\r\n", __func__);
        return false;
    }

    memset(results->state, 0, 53);

    uint16_t used;
    // Section Header + Section Data (7 bytes) + Section Footer
    used = matchGenericUint8(results->rawbuf + offset, (uint8_t*)&(results->state[0]),
                        results->rawlen - offset, nbits,
                        kAquaHdrMark, kAquaHdrSpace,
                        kAquaBitOneMark, 2200,
                        kAquaBitZeroMark, kAquaZeroSpace,
                        500, 0,
                        true,
                        50, 0, false);

    if (used == 0){
        os_trace("%s step 2 not used failed\r\n", __func__);
        return false;
    }
    offset += used;

    results->decode_type = AQUA;
    results->bits = nbits;
    // results->command = 0;
    // results->address = 0;


    os_trace("%s success\r\n", __func__);
    return true;
}


void sendAquaAc(const uint8_t *data, uint16_t nbytes, uint16_t repeat)
{
    vTaskEnterCritical();

    enableIROut(38, 50);

    mark(kAquaHdrMark);
    space(kAquaSpaceMark);

    sendGenericBytes(kAquaHdrMark, kAquaHdrSpace,
                    520, 1700,
                    500, 600, 520, 0, data, kAquaAcStateLength, 38, false, repeat, 50);

    vTaskExitCritical();
}

static uint8_t reverse_bit(uint8_t num, uint8_t n)
{
    uint8_t res = 0;
    for (int i = 0; i < n; i++) {
        res = (res << 1) | (num & 1);
        num >>= 1;
    }
    return res;
}

static void hash_code(AquaProtocol* data, state_ir_t state_ir)
{
    uint8_t dta_off[] = {0x65,0x01,0x07,0x00,0x00,0x05,0x00,0x04,0x00,0x00,0x00,0x00,0xA0,0x00};
    memset(data->data.raw, 0, kAquaAcStateLength);
    memcpy(data->data.raw, dta_off, kAquaAcStateLength);

    if (state_ir.power) {
        data->data.power = 1;
        data->data.reverse_power = 3;
    }

    switch (state_ir.mode)
    {
    case opmode_kCool:
        data->data.mode = 0x4;
        break;
    
    case opmode_kDry:
        data->data.mode = 0x2;
        break;
    
    default:
        break;
    }

    switch (state_ir.fanspeed)
    {
    case fanspeed_kMax:
        data->data.fan = 0x4;
        break;
    
    case fanspeed_kMin:
        data->data.fan = 0x6;
        break;
    
    case fanspeed_kMedium:
        data->data.fan = 0x2;
        break;
    
    case fanspeed_kAuto:
        data->data.fan = 0x5;
        break;
    
    default:
        break;
    }

    uint8_t data_temp = (uint8_t )state_ir.degrees - 16;
    uint8_t data_temp_reverse = 0;

    for (int i = 0; i < 4; i++) {
        data_temp_reverse = (data_temp_reverse << 1) | (data_temp & 1);
        data_temp >>= 1;
    }
    
    data->data.reverse_temp = data_temp_reverse;

    if (state_ir.mode == opmode_kCool) {
        switch (state_ir.fanspeed)
        {
            case fanspeed_kMax:
                data->data.raw[13] |= 0xe8;
                break;
            
            case fanspeed_kMin:
                data->data.raw[13] |= 0xea;
                break;
            
            case fanspeed_kMedium:
                data->data.raw[13] |= 0xec;
                break;
            
            case fanspeed_kAuto:
                data->data.raw[13] |= 0xe9;
                break;
            
            default:
                break;
        }
    } else if (state_ir.mode == opmode_kDry) {
        switch (state_ir.fanspeed)
        {
            case fanspeed_kMax:
                data->data.raw[13] |= 0xec;
                break;
            
            case fanspeed_kMin:
                data->data.raw[13] |= 0xee;
                break;
            
            case fanspeed_kMedium:
                data->data.raw[13] |= 0xea;
                break;
            
            case fanspeed_kAuto:
                data->data.raw[13] |= 0xed;
                break;
            
            default:
                break;
        } 
    }

    uint8_t res_revese = reverse_bit(data->data.raw[13] & 0x0F, 4) + ((uint8_t)state_ir.degrees - 16);

    data->data.raw[13] &= 0xF0;

    data->data.raw[13] |= reverse_bit(res_revese & 0x0F, 4);

}

void Aqua_send(state_ir_t state_ir)
{

    AquaProtocol protocol;

    switch ((uint8_t)state_ir.mode)
    {
    case 2:
        state_ir.mode = opmode_kCool;
        break;
    
    case 4:
        state_ir.mode = opmode_kDry;
        break;
    
    default:
        break;
    }

    switch ((uint8_t)state_ir.fanspeed)
    {
    case 6:
        state_ir.fanspeed = fanspeed_kMin;
        break;
    
    case 4:
        state_ir.fanspeed = fanspeed_kMedium;
        break;
    
    case 2:
        state_ir.fanspeed = fanspeed_kMax;
        break;
    
    case 10:
        state_ir.fanspeed = fanspeed_kAuto;
        break;
    
    default:
        break;
    }

    hash_code(&protocol, state_ir);

    // os_trace("AQUA_SEND: 0x");
    // for (uint8_t i = 0; i < 14; i++) {
    //     os_trace("%02x", protocol.data.raw[i]);
    // }
    // os_trace("\r\n");

    sendAquaAc(protocol.data.raw, kAquaAcStateLength, 0);
}