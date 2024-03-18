#include "ir_Sharp.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

// Constants
// period time = 1/38000Hz = 26.316 microseconds.
const uint16_t kSharpTick = 26;
const uint16_t kSharpBitMarkTicks = 10;
const uint16_t kSharpBitMark = kSharpBitMarkTicks * kSharpTick;
const uint16_t kSharpOneSpaceTicks = 70;
const uint16_t kSharpOneSpace = kSharpOneSpaceTicks * kSharpTick;
const uint16_t kSharpZeroSpaceTicks = 30;
const uint16_t kSharpZeroSpace = kSharpZeroSpaceTicks * kSharpTick;
const uint16_t kSharpGapTicks = 1677;
const uint16_t kSharpGap = kSharpGapTicks * kSharpTick;
// Address(5) + Command(8) + Expansion(1) + Check(1)
const uint64_t kSharpToggleMask =
    ((uint64_t)1 << (kSharpBits - kSharpAddressBits)) - 1;
const uint64_t kSharpAddressMask = ((uint64_t)1 << kSharpAddressBits) - 1;
const uint64_t kSharpCommandMask = ((uint64_t)1 << kSharpCommandBits) - 1;


uint8_t IRSharpAccalcChecksum(uint8_t state[], const uint16_t length) {
  uint8_t xorsum = xorBytes(state, length - 1, 0);
  xorsum ^= GETBITS8(state[length - 1], kLowNibble, kNibbleSize);
  xorsum ^= GETBITS8(xorsum, kHighNibble, kNibbleSize);
  return GETBITS8(xorsum, kLowNibble, kNibbleSize);
}

bool IRSharpAcvalidChecksum(uint8_t state[], const uint16_t length) {
  return GETBITS8(state[length - 1], kHighNibble, kNibbleSize) ==
      IRSharpAccalcChecksum(state, length);
}

bool decodeSharpAc(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict) {
  // Compliance
  if (strict && nbits != kSharpAcBits) return false;

  // Match Header + Data + Footer
  uint16_t used;
  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, nbits,
                      kSharpAcHdrMark - 200, kSharpAcHdrMark + 200, kSharpAcHdrSpace - 200, kSharpAcHdrSpace + 200,
                      kSharpAcBitMark - 320, kSharpAcBitMark + 430, kSharpAcOneSpace - 400, kSharpAcOneSpace + 300,
                      kSharpAcBitMark - 320, kSharpAcBitMark + 430, kSharpAcZeroSpace - 350, kSharpAcZeroSpace + 350,
                      kSharpAcBitMark - 320, kSharpAcBitMark + 430, kSharpAcGap - 100, kSharpAcGap + 100, true,
                      45, kMarkExcess, false);
  if (used == 0) {
    // os_trace("%s used failed\r\n", __func__);
    return false;
  }
  offset += used;
  // Compliance
  if (strict) {
    if (!IRSharpAcvalidChecksum(results->state, kSharpAcStateLength)) {
        os_trace("%s invalid checksum\r\n", __func__);
        return false;
    }
  }

  // Success
  results->decode_type = SHARP_AC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.

  os_trace("%s success\r\n", __func__);

  return true;
}

static sharp_ac_remote_model_t IRSharpAcgetModel(SharpProtocol _)
{
  if (_.Model2) {
    if (_.Model)
      return A705;
    else
      return A903;
  } else {
    return A907;
  }
}

static bool IRSharpAcgetPower(SharpProtocol _)
{
  switch (_.PowerSpecial) {
    case kSharpAcPowerUnknown:
    case kSharpAcPowerOff: return false;
    default: return true;  // Everything else is "probably" on.
  }
}

static opmode_t IRSharpActoCommonMode(const uint8_t mode, const sharp_ac_remote_model_t model)
{
  switch (mode) {
    case kSharpAcCool: return opmode_kCool;
    case kSharpAcHeat: return opmode_kHeat;
    case kSharpAcDry:  return opmode_kDry;
    case kSharpAcAuto:  // Also kSharpAcFan
      switch (model) {
        case A705: return opmode_kFan;
        default:   return opmode_kAuto;
      }
      break;
    default:           return opmode_kAuto;
  }
}

