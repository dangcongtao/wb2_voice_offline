#include "ir_Mitsubishi.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"
#include "ir_Nagakawa.h"
#include "ir_Tcl.h"

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

        hdrspace_lower = hdrspace - 200;
        hdrspace_upper = hdrspace + 200;
        bitmark_lower = bitmark - 400;
        bitmark_upper = bitmark + 400;
        onespace_lower = onespace - 200;
        onespace_upper = onespace + 500;
        zerospace_lower = zerospace - 225;
        zerospace_upper = zerospace + 350;
    }
#endif  // (DECODE_TCL112AC || DECODE_TEKNOPOINT)
  if (typeguess == UNKNOWN) return false;  // No header matched.
  offset++;

  uint16_t used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                               results->rawlen - offset, nbits,
                               0,  0,// Skip the header as we matched it earlier.
                               hdrspace_lower, hdrspace_upper, bitmark_lower, bitmark_upper, onespace_lower, onespace_upper, bitmark_lower, bitmark_upper, zerospace_lower, zerospace_upper,
                               bitmark_lower, bitmark_upper, gap - 100, gap + 100,
                               true, tolerance, 0, false);
  if (!used) {
    os_trace("%s matchGenericState failed\r\n", __func__);
    return false;
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
  os_trace("%s success %u\r\n", __func__, typeguess);
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

  os_trace("MITSUBISHI112_SEND: 0x ");
  for(uint8_t i = 0; i < 14; i++) {
      os_trace("%02X-", _.raw[i]);
  }
  os_trace("\r\n");

  sendMitsubishi112(_.raw, kMitsubishi112StateLength, kNoRepeat);
}