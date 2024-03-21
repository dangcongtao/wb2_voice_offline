#include "ir_Gree.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"
#include "ir_Kelvinator.h"

const uint16_t kGreeHdrMark = 9000;
const uint16_t kGreeHdrSpace = 4500;  ///< See #684 & real example in unit tests
const uint16_t kGreeBitMark = 540;
const uint16_t kGreeOneSpace = 1600;
const uint16_t kGreeZeroSpace = 560;
const uint16_t kGreeMsgSpace = 19980;  ///< See #1508, #386, & Kelvinator
const uint8_t kGreeBlockFooter = 0b010;
const uint8_t kGreeBlockFooterBits = 3;

static void sendGree(const uint8_t data[], const uint16_t nbytes,
                      const uint16_t repeat) {
  if (nbytes < kGreeStateLength)
    return;  // Not enough bytes to send a proper message.

  vTaskEnterCritical();

  for (uint16_t r = 0; r <= repeat; r++) {
    // Block #1
    sendGenericBytes(kGreeHdrMark, kGreeHdrSpace, kGreeBitMark, kGreeOneSpace,
                kGreeBitMark, kGreeZeroSpace, 0, 0,  // No Footer.
                data, 4, 38, false, 0, 50);
    // Footer #1
    sendGeneric(0, 0,  // No Header
                kGreeBitMark, kGreeOneSpace, kGreeBitMark, kGreeZeroSpace,
                kGreeBitMark, kGreeMsgSpace, 0b010, 3, 38, false, 0, 50);

    // Block #2
    sendGenericBytes(0, 0,  // No Header for Block #2
                kGreeBitMark, kGreeOneSpace, kGreeBitMark, kGreeZeroSpace,
                kGreeBitMark, kGreeMsgSpace, data + 4, nbytes - 4, 38, false, 0,
                50);
  }
  vTaskExitCritical();
}

static bool validChecksum(const uint8_t state[], const uint16_t length) {
  // Top 4 bits of the last byte in the state is the state's checksum.
  return GETBITS8(state[length - 1], kHighNibble, kNibbleSize) ==
      IRKelvinatorAC_calcBlockChecksum(state, length);
}

bool decodeGree(decode_results* results, uint16_t offset,
                        const uint16_t nbits, bool const strict) {
  if (results->rawlen <=
      2 * (nbits + kGreeBlockFooterBits) + (kHeader + kFooter + 1) - 1 + offset)
    return false;  // Can't possibly be a valid Gree message.
  if (strict && nbits != kGreeBits)
    return false;  // Not strictly a Gree message.

  // There are two blocks back-to-back in a full Gree IR message
  // sequence.

  uint16_t used;
  // Header + Data Block #1 (32 bits)
  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, nbits / 2,
                      kGreeHdrMark-800, kGreeHdrMark+800, kGreeHdrSpace-500, kGreeHdrSpace+500,
                      kGreeBitMark-440, kGreeBitMark+400, kGreeOneSpace-600, kGreeOneSpace+500,
                      kGreeBitMark-440, kGreeBitMark+400, kGreeZeroSpace-460, kGreeZeroSpace+440,
                      0, 0, 0, 0, false,
                      45, kMarkExcess, false);
  if (used == 0) {
    // os_trace("%s matchGenericState 1 failed\r\n", __func__);
    return false;
  }
  offset += used;

  // Block #1 footer (3 bits, B010)
  match_result_t data_result;
  data_result = matchDataLimit(&(results->rawbuf[offset]), kGreeBlockFooterBits,
                          kGreeBitMark-440, kGreeBitMark+440, kGreeOneSpace-500, kGreeOneSpace+500, kGreeBitMark-440, kGreeBitMark+440,
                          kGreeZeroSpace-460, kGreeZeroSpace+440, _tolerance, kMarkExcess, false, true);
  if (data_result.success == false) return false;
  if (data_result.data != kGreeBlockFooter) return false;
  offset += data_result.used;

  // Inter-block gap + Data Block #2 (32 bits) + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state + 4,
                    results->rawlen - offset, nbits / 2,
                    kGreeBitMark-440, kGreeBitMark+400, kGreeMsgSpace-3000, kGreeMsgSpace+3000,
                    kGreeBitMark-440, kGreeBitMark+400, kGreeOneSpace-600, kGreeOneSpace+500,
                    kGreeBitMark-440, kGreeBitMark+400, kGreeZeroSpace-460, kGreeZeroSpace+440,
                    kGreeBitMark-440, kGreeBitMark+400, kGreeMsgSpace-5000, kGreeMsgSpace+3000, true,
                    45, kMarkExcess, false)) {
                        // os_trace("%s matchGenericState 2 failed\r\n", __func__);
                        return false;
                    }

  // Compliance
  if (strict) {
    // Verify the message's checksum is correct.
    if (!validChecksum(results->state, kGreeStateLength)){
      os_trace("%s verify checksum is incorrect\r\n", __func__);
      return false;
    }
  }

  // Success
  results->decode_type = GREE;
  results->bits = nbits;

  os_trace("%s success\r\n", __func__);
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

