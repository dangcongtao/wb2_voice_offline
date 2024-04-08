#include "ir_Mitsubishi.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"
#include "ir_Nagakawa.h"
#include "ir_Tcl.h"


// Constants
// Mitsubishi TV
// period time is 1/33000Hz = 30.303 uSeconds (T)
const uint16_t kMitsubishiTick = 30;
const uint16_t kMitsubishiBitMarkTicks = 10;
const uint16_t kMitsubishiBitMark = kMitsubishiBitMarkTicks * kMitsubishiTick;
const uint16_t kMitsubishiOneSpaceTicks = 70;
const uint16_t kMitsubishiOneSpace = kMitsubishiOneSpaceTicks * kMitsubishiTick;
const uint16_t kMitsubishiZeroSpaceTicks = 30;
const uint16_t kMitsubishiZeroSpace =
    kMitsubishiZeroSpaceTicks * kMitsubishiTick;
const uint16_t kMitsubishiMinCommandLengthTicks = 1786;
const uint16_t kMitsubishiMinCommandLength =
    kMitsubishiMinCommandLengthTicks * kMitsubishiTick;
const uint16_t kMitsubishiMinGapTicks = 936;
const uint16_t kMitsubishiMinGap = kMitsubishiMinGapTicks * kMitsubishiTick;

// Mitsubishi Projector (HC3000)
const uint16_t kMitsubishi2HdrMark = 8400;
const uint16_t kMitsubishi2HdrSpace = kMitsubishi2HdrMark / 2;
const uint16_t kMitsubishi2BitMark = 560;
const uint16_t kMitsubishi2ZeroSpace = 520;
const uint16_t kMitsubishi2OneSpace = kMitsubishi2ZeroSpace * 3;
const uint16_t kMitsubishi2MinGap = 28500;

// Mitsubishi A/C
const uint16_t kMitsubishiAcHdrMark = 3400;
const uint16_t kMitsubishiAcHdrSpace = 1750;
const uint16_t kMitsubishiAcBitMark = 450;
const uint16_t kMitsubishiAcOneSpace = 1300;
const uint16_t kMitsubishiAcZeroSpace = 420;
const uint16_t kMitsubishiAcRptMark = 440;
const uint16_t kMitsubishiAcRptSpace = 15500;
const uint8_t  kMitsubishiAcExtraTolerance = 5;

// Mitsubishi 136 bit A/C
const uint16_t kMitsubishi136HdrMark = 3324;
const uint16_t kMitsubishi136HdrSpace = 1474;
const uint16_t kMitsubishi136BitMark = 467;
const uint16_t kMitsubishi136OneSpace = 1137;
const uint16_t kMitsubishi136ZeroSpace = 351;
const uint32_t kMitsubishi136Gap = kDefaultMessageGap;

// Mitsubishi 112 bit A/C
const uint16_t kMitsubishi112HdrMark = 3450;
const uint16_t kMitsubishi112HdrSpace = 1696;
const uint16_t kMitsubishi112BitMark = 450;
const uint16_t kMitsubishi112OneSpace = 1250;
const uint16_t kMitsubishi112ZeroSpace = 385;
const uint32_t kMitsubishi112Gap = kDefaultMessageGap;
// Total tolerance percentage to use for matching the header mark.
const uint8_t  kMitsubishi112HdrMarkTolerance = 5;

static void sendMitsubishi112(const unsigned char data[],
                               const uint16_t nbytes,
                               const uint16_t repeat) {
  if (nbytes < kMitsubishi112StateLength)
    return;  // Not enough bytes to send a proper message.
  vTaskEnterCritical();
  sendGenericBytes(kMitsubishi112HdrMark, kMitsubishi112HdrSpace,
              kMitsubishi112BitMark, kMitsubishi112OneSpace,
              kMitsubishi112BitMark, kMitsubishi112ZeroSpace,
              kMitsubishi112BitMark, kMitsubishi112Gap,
              data, nbytes, 38, false, repeat, 50);
  vTaskExitCritical();
}

uint8_t Mitsubishi112calcChecksum(uint8_t state[], const uint16_t length) {
  if (length) {
    if (length > 4 && state[3] == 0x02) {  // Special nessage?
      return sumBytesState(state, length - 1, 0xF);  // Checksum needs an offset.
    } else {
      return sumBytesState(state, length - 1, 0);
    }
  } else {
    return 0;
  }
}

