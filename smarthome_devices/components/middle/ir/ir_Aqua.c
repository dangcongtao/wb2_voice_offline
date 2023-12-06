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

    // if (offset != kFunikiBits) {
    //     os_trace("step 1 failed\r\n");
    //     return false;
    // }

    if (results->rawlen != 230) {
        // os_trace("aqua len invalid\r\n");
        return false;
    }

    // if (!matchGeneric(results->rawbuf + offset, &data,
    //                 results->rawlen - offset, nbits,
    //                 kSamsungHdrMark, kSamsungHdrSpace,
    //                 kSamsungBitMark, kSamsungOneSpace,
    //                 kSamsungBitMark, kSamsungZeroSpace,
    //                 kSamsungBitMark, kSamsungMinGap, true, kUseDefTol, 0, true)) return false;

    // if (!matchMark(results->rawbuf[offset], kFunikiAcBitMark, kUseDefTol, kMarkExcess)){
    //     os_trace("match mark 1 failed\r\n");
    //     return false;
    // }

    // if (!matchSpace(results->rawbuf[offset++], kFunikiAcHdrSpace, kUseDefTol, kMarkExcess)){
    //     os_trace("match space 2 failed\r\n");
    //     return false;
    // }

    if (!matchMark(results->rawbuf[offset++], kAquaHdrMark, kUseDefTol, kMarkExcess)) {
        // os_trace("%s mark 1 failed\r\n", __func__);
        return false;
    }

    if (!matchSpace(results->rawbuf[offset++], kAquaSpaceMark, kUseDefTol, kMarkExcess)) {
        // os_trace("%s space 2 failed\r\n", __func__);
        return false;
    }

    if (!matchMark(results->rawbuf[offset++], kAquaHdrMark, kUseDefTol, kMarkExcess)) {
        // os_trace("%s mark 3 failed\r\n", __func__);
        return false;
    }

    memset(results->state, 0, 53);

    uint16_t used;
    // Section Header + Section Data (7 bytes) + Section Footer
    used = matchGenericUint8(results->rawbuf + offset, results->state,
                        results->rawlen - offset, nbits,
                        0, kAquaHdrSpace,
                        kAquaBitOneMark, 2200,
                        kAquaBitZeroMark, kAquaZeroSpace,
                        0, 0,
                        true,
                        50, 0, false);

    if (used == 0){
        os_trace("%s step 2 not used failed\r\n", __func__);
        return false;
    }
    os_trace("used: %u\r\n", used);
    offset += used;

    // if (!matchMark(results->rawbuf[offset++], kFunikiHdr2Mark, kUseDefTol, kMarkExcess)){
    //     os_trace("match mark 2 failed\r\n");
    //     return false;
    // }

//     uint64_t data = 0;
//   uint16_t used = matchGeneric(results->rawbuf + offset, &data,
//                                results->rawlen - offset, nbits,
//                                0,  // Already matched the Header mark.
//                                kFunikiHdr1Space,
//                                kBitmark, kLgOneSpace, kBitmark, kLgZeroSpace,
//                                kBitmark, kLgMinGap, true, kUseDefTol, 0, true);


    results->decode_type = AQUA;
    results->bits = nbits;
    results->command = 0;
    results->address = 0;


    os_trace("decode Aqua success\r\n");
    return true;
}