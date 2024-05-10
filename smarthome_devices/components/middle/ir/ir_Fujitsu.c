#include "ir_Fujitsu.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

// These values are based on averages of measurements
const uint16_t kFujitsuAcHdrMark = 3324;
const uint16_t kFujitsuAcHdrSpace = 1574;
const uint16_t kFujitsuAcBitMark = 448;
const uint16_t kFujitsuAcOneSpace = 1182;
const uint16_t kFujitsuAcZeroSpace = 390;
const uint16_t kFujitsuAcMinGap = 8100;
const uint8_t  kFujitsuAcExtraTolerance = 5;  // Extra tolerance percentage.

static bool IRFujitsuAC_validChecksum(uint8_t state[], const uint16_t length) {
  uint8_t sum = 0;
  uint8_t sum_complement = 0;
  uint8_t checksum = state[length - 1];
  switch (length) {
    case kFujitsuAcStateLengthShort:  // ARRAH2E, ARREB1E, & ARRY4
      return state[length - 1] == (uint8_t)~state[length - 2];
    case kFujitsuAcStateLength - 1:  // ARDB1 & ARJW2
      sum = sumBytesState(state, length - 1, 0);
      sum_complement = 0x9B;
      break;
    case kFujitsuAcStateLength:  // ARRAH2E, ARRY4, & ARREB1E
      sum = sumBytesState(state + kFujitsuAcStateLengthShort,
                     length - 1 - kFujitsuAcStateLengthShort, 0);
      break;
    default:        // Includes ARDB1 & ARJW2 short.
      return true;  // Assume the checksum is valid for other lengths.
  }
  return checksum == (uint8_t)(sum_complement - sum);  // Does it match?
}

bool decodeFujitsuAC(decode_results* results, uint16_t offset,
                             const uint16_t nbits,
                             const bool strict) {
  uint16_t dataBitsSoFar = 0;

  // Have we got enough data to successfully decode?
  if (results->rawlen < (2 * kFujitsuAcMinBits) + kHeader + kFooter - 1 +
      offset)
    return false;  // Can't possibly be a valid message.

  // Compliance
  if (strict) {
    switch (nbits) {
      case kFujitsuAcBits:
      case kFujitsuAcBits - 8:
      case kFujitsuAcMinBits:
      case kFujitsuAcMinBits + 8: break;
      default: return false;  // Must be called with the correct nr. of bits.
    }
  }

  // Header / Some of the Data
  uint16_t used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                               results->rawlen - offset, kFujitsuAcMinBits - 8,
                               kFujitsuAcHdrMark-324,kFujitsuAcHdrMark+376, kFujitsuAcHdrSpace-374, kFujitsuAcHdrSpace+376,  // Header
                               kFujitsuAcBitMark-300, kFujitsuAcBitMark+300, kFujitsuAcOneSpace-300, kFujitsuAcOneSpace+300,  // Data
                               kFujitsuAcBitMark-300, kFujitsuAcBitMark+300, kFujitsuAcZeroSpace -300, kFujitsuAcZeroSpace+300,
                               0, 0, 0, 0,  // No Footer (yet)
                               false, _tolerance + kFujitsuAcExtraTolerance, 0,
                               false);  // LSBF
  if (!used) return false;
  offset += used;
  // Check we have the typical data header.
  if (results->state[0] != 0x14 || results->state[1] != 0x63) return false;
  dataBitsSoFar += kFujitsuAcMinBits - 8;

  // Keep reading bytes until we either run out of message or state to fill.
  match_result_t data_result;
  for (uint16_t i = 5;
       offset <= results->rawlen - 16 && i < kFujitsuAcStateLength;
       i++, dataBitsSoFar += 8, offset += data_result.used) {
    data_result = matchDataLimit(
        &(results->rawbuf[offset]), 8,  kFujitsuAcBitMark-300, kFujitsuAcBitMark+300, kFujitsuAcOneSpace-300, kFujitsuAcOneSpace+300,
        kFujitsuAcBitMark-300, kFujitsuAcBitMark+300, kFujitsuAcZeroSpace -300, kFujitsuAcZeroSpace+300,
        _tolerance + kFujitsuAcExtraTolerance, 0, false, true);
    if (data_result.success == false) break;  // Fail
    results->state[i] = data_result.data;
  }

  // Footer
  if (offset > results->rawlen ||
      !matchLimit(results->rawbuf[offset++], kFujitsuAcBitMark-348, kFujitsuAcBitMark+342))
    return false;
  // The space is optional if we are out of capture.
  if (offset < results->rawlen &&
      !matchAtLeastLimit(results->rawbuf[offset], kFujitsuAcMinGap-2000))
    return false;

  // Compliance
  if (strict) {
    if (dataBitsSoFar != nbits) return false;
  }
  os_trace("%s success\r\n", __func__);
  results->decode_type = FUJITSU_AC;
  results->bits = dataBitsSoFar;

  // Compliance
  switch (dataBitsSoFar) {
    case kFujitsuAcMinBits:
      // Check if this values indicate that this should have been a long state
      // message.
      if (results->state[5] == 0xFC) return false;
      return true;  // Success
    case kFujitsuAcMinBits + 8:
      // Check if this values indicate that this should have been a long state
      // message.
      if (results->state[5] == 0xFE) return false;
      // The last byte needs to be the inverse of the penultimate byte.
      if (results->state[5] != (uint8_t)~results->state[6]) return false;
      return true;  // Success
    case kFujitsuAcBits - 8:
      // Long messages of this size require this byte be correct.
      if (results->state[5] != 0xFC) return false;
      break;
    case kFujitsuAcBits:
      // Long messages of this size require this byte be correct.
      if (results->state[5] != 0xFE) return false;
      break;
    default:
      os_trace("%s Unexpected size\r\n", __func__);
      return false;  // Unexpected size.
  }
  if (!IRFujitsuAC_validChecksum(results->state, dataBitsSoFar / 8)) {
    os_trace("%s invalid\r\n", __func__);
    return false;
  }

  // Success
  return true;  // All good.
}

