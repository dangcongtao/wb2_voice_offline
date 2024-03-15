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
  os_trace("%s success %u\r\n", __func__, results->decode_type);
  return true;
}

void sendLG(uint64_t data, uint16_t nbits, uint16_t repeat) {
  uint16_t repeatHeaderMark = 0;
  uint8_t duty = kDutyDefault;
  vTaskEnterCritical();
  // if (nbits >= kLg32Bits) {
  //   // LG 32bit protocol is near identical to Samsung except for repeats.
  //   sendSAMSUNG(data, nbits, 0);  // Send it as a single Samsung message.
  //   repeatHeaderMark = kLg32RptHdrMark;
  //   duty = 33;
  //   repeat++;
  // } else {
    // LG (28-bit) protocol.
    repeatHeaderMark = kLgHdrMark;
    sendGeneric1(kLgHdrMark, kLgHdrSpace, kLgBitMark, kLgOneSpace, kLgBitMark,
                kLgZeroSpace, kLgBitMark, kLgMinGap, kLgMinMessageLength, data,
                nbits, 38, true, 0,  // Repeats are handled later.
                duty);
  // }

  // Repeat
  // Protocol has a mandatory repeat-specific code sent after every command.
  if (repeat)
    sendGeneric1(repeatHeaderMark, kLgRptSpace, 0, 0, 0, 0,  // No data is sent.
                kLgBitMark, kLgMinGap, kLgMinMessageLength, 0, 0,  // No data.
                38, true, repeat - 1, duty);
  
  vTaskExitCritical();
}

void sendLG2(uint64_t data, uint16_t nbits, uint16_t repeat) {
  
  if (nbits >= kLg32Bits) {
    // Let the original routine handle it.
    vTaskEnterCritical();
    sendLG(data, nbits, repeat);  // Send it as a single Samsung message.
    vTaskExitCritical();
    return;
  }
  vTaskEnterCritical();
  // LGv2 (28-bit) protocol.
  sendGeneric1(kLg2HdrMark, kLg2HdrSpace, kLg2BitMark, kLgOneSpace, kLg2BitMark,
              kLgZeroSpace, kLg2BitMark, kLgMinGap, kLgMinMessageLength, data,
              nbits, 38, true, 0,  // Repeats are handled later.
              33);  // Use a duty cycle of 33% (Testing)

  // TODO(crackn): Verify the details of what repeat messages look like.
  // Repeat
  // Protocol has a mandatory repeat-specific code sent after every command.
  if (repeat)
    sendGeneric1(kLg2HdrMark, kLgRptSpace, 0, 0, 0, 0,  // No data is sent.
                kLgBitMark, kLgMinGap, kLgMinMessageLength, 0, 0,  // No data.
                38, true, repeat - 1, 50);

  vTaskExitCritical();
}

void updateSwingPrev(void) {
  _swingv_prev = _swingv;
  for (uint8_t i = 0; i < kLgAcSwingVMaxVanes; i++)
    _vaneswingv_prev[i] = _vaneswingv[i];
}

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

static void _setTemp(const uint8_t value) { _.data.Temp = value; }

static void setTemp(const uint8_t degrees) {
  uint8_t temp = max(kLgAcMinTemp, degrees);
  temp = min(kLgAcMaxTemp, temp);
  _temp = temp;
  _setTemp(temp - kLgAcTempAdjust);
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

uint8_t getPower(void) {
  return _.data.Power == kLgAcPowerOn;
}

static void setMode(const uint8_t mode) {
  switch (mode) {
    case kLgAcAuto:
    case kLgAcDry:
    case kLgAcHeat:
    case kLgAcCool:
    case kLgAcFan:
      _.data.Mode = mode;
      break;
    default:
      _.data.Mode = kLgAcAuto;
  }
}

static void setFan(lg_ac_remote_model_t model, const uint8_t speed) {
  uint8_t _speed = speed;
  // Only model AKB74955603 has these speeds, so convert if we have to.
  if (model != AKB74955603) {
    switch (speed) {
      case kLgAcFanLowAlt:
        _.data.Fan = kLgAcFanLow;
        return;
      case kLgAcFanHigh:
        _.data.Fan = kLgAcFanMax;
        return;
    }
  }

  switch (speed) {
    case kLgAcFanLow:
    case kLgAcFanLowAlt:
      _speed = (model != AKB74955603)
          ? kLgAcFanLow : kLgAcFanLowAlt;
      break;
    case kLgAcFanHigh:
      _speed = (model != AKB74955603)
          ? kLgAcFanMax : speed;
      break;
    case kLgAcFanAuto:
    case kLgAcFanLowest:
    case kLgAcFanMedium:
    case kLgAcFanMax:
      _speed = speed;
      break;
    default:
      _speed = kLgAcFanAuto;
  }
  _.data.Fan = _speed;
}

static void setPower(const bool on) {
  if (on)
    setTemp(_temp);  // Reset the temp if we are on.
  else {
    _setTemp(0);  // Off clears the temp.
  }
  _.data.Power = (on ? kLgAcPowerOn : kLgAcPowerOff);
}

lg_ac_remote_model_t getModel(void) { return _model; }

static opmode_t toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kLgAcCool: return opmode_kCool;
    case kLgAcHeat: return opmode_kHeat;
    case kLgAcDry:  return opmode_kDry;
    case kLgAcFan:  return opmode_kFan;
    default:        return opmode_kAuto;
  }
}

static fanspeed_t toCommonFanSpeed(const uint8_t speed) {
  switch (speed) {
    case kLgAcFanMax:     return fanspeed_kMax;
    case kLgAcFanHigh:    return fanspeed_kHigh;
    case kLgAcFanMedium:  return fanspeed_kMedium;
    case kLgAcFanLow:
    case kLgAcFanLowAlt:  return fanspeed_kLow;
    case kLgAcFanLowest:  return fanspeed_kMin;
    default:              return fanspeed_kAuto;
  }
}

