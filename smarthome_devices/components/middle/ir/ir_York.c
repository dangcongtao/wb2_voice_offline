#include "ir_York.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"


// Constants
const uint16_t kYorkHdrMark = 4887;
const uint16_t kYorkBitMark = 612;
const uint16_t kYorkHdrSpace = 2267;
const uint16_t kYorkOneSpace = 1778;
const uint16_t kYorkZeroSpace = 579;
const uint16_t kYorkFreq = 38000;  // Hz. (Guessing the most common frequency.)


bool decodeYork(decode_results *results, uint16_t offset,
                        const uint16_t nbits, const bool strict) {
  if (strict && nbits != kYorkBits)
    return false;

  uint16_t used = 0;

  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, nbits,
                      kYorkHdrMark-1000, kYorkHdrMark+1000, kYorkHdrSpace-500, kYorkHdrSpace+500,
                      kYorkBitMark-400, kYorkBitMark+400, kYorkOneSpace-500, kYorkOneSpace+500,
                      kYorkBitMark-400, kYorkBitMark+400, kYorkZeroSpace - 400, kYorkZeroSpace+400,
                      kYorkBitMark-400, kYorkBitMark+400, kDefaultMessageGap - 25000, kDefaultMessageGap+25000,
                      false, _tolerance, kMarkExcess,
                      false);  // LSB
  if (used == 0) return false;  // We failed to find any data.
    os_trace("%s success\r\n", __func__);
  // Succes
  results->decode_type = YORK;
  results->bits = nbits;

  return true;
}