static bool Mitsubishi112validChecksum(uint8_t state[], const uint16_t length) {
  return (length > 1 && state[length - 1] == Mitsubishi112calcChecksum(state, length));
}

bool decodeMitsubishi112(decode_results *results, uint16_t offset,
                                 const uint16_t nbits, const bool strict) {
  if (results->rawlen < (2 * nbits) + kHeader + kFooter - 1 + offset)
    return false;
  if (nbits % 8 != 0) return false;  // Not a multiple of an 8 bit byte.
  if (strict) {  // Do checks to see if it matches the spec.
    if (nbits != kMitsubishi112Bits
    //  && nbits != kTcl112AcBits
     ) return false;
  }
  decode_type_t typeguess = UNKNOWN;
  uint16_t hdrspace;
  uint16_t bitmark;
  uint16_t onespace;
  uint16_t zerospace;
  uint32_t gap;
  uint8_t tolerance = _tolerance;

  uint16_t hdrspace_lower;
  uint16_t hdrspace_upper;
  uint16_t bitmark_lower;
  uint16_t bitmark_upper;
  uint16_t onespace_lower;
  uint16_t onespace_upper;
  uint16_t zerospace_lower;
  uint16_t zerospace_upper;

  // Header
#if DECODE_MITSUBISHI112
  if (matchLimit(results->rawbuf[offset], kMitsubishi112HdrMark - 100, kMitsubishi112HdrMark + 80)) {
        typeguess = MITSUBISHI112;
        hdrspace = kMitsubishi112HdrSpace;
        bitmark = kMitsubishi112BitMark;
        onespace = kMitsubishi112OneSpace;
        zerospace = kMitsubishi112ZeroSpace;
        gap = kMitsubishi112Gap;
        hdrspace_lower = hdrspace - 200;
        hdrspace_upper = hdrspace + 200;
        bitmark_lower = bitmark - 340;
        bitmark_upper = bitmark + 340;
        onespace_lower = onespace - 350;
        onespace_upper = onespace + 500;
        zerospace_lower = zerospace - 300;
        zerospace_upper = zerospace + 400;
        os_trace("%s guess mitsu112\r\n", __func__);
  }
#endif  // DECODE_MITSUBISHI112
#if DECODE_NAGAKAWA
    if (typeguess == UNKNOWN && matchLimit(results->rawbuf[offset], kNagakawaHdrMark - 90, kNagakawaHdrMark + 100)) {
        os_trace("%s guess nagakawa\r\n", __func__);
        typeguess = NAGAKAWA;
        hdrspace = kNagakawaHdrSpace;
        bitmark = kNagakawaBitMark;
        onespace = kNagakawaOneSpace;
        zerospace = kNagakawaZeroSpace;
        gap = kNagakawaGap;

        hdrspace_lower = hdrspace - 200;
        hdrspace_upper = hdrspace + 200;
        bitmark_lower = bitmark - 340;
        bitmark_upper = bitmark + 340;
        onespace_lower = onespace - 350;
        onespace_upper = onespace + 500;
        zerospace_lower = zerospace - 300;
        zerospace_upper = zerospace + 400;
    }
#endif
#if (DECODE_TCL112AC || DECODE_TEKNOPOINT)
  if (typeguess == UNKNOWN &&  // We didn't match Mitsubishi112
      matchLimit(results->rawbuf[offset], kTcl112AcHdrMark - 100, kTcl112AcHdrMark + 150)) 
    {
        os_trace("%s guess tcl112ac\r\n", __func__);
        typeguess = TCL112AC;
        hdrspace = kTcl112AcHdrSpace;
        bitmark = kTcl112AcBitMark;
        onespace = kTcl112AcOneSpace;
        zerospace = kTcl112AcZeroSpace;
        gap = kTcl112AcGap;
        tolerance += kTcl112AcTolerance;

        hdrspace_lower = hdrspace - 400;
        hdrspace_upper = hdrspace + 400;
        bitmark_lower = bitmark - 400;
        bitmark_upper = bitmark + 400;
        onespace_lower = onespace - 350;
        onespace_upper = onespace + 400;
        zerospace_lower = zerospace - 225;
        zerospace_upper = zerospace + 300;
    }
#endif  // (DECODE_TCL112AC || DECODE_TEKNOPOINT)
  if (typeguess == UNKNOWN) return false;  // No header matched.
  offset++;

  if (typeguess == TCL112AC && results->rawlen >= 452 && results->rawlen <= 456) {
    offset += kTcl112AcBits * 2 + 4;
    uint16_t used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                                results->rawlen - offset, nbits,
                                0,  0,// Skip the header as we matched it earlier.
                                hdrspace_lower, hdrspace_upper, bitmark_lower, bitmark_upper, onespace_lower, onespace_upper, bitmark_lower, bitmark_upper, zerospace_lower, zerospace_upper,
                                bitmark_lower, bitmark_upper, gap - 35000, gap + 25000,
                                true, tolerance, 0, false);
    if (!used) {
      os_trace("%s tcl repeat 1 matchGenericState failed\r\n", __func__);
      return false;
    }
  } else {
    uint16_t used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                                results->rawlen - offset, nbits,
                                0,  0,// Skip the header as we matched it earlier.
                                hdrspace_lower, hdrspace_upper, bitmark_lower, bitmark_upper, onespace_lower, onespace_upper, bitmark_lower, bitmark_upper, zerospace_lower, zerospace_upper,
                                bitmark_lower, bitmark_upper, gap - 35000, gap + 25000,
                                true, tolerance, 0, false);
    if (!used) {
      os_trace("%s matchGenericState failed\r\n", __func__);
      return false;
    }
  }
  if (strict) {
    // Header validation: Codes start with 0x23CB26
    if (results->state[0] != 0x23 || results->state[1] != 0xCB ||
        results->state[2] != 0x26) return false;
    // TCL112 and MITSUBISHI112 share the exact same checksum.
    if (!Mitsubishi112validChecksum(results->state, nbits / 8)) {
        os_trace("%s invalid checksum\r\n", __func__);
        return false;
    }
  }
  // Success
  results->decode_type = typeguess;
  results->bits = nbits;
  os_trace("%s success, typeguess: %u\r\n", __func__, typeguess);
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRMitsubishi112checksum(Mitsubishi112Protocol *_) 
{
  _->Sum = Mitsubishi112calcChecksum(_->raw, kMitsubishi112StateLength);
}