static uint8_t _cmd;
static fujitsu_ac_remote_model_t _model;
static  uint8_t _state_length;
static  uint8_t _state_length_short;

static void setModel(const fujitsu_ac_remote_model_t model) {
  _model = model;
  switch (model) {
    case ARDB1:
    case ARJW2:
      _state_length = kFujitsuAcStateLength - 1;
      _state_length_short = kFujitsuAcStateLengthShort - 1;
      break;
    case ARRY4:
    case ARRAH2E:
    case ARREB1E:
    default:
      _state_length = kFujitsuAcStateLength;
      _state_length_short = kFujitsuAcStateLengthShort;
  }
}

static void setCmd(const uint8_t cmd) {
  switch (cmd) {
    case kFujitsuAcCmdTurnOff:
    case kFujitsuAcCmdTurnOn:
    case kFujitsuAcCmdStayOn:
    case kFujitsuAcCmdStepVert:
    case kFujitsuAcCmdToggleSwingVert:
      _cmd = cmd;
      break;
    case kFujitsuAcCmdStepHoriz:
    case kFujitsuAcCmdToggleSwingHoriz:
      switch (_model) {
        // Only these remotes have horizontal.
        case ARRAH2E:
        case ARJW2:
          _cmd = cmd;
          break;
        default:
          _cmd = kFujitsuAcCmdStayOn;
      }
      break;
    case kFujitsuAcCmdEcono:
    case kFujitsuAcCmdPowerful:
      switch (_model) {
        // Only these remotes have these commands.
        case ARREB1E:
        case ARREW4E:
          _cmd = cmd;
        break;
      default:
        _cmd = kFujitsuAcCmdStayOn;
      }
      break;
    default:
      _cmd = kFujitsuAcCmdStayOn;
  }
}

static bool get10CHeat(FujitsuProtocol *_) {
  switch (_model) {
    case ARRAH2E:
    case ARREW4E:
      return (_->Clean && _->Power && _->Mode == kFujitsuAcModeFan &&
              _->Fan == kFujitsuAcFanAuto && _->Swing == kFujitsuAcSwingOff);
    default: return false;
  }
}

static void IRFujitsuAC_buildFromState(FujitsuProtocol *_, const uint16_t length) {
  switch (length) {
    case kFujitsuAcStateLength - 1:
    case kFujitsuAcStateLengthShort - 1:
      setModel(ARDB1);
      // ARJW2 has horizontal swing.
      if (_->Swing > kFujitsuAcSwingVert)
        setModel(ARJW2);
      break;
    default:
      switch (_->Cmd) {
        case kFujitsuAcCmdEcono:
        case kFujitsuAcCmdPowerful:
          setModel(ARREB1E);
          break;
        default:
          setModel(ARRAH2E);
      }
  }
  switch (_->RestLength) {
    case 8:
      if (_model != ARJW2)
        setModel(ARDB1);
      break;
    case 9:
      if (_model != ARREB1E)
        setModel(ARRAH2E);
      break;
  }
  if (_->Power)
    setCmd(kFujitsuAcCmdTurnOn);
  else
    setCmd(kFujitsuAcCmdStayOn);
  // Currently the only way we know how to tell ARRAH2E & ARRY4 apart is if
  // either the raw Filter or Clean setting is on.
  if (_model == ARRAH2E && (_->Filter || _->Clean) &&
      !get10CHeat(_))
    setModel(ARRY4);
  if (_state_length == kFujitsuAcStateLength && _->OutsideQuiet)
    setModel(ARREB1E);
  switch (_->Cmd) {
    case kFujitsuAcCmdTurnOff:
    case kFujitsuAcCmdStepHoriz:
    case kFujitsuAcCmdToggleSwingHoriz:
    case kFujitsuAcCmdStepVert:
    case kFujitsuAcCmdToggleSwingVert:
    case kFujitsuAcCmdEcono:
    case kFujitsuAcCmdPowerful:
      setCmd(_->Cmd);
      break;
  }
  if (_->Protocol == 0x31) setModel(ARREW4E);
}

