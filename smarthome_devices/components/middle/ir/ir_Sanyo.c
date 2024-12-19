#include "ir_Sanyo.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

// Constants
// Sanyo SA 8650B
const uint16_t kSanyoSa8650bHdrMark = 3500; // seen range 3500
const uint16_t kSanyoSa8650bHdrSpace = 950; // seen 950
const uint16_t kSanyoSa8650bOneMark = 2400; // seen 2400
const uint16_t kSanyoSa8650bZeroMark = 700; // seen 700
// usually see 713 - not using ticks as get number wrapround
const uint16_t kSanyoSa8650bDoubleSpaceUsecs = 800;
const uint16_t kSanyoSa8650bRptLength = 45000;

// Sanyo LC7461
const uint16_t kSanyoLc7461AddressMask = (1 << kSanyoLC7461AddressBits) - 1;
const uint16_t kSanyoLc7461CommandMask = (1 << kSanyoLC7461CommandBits) - 1;
const uint16_t kSanyoLc7461HdrMark = 9000;
const uint16_t kSanyoLc7461HdrSpace = 4500;
const uint16_t kSanyoLc7461BitMark = 560;   // 1T
const uint16_t kSanyoLc7461OneSpace = 1690; // 3T
const uint16_t kSanyoLc7461ZeroSpace = 560; // 1T
const uint32_t kSanyoLc7461MinCommandLength = 108000;

const uint16_t kSanyoLc7461MinGap =
    kSanyoLc7461MinCommandLength -
    (kSanyoLc7461HdrMark + kSanyoLc7461HdrSpace +
     kSanyoLC7461Bits * (kSanyoLc7461BitMark +
                         (kSanyoLc7461OneSpace + kSanyoLc7461ZeroSpace) / 2) +
     kSanyoLc7461BitMark);

const uint16_t kSanyoAcHdrMark = 8500;           ///< uSeconds
const uint16_t kSanyoAcHdrSpace = 4200;          ///< uSeconds
const uint16_t kSanyoAcBitMark = 500;            ///< uSeconds
const uint16_t kSanyoAcOneSpace = 1600;          ///< uSeconds
const uint16_t kSanyoAcZeroSpace = 550;          ///< uSeconds
const uint32_t kSanyoAcGap = kDefaultMessageGap; ///< uSeconds (Guess only)
const uint16_t kSanyoAcFreq = 38000;             ///< Hz. (Guess only)

const uint16_t kSanyoAc88HdrMark = 5400;    ///< uSeconds
const uint16_t kSanyoAc88HdrSpace = 2000;   ///< uSeconds
const uint16_t kSanyoAc88BitMark = 500;     ///< uSeconds
const uint16_t kSanyoAc88OneSpace = 1500;   ///< uSeconds
const uint16_t kSanyoAc88ZeroSpace = 750;   ///< uSeconds
const uint32_t kSanyoAc88Gap = 3675;        ///< uSeconds
const uint16_t kSanyoAc88Freq = 38000;      ///< Hz. (Guess only)
const uint8_t kSanyoAc88ExtraTolerance = 5; /// (%) Extra tolerance to use.

const uint16_t kSanyoAc152HdrMark = 3300;     ///< uSeconds
const uint16_t kSanyoAc152BitMark = 440;      ///< uSeconds
const uint16_t kSanyoAc152HdrSpace = 1725;    ///< uSeconds
const uint16_t kSanyoAc152OneSpace = 1290;    ///< uSeconds
const uint16_t kSanyoAc152ZeroSpace = 405;    ///< uSeconds
const uint16_t kSanyoAc152Freq = 38000;       ///< Hz. (Guess only)
const uint8_t kSanyoAc152ExtraTolerance = 13; /// (%) Extra tolerance to use.

static uint8_t pre_SanyoAc152_power = 1;

static uint8_t IRSanyoAc_calcChecksum(const uint8_t state[],
                                      const uint16_t length)
{
  return length ? sumNibblesByte(state, length - 1, 0) : 0;
}

static bool IRSanyoAc_validChecksum(const uint8_t state[], const uint16_t length)
{
  return length && state[length - 1] == IRSanyoAc_calcChecksum(state, length);
}