fanspeed_t IRSharpActoCommonFanSpeed(const uint8_t speed, const sharp_ac_remote_model_t model)
{
  switch (model) {
    case A705:
    case A903:
      switch (speed) {
        case kSharpAcFanA705Low:  return fanspeed_kLow;
        case kSharpAcFanA705Med:  return fanspeed_kMedium;
      }
      // FALL-THRU
    default:
      switch (speed) {
        case kSharpAcFanMax:  return fanspeed_kMax;
        case kSharpAcFanHigh: return fanspeed_kHigh;
        case kSharpAcFanMed:  return fanspeed_kMedium;
        case kSharpAcFanMin:  return fanspeed_kMin;
        default:              return fanspeed_kAuto;
      }
  }
}

void SharpAc_setRaw(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state) 
{
    SharpProtocol _;
    memcpy(_.raw, newState, kSharpAcStateLength);

    ir_state->protocol = SHARP_AC;
    ir_state->model = IRSharpAcgetModel(_);
    ir_state->power = IRSharpAcgetPower(_);
    ir_state->mode = IRSharpActoCommonMode(_.Mode, (sharp_ac_remote_model_t)ir_state->model);
    ir_state->degrees = _.Temp + kSharpAcMinTemp;
    ir_state->fanspeed = IRSharpActoCommonFanSpeed(_.Fan, (sharp_ac_remote_model_t)ir_state->model);
}

void SharpAc_setInfo(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state) 
{
    SharpProtocol _;
    memcpy(_.raw, newState, kSharpAcStateLength);

    ir_state->protocol = SHARP_AC;
    ir_state->model = IRSharpAcgetModel(_);
    ir_state->power = IRSharpAcgetPower(_);
    ir_state->mode = _.Mode;
    ir_state->degrees = _.Temp + kSharpAcMinTemp;
    ir_state->fanspeed = _.Fan;
}

static void IRSharpAcsetPowerSpecial(SharpProtocol *_, const uint8_t value) {
  _->PowerSpecial = value;
}

static void IRSharpAcclearPowerSpecial(SharpProtocol *_) {
  IRSharpAcsetPowerSpecial(_, _->PowerSpecial & kSharpAcPowerOn);
}

static void IRSharpAcsetTemp(SharpProtocol *_, const uint8_t temp) {
  os_trace("%s temp: %u\r\n", __func__, temp);
  switch (_->Mode) {
    // Auto & Dry don't allow temp changes and have a special temp.
    case kSharpAcAuto:
    case kSharpAcDry:
      _->raw[kSharpAcByteTemp] = 0;
      return;
    default:
      switch (IRSharpAcgetModel(*_)) {
        case A705:
          _->raw[kSharpAcByteTemp] = 0xD0;
          break;
        default:
          _->raw[kSharpAcByteTemp] = 0xC0;
      }
  }
  uint8_t degrees = max(temp, kSharpAcMinTemp);
  degrees = min(degrees, kSharpAcMaxTemp);
  _->Temp = degrees - kSharpAcMinTemp;
  _->Special = kSharpAcSpecialTempEcono;
  IRSharpAcclearPowerSpecial(_);
}

static void IRSharpAcsetFan(SharpProtocol *_, const uint8_t speed) {
  switch (speed) {
    case kSharpAcFanAuto:
    case kSharpAcFanMin:
    case kSharpAcFanMed:
    case kSharpAcFanHigh:
    case kSharpAcFanMax:
      _->Fan = speed;
      // if (save) _fan = speed;
      break;
    default:
      _->Fan = kSharpAcFanAuto;
      // _fan = kSharpAcFanAuto;
  }
  _->Special = kSharpAcSpecialFan;
  IRSharpAcclearPowerSpecial(_);
}

