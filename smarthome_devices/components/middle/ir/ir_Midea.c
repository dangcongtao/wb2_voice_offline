#include "ir_Midea.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"
#include "ir_NEC.h"


// Constants
const uint16_t kMideaTick = 80;
const uint16_t kMideaBitMarkTicks = 7;
const uint16_t kMideaBitMark = kMideaBitMarkTicks * kMideaTick;
const uint16_t kMideaOneSpaceTicks = 21;
const uint16_t kMideaOneSpace = kMideaOneSpaceTicks * kMideaTick;
const uint16_t kMideaZeroSpaceTicks = 7;
const uint16_t kMideaZeroSpace = kMideaZeroSpaceTicks * kMideaTick;
const uint16_t kMideaHdrMarkTicks = 56;
const uint16_t kMideaHdrMark = kMideaHdrMarkTicks * kMideaTick;
const uint16_t kMideaHdrSpaceTicks = 56;
const uint16_t kMideaHdrSpace = kMideaHdrSpaceTicks * kMideaTick;
const uint16_t kMideaMinGapTicks =
    kMideaHdrMarkTicks + kMideaZeroSpaceTicks + kMideaBitMarkTicks;
const uint16_t kMideaMinGap = kMideaMinGapTicks * kMideaTick;
const uint8_t kMideaTolerance = 30;  // Percent
const uint16_t kMidea24MinGap = 13000;  ///< uSecs


static uint8_t calcChecksum(const uint64_t state) {
  uint8_t sum = 0;
  uint64_t temp_state = state;

  for (uint8_t i = 0; i < 5; i++) {
    temp_state >>= 8;
    sum += reverseBits((temp_state & 0xFF), 8);
  }
  sum = 256 - sum;
  return reverseBits(sum, 8);
}

static bool validChecksum(const uint64_t state) {
  return GETBITS64(state, 0, 8) == calcChecksum(state);
}

bool decodeMidea(decode_results *results, uint16_t offset,
                         const uint16_t nbits, const bool strict) {
  uint8_t min_nr_of_messages = 1;
  if (strict) {
    if (nbits != kMideaBits) return false;  // Not strictly a MIDEA message.
    min_nr_of_messages = 2;
  }

  // The protocol sends the data normal + inverted, alternating on
  // each byte. Hence twice the number of expected data bits.
  if (results->rawlen <
      min_nr_of_messages * (2 * nbits + kHeader + kFooter) - 1 + offset)
    return false;  // Can't possibly be a valid MIDEA message.

  uint64_t data = 0;
  uint64_t inverted = 0;

  if (nbits > sizeof(data) * 8)
    return false;  // We can't possibly capture a Midea packet that big.

  for (uint8_t i = 0; i < min_nr_of_messages; i++) {
    // Match Header + Data + Footer
    uint16_t used;
    used = matchGeneric(results->rawbuf + offset, i % 2 ? &inverted : &data,
                        results->rawlen - offset, nbits,
                        kMideaHdrMark, kMideaHdrSpace,
                        kMideaBitMark, kMideaOneSpace,
                        kMideaBitMark, kMideaZeroSpace,
                        kMideaBitMark, kMideaMinGap,
                        i % 2,  // No "atleast" on 1st part, but yes on the 2nd.
                        45, kMarkExcess, true);
                    
    if (!used) {
        os_trace("%s matchGeneric failed %u\r\n", __func__, i);
        return false;
    }
    offset += used;
  }

  // Compliance
  if (strict) {
    // Protocol requires a second message with all the data bits inverted.
    // We should have checked we got a second message in the previous loop.
    // Just need to check it's value is an inverted copy of the first message.
    uint64_t mask = (1ULL << kMideaBits) - 1;
    // os_trace("%s check inverted failed, test: %016llx, mask: %016llx, data: %016llx, inverted: %016llx\r\n", __func__, 0xFFFFFFFFFFFA, mask, data, inverted);
    // os_trace("%s exp1: %016llx, exp2: %016llx\r\n", __func__, (data & mask), ((inverted ^ mask) & mask));
    if ((data & mask) != ((inverted ^ mask) & mask)) {
        return false;
    }
    if (!validChecksum(data)) {
        return false;
    }
  }

  // Success
  results->decode_type = MIDEA;
  results->bits = nbits;
  results->value = data;
  results->address = 0;
  results->command = 0;
  os_trace("%s success\r\n", __func__);
  return true;
}

bool decodeMidea24(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  // Not strictly a MIDEA24 message.
  if (strict && nbits != kMidea24Bits) return false;
  if (nbits > 32) return false;  // Can't successfully match something that big.

  uint64_t longdata = 0;
  if (!matchGeneric(results->rawbuf + offset, &longdata,
                    results->rawlen - offset, nbits * 2,
                    kNecHdrMark, kNecHdrSpace,
                    kNecBitMark, kNecOneSpace,
                    kNecBitMark, kNecZeroSpace,
                    kNecBitMark, kMidea24MinGap, true, kUseDefTol, kMarkExcess, true)) return false;

  // Build the result by checking every second byte is a complement(inversion)
  // of the previous one.
  uint32_t data = 0;
  for (uint8_t i = nbits * 2; i >= 16;) {
    // Shuffle the data collected so far.
    data <<= 8;
    i -= 8;
    uint8_t current = GETBITS64(longdata, i, 8);
    i -= 8;
    uint8_t next = GETBITS64(longdata, i, 8);
    // Check they are an inverted pair.
    if (current != (next ^ 0xFF)) return false;  // They are not, so abort.
    data |= current;
  }

  // Success
  results->decode_type = MIDEA24;
  results->bits = nbits;
  results->value = data;
  results->address = 0;
  results->command = 0;
  os_trace("%s success\r\n", __func__);
  return true;
}
static opmode_t toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kMideaACCool: return opmode_kCool;
    case kMideaACHeat: return opmode_kHeat;
    case kMideaACDry:  return opmode_kDry;
    case kMideaACFan:  return opmode_kFan;
    default:           return opmode_kAuto;
  }
}

