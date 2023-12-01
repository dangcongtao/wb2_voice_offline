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

void AQUA_setRaw(const uint8_t* new_code, const decode_type_t protocol) 
{
    memcpy(_.data.raw, new_code, kAquaAcStateLength);
    // _.data.raw = new_code;
}

static uint8_t getTemp()
{
    uint8_t res = 0;
    uint8_t num = _.data.reverse_temp;
    for (int i = 0; i < 4; i++) {
        res = (res << 1) | (num & 1);
        num >>= 1;
    }
    os_trace("%s res: %u\r\n", __func__, res);
    if (res >= 9) {
        res += 7;
    } else {
        res += 23;
    }
    return res; 
}

void aqua_getString(void)
{
    os_trace("power: 0, mode: 0, temp: %u, fan: 0\r\n", getTemp());
}

bool decodeAquaAC(decode_results *results, uint16_t offset, const uint16_t nbits, const bool strict) 
{

    os_trace("%s() offset: %d, nbits: %d, kLg32Bits: %d\r\n", __func__, offset, nbits, kFunikiBits);

    // if (offset != kFunikiBits) {
    //     os_trace("step 1 failed\r\n");
    //     return false;
    // }

    if (results->rawlen != 230) {
        os_trace("aqua len invalid\r\n");
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
        os_trace("%s mark 1 failed\r\n", __func__);
        return false;
    }

    if (!matchSpace(results->rawbuf[offset++], kAquaSpaceMark, kUseDefTol, kMarkExcess)) {
        os_trace("%s space 2 failed\r\n", __func__);
        return false;
    }

    if (!matchMark(results->rawbuf[offset++], kAquaHdrMark, kUseDefTol, kMarkExcess)) {
        os_trace("%s mark 3 failed\r\n", __func__);
        return false;
    }

    memset(results->state, 0, 53);

    uint16_t used;
    // Section Header + Section Data (7 bytes) + Section Footer
    used = matchGeneric(results->rawbuf + offset, results->state,
                        results->rawlen - offset, nbits,
                        0, kAquaHdrSpace,
                        kAquaBitOneMark, 2200,
                        kAquaBitZeroMark, kAquaZeroSpace,
                        0, 0,
                        true,
                        50, 0, false);

    if (used == 0){
        os_trace("step 2 not used failed\r\n");
        return false;
    }
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