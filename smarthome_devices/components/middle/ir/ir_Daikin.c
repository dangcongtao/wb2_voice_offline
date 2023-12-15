#include "ir_Daikin.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

static bool IRDaikin_validChecksum(uint8_t state[], const uint16_t length) {
  // Data #1
  if (length < kDaikinSection1Length ||
      state[kDaikinByteChecksum1] != sumBytesState(state, kDaikinSection1Length - 1, 0))
    return false;
  // Data #2
  if (length < kDaikinSection1Length + kDaikinSection2Length ||
      state[kDaikinByteChecksum2] != sumBytesState(state + kDaikinSection1Length,
                                              kDaikinSection2Length - 1, 0))
    return false;
  // Data #3
  if (length < kDaikinSection1Length + kDaikinSection2Length + 2 ||
      state[length - 1] != sumBytesState(state + kDaikinSection1Length +
                                    kDaikinSection2Length,
                                    length - (kDaikinSection1Length +
                                              kDaikinSection2Length) - 1, 0))
    return false;
  return true;
}

bool decodeDaikin(decode_results *results, uint16_t offset,
                          const uint16_t nbits, const bool strict) 
{
  // Is there enough data to match successfully?
  if (results->rawlen < (2 * (nbits + kDaikinHeaderLength) +
                         kDaikinSections * (kHeader + kFooter) + kFooter - 1) +
                         offset) 
    {
        os_trace("%s Is there enough data to match successfully\r\n", __func__);
        return false;
    }

  // Compliance
  if (strict && nbits != kDaikinBits) {
    os_trace("%s Compliance\r\n", __func__);
    return false;
  }

  match_result_t data_result;

  // Header #1 - Doesn't count as data.
  data_result = matchDataLimit(&(results->rawbuf[offset]), kDaikinHeaderLength,
                          kDaikinBitMark - 300, kDaikinBitMark + 300, kDaikinOneSpace - 300, kDaikinOneSpace + 300,
                          kDaikinBitMark - 300, kDaikinBitMark + 300, kDaikinZeroSpace - 300, kDaikinZeroSpace + 300,
                          kDaikinTolerance, kDaikinMarkExcess, false, true);
  offset += data_result.used;
  if (data_result.success == false) {
    os_trace("%s Fail\r\n", __func__);
    return false;  // Fail
  }
  if (data_result.data) {
    os_trace("%s The header bits should be zero.\r\n", __func__);
    return false;  // The header bits should be zero.
  }
  // Footer
  if (!matchLimit(results->rawbuf[offset++], kDaikinBitMark - 300, kDaikinBitMark + 300)) {
                    os_trace("%s Footer 1\r\n", __func__);
                    return false;
                 }
  if (!matchLimit(results->rawbuf[offset++], kDaikinZeroSpace + kDaikinGap - 7000, kDaikinZeroSpace + kDaikinGap + 7000 )) {
                    os_trace("%s Footer 2\r\n", __func__);
                    return false;
                  }
  // Sections
  uint8_t ksectionSize[] = {
      kDaikinSection1Length, kDaikinSection2Length, kDaikinSection3Length};
  uint16_t pos = 0;
  for (uint8_t section = 0; section < kDaikinSections; section++) {
    uint16_t used;
    // Section Header + Section Data (7 bytes) + Section Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, ksectionSize[section] * 8,
                        kDaikinHdrMark-400, kDaikinHdrMark+500, kDaikinHdrSpace-500, kDaikinHdrSpace+400,
                        kDaikinBitMark-300, kDaikinBitMark+300, kDaikinOneSpace-300, kDaikinOneSpace+300,
                        kDaikinBitMark-300, kDaikinBitMark+300, kDaikinZeroSpace-300, kDaikinZeroSpace+400,
                        kDaikinBitMark-300, kDaikinBitMark+300, kDaikinZeroSpace + kDaikinGap-7000, kDaikinZeroSpace + kDaikinGap+7000,
                        section >= kDaikinSections - 1,
                        45, kDaikinMarkExcess, false);
    if (used == 0) {
        os_trace("%s used == 0, section %u\r\n", __func__, section);
        return false;
    }
    offset += used;
    pos += ksectionSize[section];
  }
  // Compliance
  if (strict) {
    // Re-check we got the correct size/length due to the way we read the data.
    if (pos * 8 != kDaikinBits) return false;
    // Validate the checksum.
    if (!IRDaikin_validChecksum(results->state, kDaikinStateLength)) {
        os_trace("%s Validate checksum failed\r\n", __func__);
        return false;
    }
  }

  // Success
  results->decode_type = DAIKIN;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  os_trace("%s success\r\n", __func__);
  return true;
}