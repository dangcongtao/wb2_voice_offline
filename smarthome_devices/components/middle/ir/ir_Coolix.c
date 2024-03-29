#include "ir_Coolix.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

// Constants
// Pulse parms are *50-100 for the Mark and *50+100 for the space
// First MARK is the one after the long gap
// pulse parameters in usec
const uint16_t kCoolixTick = 276;  // Approximately 10.5 cycles at 38kHz
const uint16_t kCoolixBitMarkTicks = 2;
const uint16_t kCoolixBitMark = kCoolixBitMarkTicks * kCoolixTick;  // 552us
const uint16_t kCoolixOneSpaceTicks = 6;
const uint16_t kCoolixOneSpace = kCoolixOneSpaceTicks * kCoolixTick;  // 1656us
const uint16_t kCoolixZeroSpaceTicks = 2;
const uint16_t kCoolixZeroSpace = kCoolixZeroSpaceTicks * kCoolixTick;  // 552us
const uint16_t kCoolixHdrMarkTicks = 17;
const uint16_t kCoolixHdrMark = kCoolixHdrMarkTicks * kCoolixTick;  // 4692us
const uint16_t kCoolixHdrSpaceTicks = 16;
const uint16_t kCoolixHdrSpace = kCoolixHdrSpaceTicks * kCoolixTick;  // 4416us
const uint16_t kCoolixMinGapTicks = kCoolixHdrMarkTicks + kCoolixZeroSpaceTicks;
const uint16_t kCoolixMinGap = kCoolixMinGapTicks * kCoolixTick;  // 5244us
const uint8_t  kCoolixExtraTolerance = 5;  // Percent

static bool powerFlag;


bool decodeCOOLIX(decode_results *results, uint16_t offset,
                          const uint16_t nbits, const bool strict) {
  // The protocol sends the data normal + inverted, alternating on
  // each byte. Hence twice the number of expected data bits.
  if (results->rawlen < 2 * 2 * nbits + kHeader + kFooter - 1 + offset)
    return false;  // Can't possibly be a valid COOLIX message.
  if (strict && nbits != kCoolixBits)
    return false;      // Not strictly a COOLIX message.
  if (nbits % 8 != 0)  // nbits has to be a multiple of nr. of bits in a byte.
    return false;

  uint64_t data = 0;
  uint64_t inverted = 0;

  if (nbits > sizeof(data) * 8)
    return false;  // We can't possibly capture a Coolix packet that big.

  // Header
  if (!matchLimit(results->rawbuf[offset++], kCoolixHdrMark-1200, kCoolixHdrMark+1200)) return false;
  if (!matchLimit(results->rawbuf[offset++], kCoolixHdrSpace-1100, kCoolixHdrSpace+1100)) return false;

  // Data
  // Twice as many bits as there are normal plus inverted bits.
  for (uint16_t i = 0; i < nbits * 2; i += 8) {
    const bool flip = (i / 8) % 2;
    uint64_t result = 0;
    // Read the next byte of data.
    const uint16_t used = matchGenericDataLimit(results->rawbuf + offset, &result,
                                       results->rawlen - offset, 8,
                                       0, 0, 0, 0,  // No Header
                                       kCoolixBitMark-450, kCoolixBitMark+450, kCoolixOneSpace-500, kCoolixOneSpace+500,  // Data
                                       kCoolixBitMark-450, kCoolixBitMark+450, kCoolixZeroSpace-450, kCoolixZeroSpace+450,
                                       0, 0, 0, 0, // No Footer
                                       false,
                                       _tolerance + kCoolixExtraTolerance,
                                       0, true);
    if (!used) return false;  // Didn't match a bytes worth of data.
    offset += used;
    if (flip) {  // The inverted byte.
      inverted <<= 8;
      inverted |= result;
    } else {
      data <<= 8;
      data |= result;
    }
  }

  // Footer
  if (!matchLimit(results->rawbuf[offset++], kCoolixBitMark-400, kCoolixBitMark+400)) return false;
  if (offset < results->rawlen &&
      !matchAtLeastLimit(results->rawbuf[offset], kCoolixMinGap-1300)) return false;

  // Compliance
  uint64_t orig = data;  // Save a copy of the data.
  if (strict) {
    for (uint16_t i = 0; i < nbits; i += 8, data >>= 8, inverted >>= 8)
      if ((data & 0xFF) != ((inverted & 0xFF) ^ 0xFF)){
        os_trace("%s invalid checksum\r\n", __func__);
        return false;
      }
  }
    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = COOLIX;
  results->bits = nbits;
  results->value = orig;
  results->address = 0;
  results->command = 0;
  return true;
}