/// Convert a native fan speed into its stdAc equivalent.
/// @param[in] speed The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
static fanspeed_t toCommonFanSpeed(const uint8_t speed) {
  switch (speed) {
    case kMideaACFanHigh: return fanspeed_kMax;
    case kMideaACFanMed:  return fanspeed_kMedium;
    case kMideaACFanLow:  return fanspeed_kMin;
    default:              return fanspeed_kAuto;
  }
}

static uint8_t getTemp(MideaProtocol _, const bool celsius) {
  uint8_t temp = _.Temp;
  if (!_.useFahrenheit)
    temp += kMideaACMinTempC;
  else
    temp += kMideaACMinTempF;
  if (celsius && _.useFahrenheit) temp = fahrenheitToCelsius(temp) + 0.5;
  if (!celsius && !_.useFahrenheit) temp = celsiusToFahrenheit(temp);
  return temp;
}

void Midea_setRaw(const uint64_t new_code, const decode_type_t protocol, state_ir_t *ir_state) 
{
    MideaProtocol _;
    _.remote_state = new_code;

    ir_state->protocol = MIDEA;
    ir_state->power = _.Power;
    ir_state->celsius = !_.useFahrenheit;
    ir_state->mode = toCommonMode(_.Mode);
    ir_state->fanspeed = toCommonFanSpeed(_.Fan);
    ir_state->degrees = getTemp(_, ir_state->celsius);
}

static void setMode(MideaProtocol *_, const uint8_t mode) {
  switch (mode) {
    case kMideaACAuto:
    case kMideaACCool:
    case kMideaACHeat:
    case kMideaACDry:
    case kMideaACFan:
      _->Mode = mode;
      break;
    default:
      _->Mode = kMideaACAuto;
  }
}

static void setFan(MideaProtocol *_, const uint8_t fan) {
  _->Fan = (fan > kMideaACFanHigh) ? kMideaACFanAuto : fan;
}

static void setTemp(MideaProtocol *_, const uint8_t temp, const bool useCelsius) {
  uint8_t max_temp = kMideaACMaxTempF;
  uint8_t min_temp = kMideaACMinTempF;
  if (useCelsius) {
    max_temp = kMideaACMaxTempC;
    min_temp = kMideaACMinTempC;
  }
  uint8_t new_temp = min(max_temp, max(min_temp, temp));
  if (!_->useFahrenheit && !useCelsius)  // Native is in C, new_temp is in F
    new_temp = fahrenheitToCelsius(new_temp) - kMideaACMinTempC;
  else if (_->useFahrenheit && useCelsius)  // Native is in F, new_temp is in C
    new_temp = celsiusToFahrenheit(new_temp) - kMideaACMinTempF;
  else  // Native and desired are the same units.
    new_temp -= min_temp;
  // Set the actual data.
  _->Temp = new_temp;
}

static void sendMidea(uint64_t data, uint16_t nbits, uint16_t repeat) {
  if (nbits % 8 != 0) return;  // nbits is required to be a multiple of 8.
    vTaskEnterCritical();
  // Set IR carrier frequency
  enableIROut(38, 50);

  for (uint16_t r = 0; r <= repeat; r++) {
    // The protocol sends the message, then follows up with an entirely
    // inverted payload.
    for (size_t inner_loop = 0; inner_loop < 2; inner_loop++) {
      // Header
      mark(kMideaHdrMark);
      space(kMideaHdrSpace);
      // Data
      //   Break data into byte segments, starting at the Most Significant
      //   Byte. Each byte then being sent normal, then followed inverted.
      for (uint16_t i = 8; i <= nbits; i += 8) {
        // Grab a bytes worth of data.
        uint8_t segment = (data >> (nbits - i)) & 0xFF;
        sendData(kMideaBitMark, kMideaOneSpace, kMideaBitMark, kMideaZeroSpace,
                 segment, 8, true);
      }
      // Footer
      mark(kMideaBitMark);
      space(kMideaMinGap);  // Pause before repeating

      // Invert the data for the 2nd phase of the message.
      // As we get called twice in the inner loop, we will always revert
      // to the original 'data' state.
      data = ~data;
    }
    space(kDefaultMessageGap);
  }
  vTaskExitCritical();
}

void Midea_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);
    MideaProtocol _;
    // reset
    _.remote_state = 0xA1826FFFFF62;

    setMode(&_, state_ir.mode);
    setFan(&_, state_ir.fanspeed);
    _.useFahrenheit = false;
    setTemp(&_, (uint8_t)state_ir.degrees, true);
    _.Power = state_ir.power;

    _.Sum = calcChecksum(_.remote_state);

    sendMidea(_.remote_state, kMideaBits, kNoRepeat);
}