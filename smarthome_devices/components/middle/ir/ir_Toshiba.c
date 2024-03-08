#include "ir_Toshiba.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

// Toshiba A/C
#define kToshibaAcHdrMark 4400
#define kToshibaAcHdrSpace 4300
#define kToshibaAcBitMark 540
#define kToshibaAcOneSpace 1600
#define kToshibaAcZeroSpace 541
// Some models have a different inter-message gap.
// See: https://github.com/crankyoldgit/IRremoteESP8266/issues/1420
#define kToshibaAcMinGap 4600    // WH-UB03NJ remote
#define kToshibaAcUsualGap 7400  // Others

static uint8_t calcChecksum(const uint8_t state[],
                                  const uint16_t length) {
  return length ? xorBytes(state, length - 1, 0) : 0;
}

static uint16_t getInternalStateLength(const uint8_t state[],
                                             const uint16_t size) {
  if (size < kToshibaAcLengthByte) return 0;
  return min((uint16_t)(state[kToshibaAcLengthByte] + kToshibaAcMinLength),
                  kToshibaACStateLengthLong);
}

static bool validChecksum(const uint8_t state[], const uint16_t length) {
  return length >= kToshibaAcMinLength &&
         state[length - 1] == calcChecksum(state, length) &&
         checkInvertedBytePairs(state, kToshibaAcInvertedLength) &&
         getInternalStateLength(state, length) == length;
}

bool decodeToshibaAC(decode_results* results, uint16_t offset,
                             const uint16_t nbits, const bool strict) {
  // Compliance
  if (strict) {
    switch (nbits) {  // Must be called with the correct nr. of bits.
      case kToshibaACBits:
      case kToshibaACBitsShort:
      case kToshibaACBitsLong:
        break;
      default:
        return false;
    }
  }

  // Match Header + Data + Footer
  if (!matchGenericState(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    kToshibaAcHdrMark, kToshibaAcHdrSpace,
                    kToshibaAcBitMark, kToshibaAcOneSpace,
                    kToshibaAcBitMark, kToshibaAcZeroSpace,
                    kToshibaAcBitMark, kToshibaAcMinGap, true,
                    40, kMarkExcess, true)) {
                        // os_trace("%s matchGenericState failed\r\n", __func__);
                        return false;
                    }
  // Compliance
  if (strict) {
    // Check that the checksum of the message is correct.
    if (!validChecksum(results->state, nbits / 8)){
        os_trace("===============> %s validChecksum failed\r\n", __func__);
        return false;
    } 
  }

  // Success
  results->decode_type = TOSHIBA_AC;
  results->bits = nbits;

  os_trace("%s success\r\n", __func__);
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

static void sendToshibaAC(const uint8_t data[], const uint16_t nbytes,
                           const uint16_t repeat) {
    vTaskEnterCritical();
    sendGenericBytes(kToshibaAcHdrMark, kToshibaAcHdrSpace, kToshibaAcBitMark,
              kToshibaAcOneSpace, kToshibaAcBitMark, kToshibaAcZeroSpace,
              kToshibaAcBitMark, kToshibaAcUsualGap, data, nbytes, 38, true,
              repeat, 50);
    vTaskExitCritical();
}

static uint16_t getStateLength(ToshibaProtocol _)
{
  return getInternalStateLength(_.raw, kToshibaACStateLengthLong);
}

static opmode_t toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kToshibaAcCool: return opmode_kCool;
    case kToshibaAcHeat: return opmode_kHeat;
    case kToshibaAcDry:  return opmode_kDry;
    case kToshibaAcFan:  return opmode_kFan;
    case kToshibaAcOff:  return opmode_kOff;
    default:             return opmode_kAuto;
  }
}

static fanspeed_t toCommonFanSpeed(const uint8_t spd) {
  switch (spd) {
    case kToshibaAcFanMax:     return fanspeed_kMax;
    case kToshibaAcFanMax - 1: return fanspeed_kHigh;
    case kToshibaAcFanMax - 2: return fanspeed_kMedium;
    case kToshibaAcFanMax - 3: return fanspeed_kLow;
    case kToshibaAcFanMax - 4: return fanspeed_kMin;
    default:                   return fanspeed_kAuto;
  }
}