static opmode_t toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kGreeCool: return opmode_kCool;
    case kGreeHeat: return opmode_kHeat;
    case kGreeDry:  return opmode_kDry;
    case kGreeFan:  return opmode_kFan;
    default:        return opmode_kAuto;
  }
}

static uint8_t getTemp(GreeProtocol _) {
  uint8_t deg = kGreeMinTempC + _.Temp;
  if (_.UseFahrenheit) {
    deg = celsiusToFahrenheit(deg);
    // Retrieve the "extra" fahrenheit from elsewhere in the code.
    if (_.TempExtraDegreeF) deg++;
    deg = max(deg, kGreeMinTempF);  // Cover the fact that 61F is < 16C
  }
  return deg;
}

static fanspeed_t toCommonFanSpeed(const uint8_t speed) {
  switch (speed) {
    case kGreeFanMax:     return fanspeed_kMax;
    case kGreeFanMax - 1: return fanspeed_kMedium;
    case kGreeFanMin:     return fanspeed_kMin;
    default:              return fanspeed_kAuto;
  }
}

void Gree_setRaw(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state) 
{
    GreeProtocol _;

    memcpy(_.remote_state, newState, kGreeStateLength);

    ir_state->protocol = GREE;
  // We can only detect the difference between models when the power is on.
    if (_.Power) {
        if (_.ModelA)
        ir_state->model = YAW1F;
        else
        ir_state->model = YBOFB;
    }
    if (_.Mode == kGreeEcono) ir_state->model = YX1FSF;

    ir_state->power = _.Power;
    ir_state->mode = toCommonMode(_.Mode);
    ir_state->celsius = !_.UseFahrenheit;
    ir_state->degrees = getTemp(_);
    ir_state->fanspeed = toCommonFanSpeed(_.Fan);


    // Toshiba_toCommon(_, ir_state);
}

static swingv_t IRGreeAC_toCommonSwingV(const uint8_t pos) {
  switch (pos) {
    case kGreeSwingUp:         return swingv_kHighest;
    case kGreeSwingMiddleUp:   return swingv_kHigh;
    case kGreeSwingMiddle:     return swingv_kMiddle;
    case kGreeSwingMiddleDown: return swingv_kLow;
    case kGreeSwingDown:       return swingv_kLowest;
    default:                   return swingv_kAuto;
  }
}

void Gree_setInfo(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state) 
{
    GreeProtocol _;

    memcpy(_.remote_state, newState, kGreeStateLength);

    ir_state->protocol = GREE;
  // We can only detect the difference between models when the power is on.
    if (_.Power) {
        if (_.ModelA)
          ir_state->model = YAW1F;
        else
          ir_state->model = YBOFB;
    }
    if (_.Mode == kGreeEcono) ir_state->model = YX1FSF;

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->celsius = !_.UseFahrenheit;
    ir_state->degrees = getTemp(_);
    ir_state->fanspeed = _.Fan;
    if (_.SwingAuto)
      ir_state->swingv = swingv_kAuto;
    else 
      ir_state->swingv = IRGreeAC_toCommonSwingV(_.SwingV);
    
    ir_state->swingh = _.SwingH;


    // Toshiba_toCommon(_, ir_state);
}