void Mitsubishi112stateReset(Mitsubishi112Protocol *_) {
  const uint8_t kReset[kMitsubishi112StateLength] = {
      0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03, 0x0B, 0x10,
      0x00, 0x00, 0x00, 0x30};
  memcpy(_->raw, kReset, kMitsubishi112StateLength);
}

static opmode_t Mitsubishi112toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kMitsubishi112Cool: return opmode_kCool;
    case kMitsubishi112Heat: return opmode_kHeat;
    case kMitsubishi112Dry:  return opmode_kDry;
    default:                return opmode_kAuto;
  }
}

fanspeed_t Mitsubishi112toCommonFanSpeed(const uint8_t speed) {
  switch (speed) {
    case kMitsubishi112FanMax: return fanspeed_kMax;
    case kMitsubishi112FanMed: return fanspeed_kMedium;
    case kMitsubishi112FanLow: return fanspeed_kLow;
    case kMitsubishi112FanMin: return fanspeed_kMin;
    default: return fanspeed_kMedium;
  }
}

void Mitsubishi112_setRaw(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state) 
{
    Mitsubishi112Protocol _;

    memcpy(_.raw, newState, kMitsubishi112StateLength);

    ir_state->protocol = MITSUBISHI112;
  // We can only detect the difference between models when the power is on.

    ir_state->power = _.Power;

    ir_state->mode = Mitsubishi112toCommonMode(_.Mode);
    ir_state->degrees = kMitsubishiAcMaxTemp - _.Temp;
    ir_state->fanspeed = Mitsubishi112toCommonFanSpeed(_.Fan);

    // Toshiba_toCommon(_, ir_state);
}

void Mitsubishi112_setInfo(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state) 
{
    Mitsubishi112Protocol _;

    memcpy(_.raw, newState, kMitsubishi112StateLength);

    ir_state->protocol = MITSUBISHI112;
  // We can only detect the difference between models when the power is on.

    ir_state->power = _.Power;

    ir_state->mode = _.Mode;
    ir_state->degrees = kMitsubishiAcMaxTemp - _.Temp;
    ir_state->fanspeed = _.Fan;
    ir_state->swingv = _.SwingV;
    ir_state->swingh = _.SwingH;

    // Toshiba_toCommon(_, ir_state);
}