bool decodeSanyoAc(decode_results *results, uint16_t offset,
                   const uint16_t nbits, const bool strict)
{
  if (strict && nbits != kSanyoAcBits)
    return false;

  // Header + Data + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                              results->rawlen - offset, nbits,
                              kSanyoAcHdrMark - 500, kSanyoAcHdrMark + 500, kSanyoAcHdrSpace - 400, kSanyoAcHdrSpace + 400,
                              kSanyoAcBitMark - 400, kSanyoAcBitMark + 350, kSanyoAcOneSpace - 500, kSanyoAcOneSpace + 500,
                              kSanyoAcBitMark - 400, kSanyoAcBitMark + 350, kSanyoAcZeroSpace - 400, kSanyoAcZeroSpace + 400,
                              kSanyoAcBitMark - 400, kSanyoAcBitMark + 350, kSanyoAcGap - 25000, kSanyoAcGap + 25000,
                              true, kUseDefTol, kMarkExcess, false))
    return false;
  // Compliance
  if (strict)
    if (!IRSanyoAc_validChecksum(results->state, nbits / 8))
      return false;

  os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = SANYO_AC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRSanyoAc_setInfo(const uint8_t newState[], state_ir_t *ir_state)
{
  SanyoProtocol _;
  memset(&_, 0, sizeof(SanyoProtocol));
  memcpy(_.raw, newState, kSanyoAcStateLength);

  ir_state->power = (_.Power == kSanyoAcPowerOn);
  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;
  ir_state->degrees = _.Temp + kSanyoAcTempDelta;
}

static void sendSanyoAc(const uint8_t data[], const uint16_t nbytes,
                        const uint16_t repeat)
{
  // Header + Data + Footer
  vTaskEnterCritical();
  sendGenericBytes(kSanyoAcHdrMark, kSanyoAcHdrSpace,
                   kSanyoAcBitMark, kSanyoAcOneSpace,
                   kSanyoAcBitMark, kSanyoAcZeroSpace,
                   kSanyoAcBitMark, kSanyoAcGap,
                   data, nbytes, kSanyoAcFreq, false, repeat, kDutyDefault);
  vTaskExitCritical();
}

void SanyoAc_send(state_ir_t state_ir)
{
  SanyoProtocol _;
  memset(&_, 0, sizeof(SanyoProtocol));
  uint8_t kReset[kSanyoAcStateLength] = {
      0x6A, 0x6D, 0x51, 0x00, 0x10, 0x45, 0x00, 0x00, 0x33};
  memcpy(_.raw, kReset, kSanyoAcStateLength);

  _.Power = (state_ir.power ? kSanyoAcPowerOn : kSanyoAcPowerOff);
  switch ((uint8_t)state_ir.mode)
  {
  case kSanyoAcAuto:
  case kSanyoAcCool:
  case kSanyoAcDry:
  case kSanyoAcHeat:
    _.Mode = state_ir.mode;
    break;
  default:
    _.Mode = kSanyoAcAuto;
  }

  _.Fan = state_ir.fanspeed;
  uint8_t temp = max((uint8_t)kSanyoAcTempMin, (uint8_t)state_ir.degrees);
  temp = min((uint8_t)kSanyoAcTempMax, temp);
  _.Temp = temp - kSanyoAcTempDelta;
  _.SwingV = kSanyoAcSwingVAuto;

  _.Sum = IRSanyoAc_calcChecksum(_.raw, kSanyoAcStateLength);
  sendSanyoAc(_.raw, kSanyoAcStateLength, kNoRepeat);
}