void LG_toCommon(state_ir_t *result)
{
  result->light = true;
  // result->swingv = toCommonSwingV(getSwingV());
  result->protocol = _protocol;
  if (isLightToggle()) {
    result->light = !result->light;
    return;
  } else {
    result->light = _light;
  }
  // result->model = getModel();
  result->power = getPower();
  result->mode = toCommonMode(_.data.Mode);
  result->celsius = true;
  result->degrees = getTemp();
  result->fanspeed = toCommonFanSpeed(_.data.Fan);
  // if (isSwingV()) result->swingv = toCommonSwingV(getSwingV());
  // if (isVaneSwingV())
  //   result->swingv = toCommonVaneSwingV(VANESWINGVPOS(getVaneCode(_.raw)));
  // result->swingh = isSwingH() ? stdAc::swingh_t::kAuto : stdAc::swingh_t::kOff;
  // Not supported.
  result->quiet = false;
  result->turbo = false;
  result->filter = false;
  result->clean = false;
  result->econo = false;
  result->beep = false;
  result->sleep = -1;
  result->clock = -1;
  return;
}

void LG_setRaw(const uint32_t new_code, const decode_type_t protocol, state_ir_t *ir_state) 
{
  _.data.raw = new_code;
  LG_toCommon(ir_state);
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
  ir_state->model = getModel();
  _temp = 15;  // Ensure there is a "sane" previous temp.
  _temp = getTemp();
  os_trace("LG model: %u\r\n", getModel());
}

void LG_setInfo(const uint32_t new_code, const decode_type_t protocol, state_ir_t *ir_state) 
{
  _.data.raw = new_code;
  ir_state->power = getPower();
  ir_state->mode = _.data.Mode;
  ir_state->celsius = true;
  ir_state->degrees = getTemp();
  ir_state->fanspeed = _.data.Fan;

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
  ir_state->model = getModel();
  _temp = 15;  // Ensure there is a "sane" previous temp.
  _temp = getTemp();
  // os_trace("LG model: %u\r\n", getModel());
}

void checksum(void);
static void send2(const uint16_t repeat) {
  if (getPower()) {
    checksum();
    os_trace("%s raw_data: %llx\r\n", __func__, (uint64_t)_.data.raw);
    sendLG2(_.data.raw, kLgBits, repeat);
    state_ir_t ir_state;
    LG_setRaw(_.data.raw, LG2, &ir_state);
    IR_logCommon(ir_state);
  } else {
    os_trace("%s send off cmd\r\n", __func__);
    _.data.raw = kLgAcOffCommand;
    sendLG2(_.data.raw, kLgBits, repeat);
  }
}
static void send(const uint16_t repeat) {
  if (getPower()) {
    checksum();
    os_trace("%s raw_data: %llx\r\n", __func__, (uint64_t)_.data.raw);
    sendLG(_.data.raw, kLgBits, repeat);
    state_ir_t ir_state;
    LG_setRaw(_.data.raw, LG, &ir_state);
    IR_logCommon(ir_state);
  } else {
    os_trace("%s send off cmd\r\n", __func__);
    _.data.raw = kLgAcOffCommand;
    sendLG(_.data.raw, kLgBits, repeat);
  }
}

void stateReset(void) {
  // LG_setRaw(kLgAcOffCommand);
  setModel(GE6711AR2853M);
  _light = true;
  _swingv = kLgAcSwingVOff;
  _swingh = false;
  for (uint8_t i = 0; i < kLgAcSwingVMaxVanes; i++)
    _vaneswingv[i] = 0;  // Reset to an unused value.
  updateSwingPrev();
}

void LG_send(state_ir_t state_ir)
{
  IRsend_begin();
  state_ir_t state_ir_new;
  LG_setRaw(kLgAcOffCommand, LG, &state_ir_new);
  setModel(GE6711AR2853M);

  os_trace("degress: %u, mode: %u, fanspeed: %u, power: %u\r\n", (uint8_t)state_ir.degrees, (uint8_t)state_ir.mode, (uint8_t)state_ir.fanspeed, (uint8_t)state_ir.power);

  setTemp((uint8_t)state_ir.degrees);

  setMode((uint8_t)state_ir.mode);
  setFan(GE6711AR2853M, (uint8_t)state_ir.fanspeed);
  setPower((state_ir.power ? 1 : 0));
  
  send(kNoRepeat);
}

void LG2_send(state_ir_t state_ir)
{
  os_trace("%s \r\n", __func__);
  IRsend_begin();
  state_ir_t state_ir_new;
  LG_setRaw(kLgAcOffCommand, LG2, &state_ir_new);
  setModel(AKB75215403);

  os_trace("degress: %u, mode: %u, fanspeed: %u, power: %u\r\n", (uint8_t)state_ir.degrees, (uint8_t)state_ir.mode, (uint8_t)state_ir.fanspeed, (uint8_t)state_ir.power);

  setTemp((uint8_t)state_ir.degrees);

  setMode((uint8_t)state_ir.mode);
  setFan(AKB75215403, (uint8_t)state_ir.fanspeed);
  setPower((state_ir.power ? 1 : 0));
  
  send2(kNoRepeat);
}

uint8_t calcChecksum(const uint32_t state) {
  return sumNibbles(state >> kLgAcChecksumSize, 4, 0, true);
}

void checksum(void)
{
  // _.data.BitA = 1;
  _.data.Sign = kLgAcSignature;
  _.data.Sum = calcChecksum(_.data.raw);
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