static void IRMitsubishi112stateReset(Mitsubishi112Protocol *_) {
  const uint8_t kReset[kMitsubishi112StateLength] = {
      0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03, 0x0B, 0x10, 0x00, 0x00, 0x00, 0x30};
  memcpy(_->raw, kReset, kMitsubishi112StateLength);
}

static void IRMitsubishi112setTemp(Mitsubishi112Protocol *_, const uint8_t degrees) {
  uint8_t temp = max((uint8_t)kMitsubishi112MinTemp, degrees);
  temp = min((uint8_t)kMitsubishi112MaxTemp, temp);
  _->Temp = kMitsubishiAcMaxTemp - temp;
}


void Mitsubishi112_send(state_ir_t state_ir)
{
  os_trace("%s\r\n", __func__);
  Mitsubishi112Protocol _;
  IRMitsubishi112stateReset(&_);
  _.SwingV = kMitsubishi112SwingVAuto; // swingv auto

  _.Mode = (uint8_t)state_ir.mode;

  _.Fan = (uint8_t)state_ir.fanspeed;

  IRMitsubishi112setTemp(&_, (uint8_t)state_ir.degrees);

  _.Power = state_ir.power;

  IRMitsubishi112checksum(&_);

  // os_trace("MITSUBISHI112_SEND: 0x ");
  // for(uint8_t i = 0; i < 14; i++) {
  //     os_trace("%02X-", _.raw[i]);
  // }
  // os_trace("\r\n");

  sendMitsubishi112(_.raw, kMitsubishi112StateLength, kNoRepeat);
}

static uint8_t IRMitsubishiAC_calculateChecksum(const uint8_t *data) {
  return sumBytesState(data, kMitsubishiACStateLength - 1, 0);
}

static bool IRMitsubishiAC_validChecksum(const uint8_t *data) {
  return (IRMitsubishiAC_calculateChecksum(data) == data[kMitsubishiACStateLength - 1]);
}

bool decodeMitsubishiAC(decode_results *results, uint16_t offset,
                                const uint16_t nbits,
                                const bool strict) {
  // Compliance
  if (strict && nbits != kMitsubishiACBits) return false;  // Out of spec.
  // Do we need to look for a repeat?
  const uint16_t expected_repeats = strict ? kMitsubishiACMinRepeat : kNoRepeat;
  // Enough data?
  if (results->rawlen <= (nbits * 2 + kHeader + kFooter) *
                         (expected_repeats + 1) + offset - 1) return false;
  uint16_t save[kStateSizeMax];
  // Handle repeats if we need too.
  for (uint16_t r = 0; r <= expected_repeats; r++) {
    // Header + Data + Footer
    uint16_t used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                                 results->rawlen - offset, nbits,
                                 kMitsubishiAcHdrMark-400, kMitsubishiAcHdrMark+400, kMitsubishiAcHdrSpace-350, kMitsubishiAcHdrSpace+350,
                                 kMitsubishiAcBitMark-350, kMitsubishiAcBitMark+350, kMitsubishiAcOneSpace-400, kMitsubishiAcOneSpace+400,
                                 kMitsubishiAcBitMark-350, kMitsubishiAcBitMark+350, kMitsubishiAcZeroSpace-320, kMitsubishiAcZeroSpace+380,
                                 kMitsubishiAcRptMark-340, kMitsubishiAcRptMark+360, kMitsubishiAcRptSpace-4000, kMitsubishiAcRptSpace+4000,
                                 r < expected_repeats,  // At least?
                                 _tolerance + kMitsubishiAcExtraTolerance,
                                 0, false);
    if (!used){
      // os_trace("%s not match\r\n", __func__);
      return false;  // No match.
    }
    offset += used;
    if (r) {  // Is this a repeat?
      // Repeats are expected to be exactly the same.
      if (memcmp(save, results->state, nbits / 8) != 0){
        os_trace("%s Repeats are expected to be exactly the same\r\n", __func__);
        return false;
      }
    } else {  // It is the first message.
      // Compliance
      if (strict) {
        // Data signature check.
        static const uint8_t signature[5] = {0x23, 0xCB, 0x26, 0x01, 0x00};
        if (memcmp(results->state, signature, 5) != 0) {
          os_trace("%s Data signature check invalid!\r\n", __func__);
          return false;
        }
        // Checksum verification.
        if (!IRMitsubishiAC_validChecksum(results->state)) {
          os_trace("%s Checksum verification invalid!\r\n", __func__);
          return false;
        }
      }
      // Save a copy of the state to compare with.
      memcpy(save, results->state, nbits / 8);
    }
  }
  os_trace("%s success\r\n", __func__);
  // Success.
  results->decode_type = MITSUBISHI_AC;
  results->bits = nbits;
  return true;
}

