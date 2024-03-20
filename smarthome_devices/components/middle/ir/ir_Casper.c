#include "ir_Casper.h"
#include "system_typedef.h"
#include "string.h"


// const uint16_t kFunikiAcBitMark = 4400;
// const uint16_t kFunikiAcHdrSpace = 4400;
// const uint16_t kFunikiHdr1Mark = 1100;             ///< uSeconds.
// const uint16_t kFunikiHdr1Space = 110;            ///< uSeconds.
// const uint16_t kFunikiHdr2Mark = 300;            ///< uSeconds.

// const uint16_t kSamsungAcSectionMark = 600;

const uint32_t kCasperMinMessageLength = 108050;  ///< uSeconds.

void sendCasperAc(const uint8_t *data, uint16_t nbytes, uint16_t repeat)
{
    os_trace("%s enter\r\n", __func__);
    vTaskEnterCritical();
    uint16_t repeatHeaderMark = 0;
    uint8_t duty = kDutyDefault;

    enableIROut(38, 50);

    mark(9000);
    space(4500);
    
    for (uint16_t offset = 0; offset < nbytes;) {

        sendData(502, 1650, 501, 600, *(data + offset), 8, false);

        offset++;
        if (offset == 6) {
            mark(480);
            space(8000);
        } else if (offset == 14) {
            mark(480);
            space(8000);
        }
    }

    mark(500);
    vTaskExitCritical();
    os_trace("%s exit\r\n", __func__);
    
}

bool decodeCasperAC(decode_results *results, uint16_t offset, const uint16_t nbits, const bool strict) 
{
    // os_trace("%s begin\r\n", __func__);
    // os_trace("decodeFunikiAC() offset: %d, nbits: %d, kFunikiBits: %d\r\n", offset, nbits, kFunikiBits);
    if (results->rawlen != 344 || nbits != kCasperBits) {
        return false;
    }

    // os_trace("%s step 1\r\n", __func__);

    if (!matchLimit(results->rawbuf[offset++], 9000-2250, 9000+2250)){
        // os_trace("%s match mark failed, offset %u,  %u\r\n", __func__, offset, results->rawbuf[offset-1]);
        return false;
    }
    if (!matchLimit(results->rawbuf[offset++], 4500-1125, 4500+1125)){
        // os_trace("%s match space failed\r\n", __func__);
        return false;
    }

    
    for (uint16_t pos = 0; pos < kCasperAcStateLength;) {
        uint16_t used;
        uint8_t temp_nbits;
        if (pos == 0) {
            temp_nbits = 6;
        } else if (pos == 6) {
            temp_nbits = 8;
        } else {
            temp_nbits = 7;
        }
        // Section Header + Section Data (7 bytes) + Section Footer
        used = matchGenericStateLimit(results->rawbuf + offset, (uint8_t*)(&results->state[pos]),
                            results->rawlen - offset, temp_nbits * 8,
                            0, 0, 0, 0,
                            502-350, 502+350, 1740-550, 1740+550,
                            502-350, 502+350, 600-450, 600+450,
                            502-350, 502+350, 8000-2000, 8000+2000,
                            pos + temp_nbits >= nbits / 8,
                            50, 0, false);

        if (used == 0){
            os_trace("%s step 2 not used failed\r\n", __func__);
            return false;
        }
        offset += used;
        // os_trace("pos: %u, used: %u, offset: %u\r\n", pos, used, offset);

        pos += temp_nbits;
    }

    results->decode_type = CASPER;
    results->bits = nbits;

    if (results->state[0] != 0x83) {
        os_trace("It is not ac device casper\r\n");
        return false;
    }

    // don set it to 0, It states will be 0
    // results->command = 0;
    // results->address = 0;


    os_trace("%s success\r\n", __func__);
    return true;
}


static uint8_t get_temp_casper(uint8_t *states)
{
    uint8_t temp = ((states[3] >> 4) & 0xF) + 16;
    return temp;
}


static fanspeed_t get_fan_casper(uint8_t *states)
{
    // memset(str, 0, 20);
    uint8_t val = (((states[2] & 0x3) << 1) | ((states[17] >> 6) & 0x1));
    switch (val)
    {
    case 0x2:
        return fanspeed_kMax;
        break;

    case 0x6:
        return fanspeed_kMin;
        break;
    
    case 0x4:
        return fanspeed_kMedium;
        break;

    case 0x3:
        return fanspeed_kHigh;
        break;
    
    case 0x7:
        return fanspeed_kLow;
        break;
    
    default:
        return fanspeed_kAuto;
        break;
    }
}


static opmode_t get_mode_casper(uint8_t *states)
{
    switch (states[3] & 0xF)
    {
    case 0x2:
        return opmode_kCool;
        break;

    case 0x4:
        return opmode_kFan;
        break;
    
    case 0x0:
        return opmode_kHeat;
        break;
    
    case 0x3:
        return opmode_kDry;
        break;
    
    default:
        return opmode_kAuto;
        break;
    }
}


static uint8_t is_power_on(uint8_t* states) 
{
    if ((states[18] >> 4) & 0x1) {
        return 1;
    }
    return 0;
}

