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

    os_trace("decodeFunikiAC() offset: %d, nbits: %d, kLg32Bits: %d\r\n", offset, nbits, kFunikiBits);

    // if (offset != kFunikiBits) {
    //     os_trace("step 1 failed\r\n");
    //     return false;
    // }

    if (results->rawlen != 200) {
        os_trace("funiki len invalid\r\n");
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

    for (uint16_t pos = 0; pos <= (nbits / 8) - kFunikiAcSectionLength;
       pos += kFunikiAcSectionLength) {

        uint16_t used;
        // Section Header + Section Data (7 bytes) + Section Footer
        used = matchGeneric(results->rawbuf + offset, results->state + pos,
                            results->rawlen - offset, kFunikiAcSectionLength * 8,
                            5050, 4450,
                            600, 1500,
                            600, 460,
                            600, 4300,
                            pos + kFunikiAcSectionLength >= nbits / 8,
                            _tolerance, 0, false);

        if (used == 0){
            os_trace("step 2 not used failed\r\n");
            return false;
        }
        offset += used;

    }

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


    results->decode_type = FUNIKI;
    results->bits = nbits;


    os_trace("decode Funiki success\r\n");
    return true;
}