static bool handleSpecialState(const uint32_t data, state_ir_t *ir_state) {
  switch (data) {
    case kCoolixClean:
        ir_state->clean = true;
    //   cleanFlag = !cleanFlag;
      break;
    case kCoolixLed:
        ir_state->light = true;
    //   ledFlag = !ledFlag;
      break;
    case kCoolixOff:
        ir_state->power = false;
    //   powerFlag = false;
      break;
    case kCoolixSwing:
        ir_state->swingv = true;
    //   swingFlag = !swingFlag;
      break;
    case kCoolixSleep:
        ir_state->sleep = true;
    //   sleepFlag = !sleepFlag;
      break;
    case kCoolixTurbo:
        ir_state->turbo = true;
    //   turboFlag = !turboFlag;
      break;
    default:
      return false;
  }
  return true;
}


void IRCoolixAC_setInfo(const uint32_t new_code, state_ir_t *ir_state) {
    CoolixProtocol _;
    memset(&_, 0, sizeof(CoolixProtocol));
    ir_state->power = true;  // Everything that is not the special power off mesg is On.
    if (!handleSpecialState(new_code, ir_state)) {
        // it isn`t special so might affect Temp|mode|Fan
        if (new_code == kCoolixCmdFan) {
            // Fan mode is a special case of Dry.
                ir_state->mode = kCoolixDry;
                ir_state->degrees = kCoolixFanTempCode;
                ir_state->fanspeed = kCoolixFanAuto;
            return;
        }
    }
    // must be a command changing Temp|Mode|Fan
    // it is safe to just copy to remote var
    _.raw = new_code;

    uint8_t mode = _.Mode;
    if (mode == kCoolixDry)
        if (_.Temp == kCoolixFanTempCode)
            ir_state->mode = kCoolixFan;
    ir_state->mode = mode;
    ir_state->fanspeed = _.Fan;
    uint8_t code = _.Temp;
    for (uint8_t i = 0; i < kCoolixTempRange; i++)
        if (kCoolixTempMap[i] == code) ir_state->degrees = kCoolixTempMin + i;
    ir_state->degrees = kCoolixTempMax;  // Not a temp we expected.

}


bool decodeCoolix48(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  if (strict && nbits != kCoolix48Bits)
    return false;      // Not strictly a COOLIX48 message.

  // Header + Data + Footer
  if (!matchGenericDataLimit(results->rawbuf + offset, &(results->value),
                    results->rawlen - offset, nbits,
                    kCoolixHdrMark-1200, kCoolixHdrMark+1200, kCoolixHdrSpace-1100, kCoolixHdrSpace+1100,
                    kCoolixBitMark-450, kCoolixBitMark+450, kCoolixOneSpace-500, kCoolixOneSpace+500,
                    kCoolixBitMark-450, kCoolixBitMark+450, kCoolixZeroSpace-450, kCoolixZeroSpace+450,
                    kCoolixBitMark-450, kCoolixBitMark+450, kCoolixMinGap-1300, kCoolixMinGap+1300,
                    true, _tolerance + kCoolixExtraTolerance, 0, true))
    return false;
    
    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = COOLIX48;
  results->bits = nbits;
  results->address = 0;
  results->command = 0;
  return true;
}