float IRFujitsuAC_getTemp(FujitsuProtocol *_){
  if (_model == ARREW4E) {
    if (_->Fahrenheit)  // Currently only ARREW4E supports native Fahrenheit.
      return _->Temp + kFujitsuAcTempOffsetF;
    else
      return (_->Temp / 2.0) + (kFujitsuAcMinTemp / 2);
  } else {
    return _->Temp / 4 + kFujitsuAcMinTemp;
  }
}

bool IRFujitsuAC_setInfo(const uint8_t newState[], const uint16_t length, state_ir_t *ir_state) {
    FujitsuProtocol _;

  if (length > kFujitsuAcStateLength) return false;
  for (uint16_t i = 0; i < kFujitsuAcStateLength; i++) {
    if (i < length)
      _.longcode[i] = newState[i];
    else
      _.longcode[i] = 0;
  }
  IRFujitsuAC_buildFromState(&_, length);
    ir_state->model = _model;
    ir_state->power = _cmd != kFujitsuAcCmdTurnOff;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = IRFujitsuAC_getTemp(&_);
    ir_state->command = _cmd;
  return true;
}

static void setTemp(FujitsuProtocol *_, const float temp, const bool useCelsius) {
  float mintemp;
  float maxtemp;
  uint8_t offset;
  bool _useCelsius;
  float _temp;

  switch (_model) {
    // These models have native Fahrenheit & Celsius upport.
    case ARREW4E:
      _useCelsius = useCelsius;
      _temp = temp;
      break;
    // Make sure everything else uses Celsius.
    default:
      _useCelsius = true;
      _temp = useCelsius ? temp : fahrenheitToCelsius(temp);
  }
  _->Fahrenheit = !_useCelsius;

  if (_useCelsius) {
    mintemp = kFujitsuAcMinTemp;
    maxtemp = kFujitsuAcMaxTemp;
    offset = kFujitsuAcTempOffsetC;
  } else {
    mintemp = kFujitsuAcMinTempF;
    maxtemp = kFujitsuAcMaxTempF;
    offset = kFujitsuAcTempOffsetF;
  }
  _temp = max(mintemp, _temp);
  _temp = min(maxtemp, _temp);
  if (_useCelsius) {
    if (_model == ARREW4E)
      _->Temp = (_temp - (offset / 2)) * 2;
    else
      _->Temp = (_temp - offset) * 4;
  } else {
    _->Temp = _temp - offset;
  }
//   _rawstatemodified = true;
  setCmd(kFujitsuAcCmdStayOn);  // No special command involved.
}

static bool updateUseLongOrShort(FujitsuProtocol *_) {
  bool fullCmd = false;
  switch (_cmd) {
    case kFujitsuAcCmdTurnOff:     // 0x02
    case kFujitsuAcCmdEcono:       // 0x09
    case kFujitsuAcCmdPowerful:    // 0x39
    case kFujitsuAcCmdStepVert:    // 0x6C
    case kFujitsuAcCmdToggleSwingVert:   // 0x6D
    case kFujitsuAcCmdStepHoriz:   // 0x79
    case kFujitsuAcCmdToggleSwingHoriz:  // 0x7A
      _->Cmd = _cmd;
    //   _rawstatemodified = true;
      break;
    default:
      switch (_model) {
        case ARRY4:
        case ARRAH2E:
        case ARREB1E:
        case ARREW4E:
          _->Cmd = 0xFE;
        //   _rawstatemodified = true;
          break;
        case ARDB1:
        case ARJW2:
          _->Cmd = 0xFC;
        //   _rawstatemodified = true;
          break;
      }
      fullCmd = true;
      break;
  }
  return fullCmd;
}

static void sendFujitsuAC(const unsigned char data[], const uint16_t nbytes,
                           const uint16_t repeat) {
vTaskEnterCritical();
  sendGenericBytes(kFujitsuAcHdrMark, kFujitsuAcHdrSpace, kFujitsuAcBitMark,
              kFujitsuAcOneSpace, kFujitsuAcBitMark, kFujitsuAcZeroSpace,
              kFujitsuAcBitMark, kFujitsuAcMinGap, data, nbytes, 38, false,
              repeat, 50);
vTaskExitCritical();
}

