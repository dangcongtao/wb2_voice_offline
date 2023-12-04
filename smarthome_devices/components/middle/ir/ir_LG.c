#include "ir_LG.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

const uint16_t kLgBitMark = 550;              ///< uSeconds.
const uint16_t kLgOneSpace = 1600;            ///< uSeconds.
const uint16_t kLgZeroSpace = 550;            ///< uSeconds.
const uint16_t kLgRptSpace = 2250;            ///< uSeconds.
const uint16_t kLgMinGap = 39750;             ///< uSeconds.
const uint32_t kLgMinMessageLength = 108050;  ///< uSeconds.
// LG (28 Bit)
const uint16_t kLgHdrMark = 8500;             ///< uSeconds.
const uint16_t kLgHdrSpace = 4250;            ///< uSeconds.
// LG (32 Bit)
const uint16_t kLg32HdrMark = 4500;           ///< uSeconds.
const uint16_t kLg32HdrSpace = 4450;          ///< uSeconds.
const uint16_t kLg32RptHdrMark = 8950;        ///< uSeconds.
// LG2 (28 Bit)
const uint16_t kLg2HdrMark = 3200;            ///< uSeconds.
const uint16_t kLg2HdrSpace = 9900;           ///< uSeconds.
const uint16_t kLg2BitMark = 480; 

const uint32_t kLgAcAKB74955603DetectionMask = 0x0000080;
const uint8_t  kLgAcChecksumSize = 4;  ///< Size in bits.
// Signature has the checksum removed, and another bit to match both Auto & Off.
const uint8_t  kLgAcSwingHOffsetSize = kLgAcChecksumSize + 1;
const uint32_t kLgAcSwingHSignature  = kLgAcSwingHOff >> kLgAcSwingHOffsetSize;
const uint32_t kLgAcVaneSwingVBase = 0x8813200;

#ifdef VANESWINGVPOS
#undef VANESWINGVPOS
#endif
#define VANESWINGVPOS(code) (code % kLgAcVaneSwingVSize)

bool decodeLG(decode_results *results, uint16_t offset,
                      const uint16_t nbits, const bool strict) {
  
  if (nbits >= kLg32Bits) {
    if (results->rawlen <= 2 * nbits + 2 * (kHeader + kFooter) - 1 + offset){
      // os_trace("Can't possibly be a valid LG32 message %d %d\r\n", results->rawlen, 2 * nbits + 2 * (kHeader + kFooter) - 1 + offset);
      return false;  // Can't possibly be a valid LG32 message.
    }
  } else {
    if (results->rawlen <= 2 * nbits + kHeader - 1 + offset){
      // os_trace("Can't possibly be a valid LG message %d %d\r\n", results->rawlen, 2 * nbits + kHeader - 1 + offset);
      return false;  // Can't possibly be a valid LG message.
    }
  }
  // Compliance
  if (strict && nbits != kLgBits && nbits != kLg32Bits){
    // os_trace("Doesn't comply with expected LG protocol\r\n");
    return false;  // Doesn't comply with expected LG protocol.
  }

  // Header (Mark)
  uint32_t kHdrSpace;
  if (matchMark(results->rawbuf[offset], kLgHdrMark, kUseDefTol, kMarkExcess))
    kHdrSpace = kLgHdrSpace;
  else if (matchMark(results->rawbuf[offset], kLg2HdrMark, kUseDefTol, kMarkExcess))
    kHdrSpace = kLg2HdrSpace;
  else if (matchMark(results->rawbuf[offset], kLg32HdrMark, kUseDefTol, kMarkExcess))
    kHdrSpace = kLg32HdrSpace;
  else {
    // os_trace("kHdrSpace invalid\r\n");
    return false;
  }
  offset++;

  // Set up the expected data section values.
  const uint16_t kBitmark = (kHdrSpace == kLg2HdrSpace) ? kLg2BitMark
                                                        : kLgBitMark;
  // Header Space + Data + Footer
  uint64_t data = 0;
  uint16_t used = matchGeneric(results->rawbuf + offset, &data,
                               results->rawlen - offset, nbits,
                               0,  // Already matched the Header mark.
                               kHdrSpace,
                               kBitmark, kLgOneSpace, kBitmark, kLgZeroSpace,
                               kBitmark, kLgMinGap, true, kUseDefTol, 0, true);
  if (!used){ 
    // os_trace("%s used not match generic\r\n", __func__);
    return false;
  }
  offset += used;

  // Repeat
  if (nbits >= kLg32Bits) {
    // If we are expecting the LG 32-bit protocol, there is always
    // a repeat message. So, check for it.
    uint64_t unused;
    if (!matchGeneric(results->rawbuf + offset, &unused,
                      results->rawlen - offset, 0,  // No Data bits to match.
                      kLg32RptHdrMark, kLgRptSpace,
                      kBitmark, kLgOneSpace, kBitmark, kLgZeroSpace,
                      kBitmark, kLgMinGap, true, kUseDefTol, kMarkExcess, true)){
                        os_trace("not match generic LG 3\r\n");
                        return false;
                      }
  }

  // The 16 bits before the checksum.
  uint16_t command = (data >> kLgAcChecksumSize);

  // Compliance
  if (strict && (data & 0xF) != sumNibbles(command, 4, 0, true)) {
    os_trace("sumNibbles wrong\r\n");
    return false;  // The last 4 bits sent are the expected checksum.
  }
  // Success
  if (kHdrSpace == kLg2HdrSpace)  // Was it an LG2 message?
    results->decode_type = LG2;
  else
    results->decode_type = LG;
  results->bits = nbits;
  results->value = data;
  results->command = command;
  results->address = data >> 20;  // The bits before the command.
  os_trace("LG success\r\n");
  return true;
}

