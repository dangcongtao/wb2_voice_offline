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
    os_trace("%s cmd: %u\r\n", __func__, _.Command);
    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kGoodweatherTempMin;
    ir_state->swingv = _.Swing;
}

void sendGoodweather(const uint64_t data, const uint16_t nbits,
                             const uint16_t repeat) {
  if (nbits != kGoodweatherBits)
    return;  // Wrong nr. of bits to send a proper message.
  // Set IR carrier frequency
  vTaskEnterCritical();
  enableIROut(38, 50);

  for (uint16_t r = 0; r <= repeat; r++) {
    // Header
    mark(kGoodweatherHdrMark);
    space(kGoodweatherHdrSpace);

    // Data
    for (int16_t i = 0; i < nbits; i += 8) {
      uint16_t chunk = (data >> i) & 0xFF;  // Grab a byte at a time.
      chunk = (~chunk) << 8 | chunk;  // Prepend a inverted copy of the byte.
      sendData(kGoodweatherBitMark, kGoodweatherOneSpace,
               kGoodweatherBitMark, kGoodweatherZeroSpace,
               chunk, 16, false);
    }
    // Footer
    mark(kGoodweatherBitMark);
    space(kGoodweatherHdrSpace);
    mark(kGoodweatherBitMark);
    space(kDefaultMessageGap);
  }
  vTaskExitCritical();
}

void Asanzo_send(state_ir_t state_ir)
{
  GoodweatherProtocol _;
  memset(&_, 0, sizeof(GoodweatherProtocol));
  _.raw = kGoodweatherStateInit;
  _.Command = kGoodweatherCmdPower;
  _.Power = state_ir.power;

  switch ((uint8_t)state_ir.mode) {
    case kGoodweatherAuto:
    case kGoodweatherDry:
    case kGoodweatherCool:
    case kGoodweatherFan:
    case kGoodweatherHeat:
      _.Mode = state_ir.mode;
      break;
    default:
      _.Mode = kGoodweatherAuto;
  }

  switch ((uint8_t)state_ir.fanspeed) {
    case kGoodweatherFanAuto:
    case kGoodweatherFanLow:
    case kGoodweatherFanMed:
    case kGoodweatherFanHigh:
      _.Fan = state_ir.fanspeed;
      break;
    default:
      _.Fan = kGoodweatherFanAuto;
  }

  uint8_t new_temp = max(kGoodweatherTempMin, (uint8_t)state_ir.degrees);
  new_temp = min(kGoodweatherTempMax, new_temp);
  // if (new_temp > getTemp()) _.Command = kGoodweatherCmdUpTemp;
  // if (new_temp < getTemp()) _.Command = kGoodweatherCmdDownTemp;
  _.Temp = new_temp - kGoodweatherTempMin;

  if (state_ir.swingv != swingv_kOff) {
    switch ((uint8_t)state_ir.swingv) {
      case kGoodweatherSwingOff:
      case kGoodweatherSwingSlow:
      case kGoodweatherSwingFast:
        _.Swing = state_ir.swingv;
        break;
      default:
        _.Swing = kGoodweatherSwingOff;
    }
  }
  os_trace("%s raw: %llu\r\n", __func__, _.raw);
  sendGoodweather(_.raw, kGoodweatherBits, kGoodweatherMinRepeat);
}