static void hash_code(uint8_t* states, const state_ir_t *ir_state)
{
    if (ir_state->power == false) {
        os_trace("%s power off\r\n", __func__);
        uint8_t dta_off[] = {0x83,0x06,0x04,0x82,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x01,0x00,0x00,0x28,0x00,0x29};
        memset(states, 0, kCasperAcStateLength);
        memcpy(states, dta_off, kCasperAcStateLength);
        return;
    }
    uint8_t dta[] = {0x83,0x06,0x04,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x00,0x01,0x00,0x00,0x38,0x00,0x39};
    memset(states, 0, kCasperAcStateLength);
    memcpy(states, dta, kCasperAcStateLength);

    // mode
    switch (ir_state->mode)
    {
    case opmode_kCool:
        states[3] |= 0x2;
        break;

    case opmode_kFan:
        states[3] |= 0x4;
        break;
    
    case opmode_kHeat:
        states[3] |= 0x0;
        break;
    
    case opmode_kDry:
        states[3] |= 0x3;
        break;
    
    default:
        states[3] |= 0x2;
        break;
    }

    // fanspeed
    switch (ir_state->fanspeed)
    {
    case fanspeed_kMedium:
        states[2] |= 0b110;
        break;

    case fanspeed_kHigh:
        states[17] |= (0b1 << 6);
    case fanspeed_kMax:
        states[2] |= 0b101;
        break;
    
    case fanspeed_kLow:
        states[17] |= (0b1 << 6);
    case fanspeed_kMin:
        states[2] |= (uint8_t)0b111;
        break;
    
    default:
        states[2] |= 0b100;
        break;
    }

    uint8_t hash13;
    if (ir_state->mode == opmode_kFan) {
        states[3] |= (((uint8_t)0x7) << 4);
        hash13 = (((uint8_t)0xF) << 4);
    } else {
        states[3] |= (((uint8_t)ir_state->degrees - 16) << 4);
        hash13 = ((((uint8_t)ir_state->degrees - 8) % 16) << 4);
    }

    // if (ir_state->fanspeed == fanspeed_kHigh || ir_state->fanspeed == fanspeed_kLow) {
    //     states[17] = 0x40;
    // }

    uint8_t a1 = states[2] & 0xF;
    uint8_t a2 = states[3] & 0xF;
    hash13 |= a1 ^ a2;

    states[13] = hash13;

    uint8_t hash_final = 0x00;
    for (uint8_t i = 15; i < 20; i++) {
        hash_final ^= states[i];
    }
    states[20] = hash_final;
    
}

void Casper_send(state_ir_t ir_state)
{
    uint8_t dataa[21];
    switch ((uint8_t)ir_state.mode)
    {
    case 2:
        ir_state.mode = opmode_kFan;
        break;
    
    case 4:
        ir_state.mode = opmode_kCool;
        break;
    
    case 12:
        ir_state.mode = opmode_kDry;
        break;
    
    case 0:
        ir_state.mode = opmode_kHeat;
        break;
    
    default:
        break;
    }


    if (ir_state.fanspeed == 6)
    {
        ir_state.fanspeed = fanspeed_kAuto;
    }

    os_trace("power: %u\r\n", ir_state.power);
    hash_code(dataa, &ir_state);
    os_trace("CASPER_HASH: 0x");
    for (uint8_t i = 0; i < 21; i++) {
        if (i == 6 || i == 14) {
            os_trace("\t|\t");
        }
        os_trace("%02x", dataa[i]);
    }
    os_trace("\r\n");
    sendCasperAc(dataa, kCasperAcStateLength, 0);
}

void Casper_test_send_ir(void)
{
    state_ir_t ir_state;
    ir_state.mode = opmode_kCool;
    ir_state.degrees = 22;
    ir_state.fanspeed = fanspeed_kMin;
    ir_state.power = 1;

    


    uint8_t dataa[21];
    hash_code(dataa, &ir_state);
    os_trace("CASPER_HASH: 0x");
    for (uint8_t i = 0; i < 21; i++) {
        if (i == 6 || i == 14) {
            os_trace("\t|\t");
        }
        os_trace("%02x", dataa[i]);
    }
    os_trace("\r\n");
    sendCasperAc(dataa, kCasperAcStateLength, 0);
}

static void Casper_toCommon(const uint8_t* new_code, state_ir_t *ir_state)
{
    ir_state->mode = get_mode_casper(new_code);
    ir_state->fanspeed = get_fan_casper(new_code);
    ir_state->power = is_power_on(new_code);
    ir_state->degrees = get_temp_casper(new_code);
}

void Casper_setInfo(const uint8_t* new_code, state_ir_t *ir_state)
{
    ir_state->mode = get_mode_casper(new_code);
    ir_state->fanspeed = get_fan_casper(new_code);
    ir_state->power = is_power_on(new_code);
    ir_state->degrees = get_temp_casper(new_code);

    // switch ((uint8_t)ir_state->mode)
    // {
    // case opmode_kFan:
    //     ir_state->mode = 2;
    //     break;
    
    // case opmode_kCool:
    //     ir_state->mode = 4;
    //     break;
    
    // case opmode_kDry:
    //     ir_state->mode = 12;
    //     break;
    
    // case opmode_kHeat:
    //     ir_state->mode = 0;
    //     break;
    
    // default:
    //     break;
    // }


    // if (ir_state->fanspeed == fanspeed_kAuto)
    // {
    //     ir_state->fanspeed = 6;
    // }

    // os_trace("power: %u\r\n", ir_state.power);
}

void Casper_setRaw(const uint8_t* new_code, const decode_type_t protocol, state_ir_t *ir_state) 
{
    // memcpy(_.data.raw, new_code, kFunikiAcStateLength);
    Casper_toCommon(new_code, ir_state);
    // _.data.raw = new_code;
}