static void IRSharpAcsetClean(SharpProtocol *_, const bool on);
static void IRSharpAcsetPower(SharpProtocol *_, const bool on, const bool prev_on) {
  IRSharpAcsetPowerSpecial(_, on ? kSharpAcPowerOnFromOff : kSharpAcPowerOff);
  // Power operations are incompatible with clean mode.
  if (_->Clean) IRSharpAcsetClean(_, false);
  _->Special = kSharpAcSpecialPower;
}

static void IRSharpAcsetMode(SharpProtocol *_, const uint8_t mode, const sharp_ac_remote_model_t model) {
  uint8_t realMode = mode;
  if (mode == kSharpAcHeat) {
    switch (model) {
      case A705:
      case A903:
        // These models have no heat mode, use Fan mode instead.
        realMode = kSharpAcFan;
        break;
      default:
        break;
    }
  }

  switch (realMode) {
    case kSharpAcAuto:  // Also kSharpAcFan
    case kSharpAcDry:
      // When Dry or Auto, Fan always 2(Auto)
      IRSharpAcsetFan(_, kSharpAcFanAuto);
      // FALLTHRU
    case kSharpAcCool:
    case kSharpAcHeat:
      _->Mode = realMode;
      break;
    default:
      IRSharpAcsetFan(_, kSharpAcFanAuto);
      _->Mode = kSharpAcAuto;
  }
  // Dry/Auto have no temp setting. This step will enforce it.
  IRSharpAcsetTemp(_, _->Temp + kSharpAcMinTemp);
  // Save the mode in case we need to revert to it. eg. Clean
  _->Special = kSharpAcSpecialPower;
  IRSharpAcclearPowerSpecial(_);
}

static void IRSharpAcsetClean(SharpProtocol *_, const bool on) {
  // Clean mode appears to be just default dry mode, with an extra bit set.
  if (on) {
    IRSharpAcsetMode(_, kSharpAcDry, IRSharpAcgetModel(*_));
    IRSharpAcsetPower(_, true, false);
  } else {
    // Restore the previous operation mode & fan speed.
    IRSharpAcsetMode(_, _->Mode, IRSharpAcgetModel(*_));
    IRSharpAcsetFan(_, _->Fan);
  }
  _->Clean = on;
  IRSharpAcclearPowerSpecial(_);
}

void sendSharpAc(const unsigned char data[], const uint16_t nbytes,
                         const uint16_t repeat) {
  if (nbytes < kSharpAcStateLength)
    return;  // Not enough bytes to send a proper message.
  vTaskEnterCritical();
  sendGenericBytes(kSharpAcHdrMark, kSharpAcHdrSpace,
              kSharpAcBitMark, kSharpAcOneSpace,
              kSharpAcBitMark, kSharpAcZeroSpace,
              kSharpAcBitMark, kSharpAcGap,
              data, nbytes, 38000, false, repeat, 50);
  vTaskExitCritical();
}

void SharpAc_send(state_ir_t state_ir)
{
  os_trace("%s\r\n", __func__);
  SharpProtocol _;

  uint8_t reset[kSharpAcStateLength] = {0xAA, 0x5A, 0xCF, 0x10, 0x00, 0x01, 0x00, 0x00, 0x08, 0x80, 0x00, 0xE0, 0x01};
  memcpy(_.raw, reset, kSharpAcStateLength);

  sharp_ac_remote_model_t my_model = IRSharpAcgetModel(_);

  IRSharpAcsetMode(&_, state_ir.mode, my_model);
  os_trace("%s set mode finished\r\n", __func__);
  IRSharpAcsetFan(&_, state_ir.fanspeed);
  os_trace("%s set fan finished\r\n", __func__);
  IRSharpAcsetPower(&_, state_ir.power, false);
  os_trace("%s set power finished\r\n", __func__);
  IRSharpAcsetTemp(&_, (uint8_t)state_ir.degrees);
  os_trace("%s set temp finished\r\n", __func__);

  _.Sum = IRSharpAccalcChecksum(_.raw, kSharpAcStateLength);

  // state_ir_t my_state;
  // SharpAc_setRaw(_.raw, SHARP_AC, &my_state);
  // IR_logCommon(my_state);

  sendSharpAc(_.raw, kSharpAcStateLength, kSharpAcDefaultRepeat);
}