void IRMitsubishiAC_setInfo(const uint8_t *data, state_ir_t *ir_state) {
  Mitsubishi144Protocol _;
  memset(&_, 0, sizeof(Mitsubishi144Protocol));
  memcpy(_.raw, data, kMitsubishiACStateLength);

  ir_state->power = _.Power;
  ir_state->mode = _.Mode;

  uint8_t fan = _.Fan;
  if (fan == kMitsubishiAcFanMax)
    fan = kMitsubishiAcFanSilent;
  ir_state->fanspeed = fan;

  ir_state->degrees = _.Temp + kMitsubishiAcMinTemp + (_.HalfDegree ? 0.5 : 0);
}

static void sendMitsubishiAC(const unsigned char data[], const uint16_t nbytes,
                              const uint16_t repeat) {
  if (nbytes < kMitsubishiACStateLength)
    return;  // Not enough bytes to send a proper message.
  vTaskEnterCritical();
  sendGenericBytes(kMitsubishiAcHdrMark, kMitsubishiAcHdrSpace, kMitsubishiAcBitMark,
              kMitsubishiAcOneSpace, kMitsubishiAcBitMark,
              kMitsubishiAcZeroSpace, kMitsubishiAcRptMark,
              kMitsubishiAcRptSpace, data, nbytes, 38, false, repeat, 50);
  vTaskExitCritical();
}

void MitsubishiAC_send(state_ir_t state_ir)
{
  Mitsubishi144Protocol _;
  uint8_t kReset[kMitsubishiACStateLength] = {
      0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x08, 0x06, 0x30, 0x45, 0x67};

  memset(&_, 0, sizeof(Mitsubishi144Protocol));
  memcpy(_.raw, kReset, kMitsubishiACStateLength);

  _.Power = state_ir.power;

  switch ((uint8_t)state_ir.mode) {
    case kMitsubishiAcAuto: _.raw[8] = 0b00110000; break;
    case kMitsubishiAcCool: _.raw[8] = 0b00110110; break;
    case kMitsubishiAcDry:  _.raw[8] = 0b00110010; break;
    case kMitsubishiAcHeat: _.raw[8] = 0b00110000; break;
    case kMitsubishiAcFan:  _.raw[8] = 0b00110111; break;
    default:
      _.raw[8] = 0b00110000;
      _.Mode = kMitsubishiAcAuto;
      return;
  }
  _.Mode = state_ir.mode;
  // iSave10C can only be on in Heat mode.
  if (state_ir.mode != kMitsubishiAcHeat) {
      _.iSave10C = false;
  }

  uint8_t fan = state_ir.fanspeed;
  // Bounds check
  if (fan > kMitsubishiAcFanSilent)
    fan = kMitsubishiAcFanMax;        // Set the fan to maximum if out of range.
  // Auto has a special bit.
  _.FanAuto = (fan == kMitsubishiAcFanAuto);
  if (fan >= kMitsubishiAcFanMax)
    fan--;  // There is no spoon^H^H^Heed 5 (max), pretend it doesn't exist.
  _.Fan = fan;

  float celsius = max(state_ir.degrees, kMitsubishiAcMinTemp);
  celsius = min(celsius, kMitsubishiAcMaxTemp);
  // Convert to integer nr. of half degrees.
  uint8_t nrHalfDegrees = celsius * 2;
  // Do we have a half degree celsius?
  _.HalfDegree = nrHalfDegrees & 1;
  _.Temp = (uint8_t)(nrHalfDegrees / 2 - kMitsubishiAcMinTemp);

  _.Sum = IRMitsubishiAC_calculateChecksum(_.raw);

  sendMitsubishiAC(_.raw, kMitsubishiACStateLength, kMitsubishiACMinRepeat);
}

static bool IRMitsubishi136_validChecksum(const uint8_t *data, const uint16_t len) {
  if (len < kMitsubishi136StateLength) return false;
  const uint16_t half = (len - kMitsubishi136PowerByte) / 2;
  for (uint8_t i = 0; i < half; i++) {
    // This variable is needed to avoid the warning: (known compiler issue)
    // warning: comparison of promoted ~unsigned with unsigned [-Wsign-compare]
    const uint8_t inverted = ~data[kMitsubishi136PowerByte + half + i];
    if (data[kMitsubishi136PowerByte + i] != inverted) return false;
  }
  return true;
}