static bool getPower(ToshibaProtocol _)
{
  return _.Mode != kToshibaAcOff;
}

static uint8_t getTemp(ToshibaProtocol _) { return _.Temp + kToshibaAcMinTemp; }

static bool getTurbo(ToshibaProtocol _)
{
  if (getStateLength(_) == kToshibaACStateLengthLong)
    return _.EcoTurbo == kToshibaAcTurboOn;
  return false;
}

static bool getEcono(ToshibaProtocol _) 
{
  if (getStateLength(_) == kToshibaACStateLengthLong)
    return _.EcoTurbo == kToshibaAcEconoOn;
  return false;
}

static void Toshiba_toCommon(ToshibaProtocol _, state_ir_t *ir_state)
{
    ir_state->protocol = TOSHIBA_AC;
    ir_state->model = -1;

    ir_state->power = getPower(_);
    ir_state->mode = toCommonMode(_.Mode);
    ir_state->degrees = getTemp(_);
    ir_state->fanspeed = toCommonFanSpeed(_.Fan);
    ir_state->turbo = getTurbo(_);
    ir_state->econo = getEcono(_);

}

void Toshiba_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
    ToshibaProtocol _;
    memset(&_, 0, sizeof(ToshibaProtocol));
    memcpy(_.raw, newState, length);
    Toshiba_toCommon(_, ir_state);
}

void Toshiba_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
    ToshibaProtocol _;
    memset(&_, 0, sizeof(ToshibaProtocol));
    memcpy(_.raw, newState, length);

    ir_state->protocol = TOSHIBA_AC;
    ir_state->model = -1;

    ir_state->power = getPower(_);
    ir_state->mode = _.Mode;
    ir_state->degrees = getTemp(_);
    ir_state->fanspeed = _.Fan;
    ir_state->turbo = getTurbo(_);
    ir_state->econo = getEcono(_);
}

static void setTemp(ToshibaProtocol *_, const uint8_t degrees) {
  uint8_t temp = max(kToshibaAcMinTemp, degrees);
  temp = min(kToshibaAcMaxTemp, temp);
  _->Temp = temp - kToshibaAcMinTemp;
}

static void setMode(ToshibaProtocol *_, const uint8_t mode) {
  switch (mode) {
    case kToshibaAcAuto:
    case kToshibaAcCool:
    case kToshibaAcDry:
    case kToshibaAcHeat:
    case kToshibaAcFan:
      // FALL-THRU
    case kToshibaAcOff:
      _->Mode = mode;
      break;
    default:
      _->Mode = kToshibaAcAuto;
  }
}

static void setFan(ToshibaProtocol *_, const uint8_t speed) {
  uint8_t fan = speed;
  // Bounds check
  if (fan > kToshibaAcFanMax)
    fan = kToshibaAcFanMax;  // Set the fan to maximum if out of range.
  if (fan > kToshibaAcFanAuto) fan++;
  _->Fan = fan;
}

static void checksum(ToshibaProtocol *_, const uint16_t length) {
  // Stored the checksum value in the last byte.
  if (length >= kToshibaAcMinLength) {
    // Set/clear the short msg bit.
    _->ShortMsg = 0;
    // Set/clear the long msg bit.
    _->LongMsg = 0;
    invertBytePairs(_->raw, kToshibaAcInvertedLength);
    // Always do the Xor checksum LAST!
    _->raw[length - 1] = calcChecksum(_->raw, length);
  }
}

void Toshiba_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);
    ToshibaProtocol _;

    uint8_t kReset[kToshibaACStateLength] = {
      0xF2, 0x0D, 0x03, 0xFC, 0x01};
    memcpy(_.raw, kReset, kToshibaACStateLength);

    setTemp(&_, (uint8_t)state_ir.degrees);
    setMode(&_, (uint8_t) state_ir.mode);
    setFan(&_, (uint8_t)state_ir.fanspeed);

    if (!state_ir.power) {  // Off
        setMode(&_, kToshibaAcOff);
    }
    checksum(&_, getStateLength(_));
    sendToshibaAC(_.raw, kToshibaACStateLength, kToshibaACMinRepeat);
}