LGProtocol _;
uint8_t _temp;
bool _light;
uint32_t _swingv;
uint32_t _swingv_prev;
uint8_t _vaneswingv[kLgAcSwingVMaxVanes];
uint8_t _vaneswingv_prev[kLgAcSwingVMaxVanes];
bool _swingh;
bool _swingh_prev;
decode_type_t _protocol;  ///< Protocol version
lg_ac_remote_model_t _model;  ///< Model type

bool isSwingVToggle(void) { return _.data.raw == kLgAcSwingVToggle; }

bool isSwing(void) {
  return ((_.data.raw >> 12) == kLgAcSwingSignature) || isSwingVToggle();
}

bool isLightToggle(void) { return _.data.raw == kLgAcLightToggle; }

bool isSwingV(void) {
  const uint32_t code = _.data.raw >> kLgAcChecksumSize;
  return (code >= (kLgAcSwingVLowest >> kLgAcChecksumSize) &&
          code < (kLgAcSwingHAuto >> kLgAcChecksumSize)) || isSwingVToggle();
}

bool isSwingH(void) {
  return (_.data.raw >> kLgAcSwingHOffsetSize) == kLgAcSwingHSignature;
}

bool isVaneSwingV(void) {
  return _.data.raw > kLgAcVaneSwingVBase &&
      _.data.raw < (kLgAcVaneSwingVBase +
               ((kLgAcSwingVMaxVanes *
                 kLgAcVaneSwingVSize) << kLgAcChecksumSize));
}

bool _isNormal(void) {
  switch (_.data.raw) {
    case kLgAcOffCommand:
    case kLgAcLightToggle:
      return false;
  }
  if (isSwing()) return false;
  return true;
}

bool _isAKB74955603(void) {
  return ((_.data.raw & kLgAcAKB74955603DetectionMask) && _isNormal()) ||
      (isSwingV() && !isSwingVToggle()) || isLightToggle();
}

bool _isAKB73757604(void) { return isSwingH() || isVaneSwingV(); }

uint8_t getVaneCode(const uint32_t raw) {
  return (raw - kLgAcVaneSwingVBase) >> kLgAcChecksumSize;
}

uint8_t getTemp(void) {
  return _isNormal() ? _.data.Temp + kLgAcTempAdjust : _temp;
}

void setModel(const lg_ac_remote_model_t model) {
  switch (model) {
    case AKB75215403:
    case AKB74955603:
    case AKB73757604:
      _protocol = LG2;
      break;
    case GE6711AR2853M:
    case LG6711A20083V:
      _protocol = LG;
      break;
    default:
      return;
  }
  _model = model;
}

void LG_setRaw(const uint32_t new_code, const decode_type_t protocol) 
{
  _.data.raw = new_code;
  // Set the default model for this protocol, if the protocol is supplied.
  switch (protocol) {
    case LG:
      if (isSwingVToggle())  // This model uses a swingv toggle message.
        setModel(LG6711A20083V);
      else  // Assume others are a different model.
        setModel(GE6711AR2853M);
      break;
    case LG2:
      setModel(AKB75215403);
      break;
    default:
      // Don't change anything if it isn't an expected protocol.
      break;
  }
  // Look for model specific settings/features to improve model detection.
  if (_isAKB74955603()) {
    setModel(AKB74955603);
    if (isSwingV()) _swingv = new_code;
  }
  if (_isAKB73757604()) {
    setModel(AKB73757604);
    if (isVaneSwingV()) {
      // Extract just the vane nr and position part of the message.
      const uint32_t vanecode = getVaneCode(_.data.raw);
      _vaneswingv[vanecode / kLgAcVaneSwingVSize] = VANESWINGVPOS(vanecode);
    } else if (isSwingH()) {
      _swingh = (_.data.raw == kLgAcSwingHAuto);
    }
  }
  _temp = 15;  // Ensure there is a "sane" previous temp.
  _temp = getTemp();
}

uint8_t getPower(void) {
  return _.data.Power == kLgAcPowerOn;
}

void lg_getString(void)
{
  os_trace("power: %u, mode: %u, temp: %u, fan: %u\r\n", getPower(), _.data.Mode, getTemp(), _.data.Fan);
}

uint8_t calcChecksum(const uint32_t state) {
  return sumNibbles(state >> kLgAcChecksumSize, 4, 0, true);
}

static bool validChecksum(const uint32_t state) {
  LGProtocol LGp;
  LGp.data.raw = state;
  return calcChecksum(state) == LGp.data.Sum;
}

bool isValidLgAc(void)
{
  return validChecksum(_.data.raw) && (_.data.Sign == kLgAcSignature);
}