bool decodeSanyoAc88(decode_results *results, uint16_t offset,
                     const uint16_t nbits, const bool strict)
{
  if (strict && nbits != kSanyoAc88Bits)
    return false;

  uint16_t used = 0;
  // Compliance
  const uint16_t expected_repeats = strict ? kSanyoAc88MinRepeat : 0;

  // Handle the expected nr of repeats.
  for (uint16_t r = 0; r <= expected_repeats; r++)
  {
    // Header + Data + Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                                  results->rawlen - offset, nbits,
                                  kSanyoAc88HdrMark - 400, kSanyoAc88HdrMark + 400, kSanyoAc88HdrSpace - 400, kSanyoAc88HdrSpace + 400,
                                  kSanyoAc88BitMark - 350, kSanyoAc88BitMark + 350, kSanyoAc88OneSpace - 400, kSanyoAc88OneSpace + 400,
                                  kSanyoAc88BitMark - 350, kSanyoAc88BitMark + 350, kSanyoAc88ZeroSpace - 350, kSanyoAc88ZeroSpace + 350,
                                  kSanyoAc88BitMark - 350, kSanyoAc88BitMark + 350,
                                  // Expect an inter-message gap, or just the end of msg?
                                  (r < expected_repeats) ? (kSanyoAc88Gap - 1000)
                                                         : (kDefaultMessageGap - 25000),
                                  (r < expected_repeats) ? (kSanyoAc88Gap + 1000)
                                                         : (kDefaultMessageGap + 25000),
                                  r == expected_repeats,
                                  _tolerance + kSanyoAc88ExtraTolerance,
                                  kMarkExcess, false);
    if (!used)
      return false; // No match!
    offset += used;
  }

  os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = SANYO_AC88;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

static uint8_t SanyoAc152_calcChecksum(const uint8_t state[],
                                       const uint16_t length)
{
  // os_trace("%s %u\r\n", __func__, sumNibblesByte(state, length - 1, 0xF8));
  return length ? sumNibblesByte(state, length - 1, 0xF8) : 0;
}

static bool SanyoAc152_validChecksum(const uint8_t state[], const uint16_t length)
{
  return length && state[length - 1] == SanyoAc152_calcChecksum(state, length);
}

bool decodeSanyoAc152(decode_results *results, uint16_t offset,
                      const uint16_t nbits, const bool strict)
{
  if (strict && nbits != kSanyoAc152Bits)
    return false;

  // Header + Data + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                              results->rawlen - offset, nbits,
                              kSanyoAc152HdrMark - 400, kSanyoAc152HdrMark + 400, kSanyoAc152HdrSpace - 400, kSanyoAc152HdrSpace + 400,
                              kSanyoAc152BitMark - 340, kSanyoAc152BitMark + 360, kSanyoAc152OneSpace - 390, kSanyoAc152OneSpace + 410,
                              kSanyoAc152BitMark - 340, kSanyoAc152BitMark + 360, kSanyoAc152ZeroSpace - 305, kSanyoAc152ZeroSpace + 305,
                              kSanyoAc152BitMark - 340, kSanyoAc152BitMark + 360,
                              kDefaultMessageGap - 25000, kDefaultMessageGap + 25000, // Just a guess.
                              false, _tolerance + kSanyoAc152ExtraTolerance,
                              kMarkExcess, false))
    return false; // No match!

  if (!SanyoAc152_validChecksum(results->state, nbits / 8))
  {
    os_trace("%s invalid checksum\r\n", __func__);
  }

  os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = SANYO_AC152;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRSanyo152Ac_setInfo(const uint8_t newState[], state_ir_t *ir_state)
{
  SanyoAc152Protocol _;
  memset(&_, 0, sizeof(SanyoAc152Protocol));
  memcpy(_.raw, newState, kSanyoAc152StateLength);

  ir_state->power = (_.Power == kSanyoAc152PowerOn);
  pre_SanyoAc152_power = ir_state->power;
  ir_state->degrees = _.Temp + kSanyoAc152TempDelta;
  if (_.Auto && _.Mode == kSanyoAc152FanClean)
  {
    if (!_.is_clothes_mode)
    {
      ir_state->mode = kSanyoAc152Auto;
      if (_.Degree_minus)
      {
        ir_state->degrees -= _.Temp_change;
      }
      else
      {
        ir_state->degrees += _.Temp_change;
      }
    }
    else
    {
      ir_state->mode = kSanyoAc152ClothesDry;
    }
  }
  else
  {
    ir_state->mode = _.Mode;
  }
  ir_state->fanspeed = _.Fan | (_.Quite << 2);
  ir_state->swingv = _.SwingV;
  os_trace("%s Power: %u, button: %u, Auto: %u, Temp_change: %u, Degree_minus: %u, is_heat: %u\r\n", __func__, _.Power, _.Button, _.Auto, _.Temp_change, _.Degree_minus, _.not_heat);
}

