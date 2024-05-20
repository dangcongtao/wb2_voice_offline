#include "ir_Goodweather.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

bool decodeGoodweather(decode_results* results, uint16_t offset,
                               const uint16_t nbits,
                               const bool strict) {
    os_trace("%s start\r\n", __func__);
  if (results->rawlen < 2 * (2 * nbits) + kHeader + 2 * kFooter - 1 + offset)
    return false;  // Can't possibly be a valid Goodweather message.
  if (strict && nbits != kGoodweatherBits)
    return false;  // Not strictly a Goodweather message.

  uint64_t dataSoFar = 0;
  uint16_t dataBitsSoFar = 0;
  match_result_t data_result;

  // Header
  if (!matchLimit(results->rawbuf[offset++], kGoodweatherHdrMark-1000, kGoodweatherHdrMark+1000)) return false;
  if (!matchLimit(results->rawbuf[offset++], kGoodweatherHdrSpace-1000, kGoodweatherHdrSpace+1000))
    return false;

  // Data
  for (; offset <= results->rawlen - 32 && dataBitsSoFar < nbits;
       dataBitsSoFar += 8) {
    os_trace_rcv("DEBUG: Attempting Byte #%u\r\n", dataBitsSoFar / 8);
    // Read in a byte at a time.
    // Normal first.
    data_result = matchDataLimit(&(results->rawbuf[offset]), 8,
                            kGoodweatherBitMark-430, kGoodweatherBitMark+430, kGoodweatherOneSpace-430, kGoodweatherOneSpace+430,
                            kGoodweatherBitMark-430, kGoodweatherBitMark+430, kGoodweatherZeroSpace-600, kGoodweatherZeroSpace+600,
                            _tolerance + kGoodweatherExtraTolerance,
                            kMarkExcess, false, true);
    if (data_result.success == false) return false;
    os_trace_rcv("DEBUG: Normal byte read okay.\r\n");
    offset += data_result.used;
    uint8_t data = (uint8_t)data_result.data;
    // Then inverted.
    data_result = matchDataLimit(&(results->rawbuf[offset]), 8,
                            kGoodweatherBitMark-430, kGoodweatherBitMark+430, kGoodweatherOneSpace-430, kGoodweatherOneSpace+430,
                            kGoodweatherBitMark-430, kGoodweatherBitMark+430, kGoodweatherZeroSpace-600, kGoodweatherZeroSpace+600,
                            _tolerance + kGoodweatherExtraTolerance,
                            kMarkExcess, false, true);
    if (data_result.success == false) return false;
    os_trace_rcv("DEBUG: Inverted byte read okay.\r\n");
    offset += data_result.used;
    uint8_t inverted = (uint8_t)data_result.data;
    os_trace_rcv("DEBUG: data = %u\r\n", (uint16_t)data);
    os_trace_rcv("DEBUG: inverted = %u\r\n", (uint16_t)inverted);
    if (data != (inverted ^ 0xFF)) return false;  // Data integrity failed.
    dataSoFar |= (uint64_t)data << dataBitsSoFar;
  }

  // Footer.
  if (!matchLimit(results->rawbuf[offset++], kGoodweatherBitMark-430, kGoodweatherBitMark+430)) return false;
  if (!matchLimit(results->rawbuf[offset++], kGoodweatherHdrSpace-1000, kGoodweatherHdrSpace+1000))
    return false;
  if (!matchLimit(results->rawbuf[offset++], kGoodweatherBitMark-430, kGoodweatherBitMark+430)) return false;
  if (offset <= results->rawlen &&
      !matchAtLeastLimit(results->rawbuf[offset], kGoodweatherHdrSpace-1000))
    return false;

  // Compliance
  if (strict && (dataBitsSoFar != kGoodweatherBits)) return false;

    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = GOODWEATHER;
  results->bits = dataBitsSoFar;
  results->value = dataSoFar;
  results->address = 0;
  results->command = 0;
  return true;
}

void GoodweatherAc_setRaw(const uint64_t state, state_ir_t *ir_state)
{ 
    GoodweatherProtocol _;
    memset(&_, 0, sizeof(GoodweatherProtocol));
    _.raw  = state; 

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kGoodweatherTempMin;
    ir_state->swingv = _.Swing;
}