static void Gree_stateReset(GreeProtocol *_) {
  // This resets to a known-good state to Power Off, Fan Auto, Mode Auto, 25C.
  memset(_->remote_state, 0, sizeof _->remote_state);
  _->Temp = 9;  // _.remote_state[1] = 0x09;
  _->Light = true;  // _.remote_state[2] = 0x20;
  _->unknown1 = 5;  // _.remote_state[3] = 0x50;
  _->unknown2 = 4;  // _.remote_state[5] = 0x20;
}

static void setTemp(GreeProtocol *_, const uint8_t temp, const bool fahrenheit) {
    float safecelsius = (float)temp;
    if (fahrenheit)
        // Covert to F, and add a fudge factor to round to the expected degree.
        // Why 0.6 you ask?! Because it works. Ya'd thing 0.5 would be good for
        // rounding, but Noooooo!
        safecelsius = fahrenheitToCelsius(temp + 0.6);

    _->UseFahrenheit = fahrenheit;

    // Make sure we have desired temp in the correct range.
    safecelsius = max((float)(kGreeMinTempC), safecelsius);
    safecelsius = min((float)(kGreeMaxTempC), safecelsius);
    // An operating mode of Auto locks the temp to a specific value. Do so.
    if (_->Mode == kGreeAuto) safecelsius = 25;

    // Set the "main" Celsius degrees.
    _->Temp = safecelsius - kGreeMinTempC;
    // Deal with the extra degree fahrenheit difference.
    _->TempExtraDegreeF = ((uint8_t)(safecelsius * 2) & 1);
}

static void setFan(GreeProtocol *_, const uint8_t speed) {
  uint8_t fan = min(kGreeFanMax, speed);  // Bounds check
  if (_->Mode == kGreeDry) fan = 1;  // DRY mode is always locked to fan 1.
  // Set the basic fan values.
  _->Fan = fan;
}

static void setMode(GreeProtocol *_, const uint8_t new_mode) {
  uint8_t mode = new_mode;
  switch (mode) {
    // AUTO is locked to 25C
    case kGreeAuto: setTemp(_, 25, false); break;
    // DRY always sets the fan to 1.
    case kGreeDry: setFan(_, 1); break;
    case kGreeCool:
    case kGreeFan:
    case kGreeEcono:
    case kGreeHeat: break;
    // If we get an unexpected mode, default to AUTO.
    default: mode = kGreeAuto;
  }
  _->Mode = mode;
}

void Gree_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);
    GreeProtocol _;
    Gree_stateReset(&_);

    // Mode
    setMode(&_, (uint8_t)state_ir.mode);
    // Temp
    setTemp(&_, (uint8_t)state_ir.degrees, false);

    setFan(&_, (uint8_t)state_ir.fanspeed);

    if (state_ir.swingh != (uint8_t)swingh_kOff) {
      if (state_ir.swingh <= kGreeSwingHMaxRight)
        _.SwingH = state_ir.swingh;
      else
        _.SwingH = kGreeSwingHOff;
    }

    if (state_ir.swingv != (uint8_t)swingv_kOff) {
      if (state_ir.swingv == swingv_kAuto) {
        _.SwingAuto = true;
        _.SwingV = kGreeSwingAuto;
      } else {
        _.SwingV = state_ir.swingv;
      }
    }

    // Power
    _.Power = state_ir.power;
    _.ModelA = (state_ir.power && state_ir.model == YAW1F);

    _.Sum = IRKelvinatorAC_calcBlockChecksum(_.remote_state, kGreeStateLength);

    sendGree(_.remote_state, kGreeStateLength, 0);
}