static bool isLongCode(FujitsuProtocol *_) {
  return _->Cmd == 0xFE || _->Cmd == 0xFC;
}

static uint8_t* getRaw(FujitsuProtocol *_) {
  return isLongCode(_) ? _->longcode : _->shortcode;
}

static uint8_t getStateLength(FujitsuProtocol *_) {
  return updateUseLongOrShort(_) ? _state_length : _state_length_short;
}

void FujitsuAC_send(state_ir_t state_ir) 
{
    os_trace("%s\r\n", __func__);
    FujitsuProtocol _;
    setModel(state_ir.model);
    for (size_t i = 0; i < kFujitsuAcStateLength; i++) {
        _.longcode[i] = 0;
    }
  setTemp(&_, 24, true);
  _.Fan = kFujitsuAcFanHigh;
  _.Mode = kFujitsuAcModeCool;
  _.Swing = kFujitsuAcSwingBoth;
  _cmd = kFujitsuAcCmdTurnOn;
  _.Filter = false;
  _.Clean = false;
  _.TimerType = kFujitsuAcStopTimers;
  _.OnTimer = 0;
  _.OffTimer = 0;
  _.longcode[0] = 0x14;
  _.longcode[1] = 0x63;
  _.longcode[3] = 0x10;
  _.longcode[4] = 0x10;

  // set mode
  if (state_ir.mode > kFujitsuAcModeHeat)
    _.Mode = kFujitsuAcModeHeat;  // Set the mode to maximum if out of range.
  else
    _.Mode = state_ir.mode;
  // set fanspeed
  if (state_ir.fanspeed > kFujitsuAcFanQuiet)
    _.Fan = kFujitsuAcFanHigh;  // Set the fan to maximum if out of range.
  else
    _.Fan = state_ir.fanspeed;

    setTemp(&_, state_ir.degrees, true);

    // set power
  setCmd(state_ir.power ? kFujitsuAcCmdTurnOn : kFujitsuAcCmdTurnOff);


    // check sum
    //   _rawstatemodified = true;
  if (updateUseLongOrShort(&_)) {  // Is it going to be a long code?
    // Nr. of bytes in the message after this byte.
    _.RestLength = _state_length - 7;
    _.Protocol = (_model == ARREW4E) ? 0x31 : 0x30;
    _.Power = (_cmd == kFujitsuAcCmdTurnOn) || get10CHeat(&_);

    // These values depend on model
    if (_model != ARREB1E &&
        _model != ARREW4E) {
      _.OutsideQuiet = 0;
      if (_model != ARRAH2E) {
        _.TimerType = kFujitsuAcStopTimers;
      }
    }
    if (_model != ARRY4) {
      switch (_model) {
        case ARRAH2E:
        case ARREW4E:
          break;
        default:
          _.Clean = false;
      }
      _.Filter = false;
    }
    // Set the On/Off/Sleep timer Nr of mins.
    // _.OffTimer = getOffSleepTimer();
    // _.OnTimer = getOnTimer();
    // Enable bit for the Off/Sleep timer
    _.OffTimerEnable = _.OffTimer > 0;
    // Enable bit for the On timer
    _.OnTimerEnable = _.OnTimer > 0;

    uint8_t checksum = 0;
    uint8_t checksum_complement = 0;
    switch (_model) {
      case ARDB1:
      case ARJW2:
        _.Swing = kFujitsuAcSwingOff;
        checksum = sumBytesState(_.longcode, _state_length - 1, 0);
        checksum_complement = 0x9B;
        break;
      case ARREB1E:
      case ARRAH2E:
      case ARRY4:
        _.unknown = 1;
        // FALL THRU
      default:
        checksum = sumBytesState(_.longcode + _state_length_short,
                            _state_length - _state_length_short - 1, 0);
    }
    // and negate the checksum and store it in the last byte.
    _.longcode[_state_length - 1] = checksum_complement - checksum;
  } else {  // short codes
    for (size_t i = 0; i < _state_length_short; i++) {
      _.shortcode[i] = _.longcode[i];
    }
    switch (_model) {
      case ARRY4:
      case ARRAH2E:
      case ARREB1E:
      case ARREW4E:
        // The last byte is the inverse of penultimate byte
        _.shortcode[_state_length_short - 1] =
            ~_.shortcode[_state_length_short - 2];
        break;
      default:
        {};  // We don't need to do anything for the others.
    }
  }
//   _rawstatemodified = true;
    sendFujitsuAC(getRaw(&_), getStateLength(&_), kFujitsuAcMinRepeat);
}