bool decodeMitsubishi136(decode_results *results, uint16_t offset,
                                 const uint16_t nbits,
                                 const bool strict) {
  if (nbits % 8 != 0) return false;  // Not a multiple of an 8 bit byte.
  if (strict) {  // Do checks to see if it matches the spec.
    if (nbits != kMitsubishi136Bits) return false;
  }
  uint16_t used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                               results->rawlen - offset, nbits,
                               kMitsubishi136HdrMark-500, kMitsubishi136HdrMark+500, kMitsubishi136HdrSpace-474, kMitsubishi136HdrSpace+476,
                               kMitsubishi136BitMark-367, kMitsubishi136BitMark+373, kMitsubishi136OneSpace-437, kMitsubishi136OneSpace+433,
                               kMitsubishi136BitMark-367, kMitsubishi136BitMark+373, kMitsubishi136ZeroSpace-251, kMitsubishi136ZeroSpace+351,
                               kMitsubishi136BitMark-367, kMitsubishi136BitMark+373, kMitsubishi136Gap-25000, kMitsubishi136Gap+25000,
                               true, _tolerance, 0, false);
  if (!used) return false;
  if (strict) {
    // Header validation: Codes start with 0x23CB26
    if (results->state[0] != 0x23 || results->state[1] != 0xCB ||
        results->state[2] != 0x26) return false;
    if (!IRMitsubishi136_validChecksum(results->state, nbits / 8)) {
       os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }
  os_trace("%s success\r\n", __func__);
  results->decode_type = MITSUBISHI136;
  results->bits = nbits;
  return true;
}

void IRMitsubishi136_setInfo(const uint8_t *data, state_ir_t *ir_state) {
  Mitsubishi136Protocol _;
  memset(&_, 0, sizeof(Mitsubishi136Protocol));
  memcpy(_.raw, data, kMitsubishi136StateLength);

  ir_state->power = _.Power;
  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;
  ir_state->degrees = _.Temp + kMitsubishiAcMinTemp;
  ir_state->swingv = _.SwingV;
}

void sendMitsubishi136(const unsigned char data[],
                               const uint16_t nbytes,
                               const uint16_t repeat) {
  if (nbytes < kMitsubishi136StateLength)
    return;  // Not enough bytes to send a proper message.
  vTaskEnterCritical();  
  sendGenericBytes(kMitsubishi136HdrMark, kMitsubishi136HdrSpace,
              kMitsubishi136BitMark, kMitsubishi136OneSpace,
              kMitsubishi136BitMark, kMitsubishi136ZeroSpace,
              kMitsubishi136BitMark, kMitsubishi136Gap,
              data, nbytes, 38, false, repeat, 50);
  vTaskExitCritical();
}

void Mitsubishi136_send(state_ir_t state_ir)
{
  Mitsubishi136Protocol _;
  memset(&_, 0, sizeof(Mitsubishi136Protocol));

  uint8_t kReset[kMitsubishi136StateLength] = {
      0x23, 0xCB, 0x26, 0x21, 0x00, 0x40, 0xC2, 0xC7, 0x04};
  memcpy(_.raw, kReset, kMitsubishi136StateLength);

  _.Power = state_ir.power;

  switch ((uint8_t)state_ir.mode) {
    case kMitsubishi136Fan:
    case kMitsubishi136Cool:
    case kMitsubishi136Heat:
    case kMitsubishi136Auto:
    case kMitsubishi136Dry:
      _.Mode = state_ir.mode;
      break;
    default:
      _.Mode = kMitsubishi136Auto;
  }

  _.Fan = min((uint8_t)state_ir.fanspeed, kMitsubishi136FanMax);

  uint8_t temp = max((uint8_t)kMitsubishi136MinTemp, (uint8_t)state_ir.degrees);
  temp = min((uint8_t)kMitsubishi136MaxTemp, temp);
  _.Temp = temp - kMitsubishiAcMinTemp;

  // checksum
  for (uint8_t i = 0; i < 6; i++)
    _.raw[kMitsubishi136PowerByte + 6 + i] = ~_.raw[kMitsubishi136PowerByte + i];
  
  sendMitsubishi136(_.raw, kMitsubishi136StateLength, kMitsubishi136MinRepeat);
}