static void sendSanyoAc152(const uint8_t data[], const uint16_t nbytes,
                           const uint16_t repeat)
{
  // (Header + Data + Footer) per repeat
  vTaskEnterCritical();
  sendGenericBytes(kSanyoAc152HdrMark, kSanyoAc152HdrSpace,
                   kSanyoAc152BitMark, kSanyoAc152OneSpace,
                   kSanyoAc152BitMark, kSanyoAc152ZeroSpace,
                   kSanyoAc152BitMark, kDefaultMessageGap,
                   data, nbytes, kSanyoAc152Freq, false, repeat, kDutyDefault);
  space(kDefaultMessageGap); // Make a guess at a post message gap.
  vTaskExitCritical();
}

void SanyoAc152_send(state_ir_t state_ir)
{
  SanyoAc152Protocol _;
  memset(&_, 0, sizeof(SanyoAc152Protocol));
  uint8_t kReset[kSanyoAc152StateLength] = {
      0x40, 0x00, 0x14, 0x80, 0x4B, 0x80, 0x22, 0xF8, 0x23, 0x00, 0x30, 0x00, 0x03, 0x01, 0x10, 0x00, 0x20, 0x00, 0x4A};
  memcpy(_.raw, kReset, kSanyoAc152StateLength);

  if (pre_SanyoAc152_power != state_ir.power)
  {
    if (state_ir.power)
    {
      _.Button = 2;
    }
    else
    {
      _.Button = 1;
    }
    pre_SanyoAc152_power = state_ir.power;
  }
  _.Power = state_ir.power;

  uint8_t temp = max((uint8_t)kSanyoAc152TempMin, (uint8_t)state_ir.degrees);
  temp = min((uint8_t)kSanyoAc152TempMax, temp);
  _.Temp = temp - kSanyoAc152TempDelta;
  switch ((uint8_t)state_ir.mode)
  {
  case kSanyoAc152FanClean:
    _.Clean = 1;
  case kSanyoAc152Heat:
  case kSanyoAc152Cool:
  case kSanyoAc152Dry:
    _.Mode = state_ir.mode;
    break;
  case kSanyoAc152Auto:
    _.Mode = 0;
    _.Auto = 1;
    _.Temp = 24 - kSanyoAc152TempDelta;
    if (state_ir.degrees > 24)
    {
      _.Degree_minus = 0;
      uint8_t temp = min(4, (uint8_t)state_ir.degrees - 24);
      _.Temp_change = (uint8_t)state_ir.degrees - 24;
    }
    else if (state_ir.degrees < 24)
    {
      _.Degree_minus = 1;
      uint8_t temp = min(4, 24 - (uint8_t)state_ir.degrees);
      _.Temp_change = temp;
    }
    break;
  case kSanyoAc152ClothesDry:
    _.Mode = 0;
    _.is_clothes_mode = 1;
    _.Auto = 1;
    _.raw[6] = 0xAA;
    _.raw[16] = 0x80;
    _.raw[9] = 0x79;
    break;
  }

  if (state_ir.mode == kSanyoAc152Heat)
  {
    _.not_heat = 0;
  }

  _.Fan = state_ir.fanspeed;
  if (state_ir.fanspeed == kSanyoAc152FanQuite)
  {
    _.Fan = kSanyoAc152FanMedium;
    _.Quite = 1;
  }
  if (state_ir.swingv != swingv_kOff)
  {
    _.SwingV = state_ir.swingv;
  }

  _.Sum = SanyoAc152_calcChecksum(_.raw, kSanyoAc152StateLength);
#if DEBUG_ENABLE == 1
  os_trace("%s: 0x ", __func__);
  for (uint8_t i = 0; i < kSanyoAc152StateLength; i++)
  {
    os_trace("%02X-", _.raw[i]);
  }
  os_trace("\r\n");
#endif
  sendSanyoAc152(_.raw, kSanyoAc152StateLength, kNoRepeat);
}
