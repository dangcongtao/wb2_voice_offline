#include "ir_Hitachi.h"

#include <blog.h>
#include "app_config.h"
#include "IRrecv.h"
#include "IRremoteWB2.h"
#include "IRsend.h"
#include "IRutils.h"

// Constants
#define kHitachiAcHdrMark       3300
#define kHitachiAcHdrSpace      1700
#define kHitachiAc1HdrMark      3400
#define kHitachiAc1HdrSpace     3400
#define kHitachiAcBitMark       400
#define kHitachiAcOneSpace      1250
#define kHitachiAcZeroSpace     500
#define kHitachiAcMinGap        kDefaultMessageGap  // Just a guess.

// Support for HitachiAc424 protocol
#define kHitachiAc424LdrMark    29784   // Leader
#define kHitachiAc424LdrSpace   49290   // Leader
#define kHitachiAc424HdrMark    3416    // Header
#define kHitachiAc424HdrSpace   1604    // Header
#define kHitachiAc424BitMark    463
#define kHitachiAc424OneSpace   1208
#define kHitachiAc424ZeroSpace  372

// Support for HitachiAc3 protocol
#define kHitachiAc3HdrMark      3400    // Header
#define kHitachiAc3HdrSpace     1660    // Header
#define kHitachiAc3BitMark      460
#define kHitachiAc3OneSpace     1250
#define kHitachiAc3ZeroSpace    410

union HitachiProtocol hitachi_protocol;
union Hitachi1Protocol hitachi1_protocol;
static bool prev_hitachi1_power = true;
Hitachi424Protocol hitachi424_protocol;
Hitachi424Protocol hitachi344_protocol;
static uint8_t _previoustemp = 0; 

/* add function */
static void HitachiAC_setTemp(const uint8_t celsius);
static void HitachiAC_setFan(const uint8_t speed);
static uint8_t HitachiAC_getFan(void);

static bool HitachiAC_validChecksum(const uint8_t state[], const uint16_t length);
static bool HitachiAC1_validChecksum(const uint8_t state[], const uint16_t length);
static bool HitachiAC3_hasInvertedStates(uint8_t state[],
                                     const uint16_t length);

static void HitachiAC1_setFan(const uint8_t speed/*force*/);
static void HitachiAC1_setSleep(uint8_t mode);
static uint8_t HitachiAC1_getTemp(void);
static void HitachiAC1_setTemp(const uint8_t celsius);
static uint8_t HitachiAC1_getMode(void);
static bool HitachiAC1_getPower(void);
static uint8_t HitachiAC1_getFan(void);
static uint8_t HitachiAC1_getModel(void);
static void HitachiAC1_setPowerToggle(const bool on);

static state_ir_t HitachiAC1_toCommon(void);

/* AC 424 */
static state_ir_t HitachiAC424_toCommon(void);

/* aC 434 */
static void HitachiAC424_setTemp(const uint8_t celsius, bool setPrevious);
static void HitachiAC424_setPower(const bool on);
static void HitachiAC424_setMode(const uint8_t mode);
static void HitachiAC424_setFan(const uint8_t speed);
static void HitachiAC424_setButton(const uint8_t button);

static bool HitachiAC344_getPower(void);
static void HitachiAC344_setPower(const bool on);

static uint8_t HitachiAC344_getTemp(void);
static void HitachiAC344_setTemp(const uint8_t celsius, bool setPrevious);

static uint8_t HitachiAC344_getMode(void);
static void HitachiAC344_setMode(const uint8_t mode);

static uint8_t HitachiAC344_getFan(void);
static void HitachiAC344_setFan(const uint8_t speed);

static void HitachiAC344_setButton(const uint8_t button);
state_ir_t HitachiAC344_toCommon(void);

#if (DECODE_HITACHI_AC || DECODE_HITACHI_AC1 || DECODE_HITACHI_AC2 || \
     DECODE_HITACHI_AC344)
/// Decode the supplied Hitachi A/C message.
/// Status: STABLE / Expected to work.
/// @param[in,out] results Ptr to the data to decode & where to store the result
/// @param[in] offset The starting index to use when attempting to decode the
///   raw data. Typically/Defaults to kStartOffset.
/// @param[in] nbits The number of data bits to expect.
///   Typically kHitachiAcBits, kHitachiAc1Bits, kHitachiAc2Bits,
///   kHitachiAc344Bits
/// @param[in] strict Flag indicating if we should perform strict matching.
/// @param[in] MSBfirst Is the data per byte stored in MSB First (true) or
///   LSB First order(false)?
/// @return True if it can decode it, false if it can't.
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/417
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/453
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1134
bool decodeHitachiAC(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict,
                             const bool MSBfirst)
{
    const uint8_t k_tolerance = _tolerance + 5;

    if (strict) 
    {
        switch (nbits) 
        {
            case kHitachiAcBits:
            case kHitachiAc1Bits:
            case kHitachiAc2Bits:
            case kHitachiAc264Bits:
            case kHitachiAc344Bits:
            break;  // Okay to continue.
            default:
            return false;  // Not strictly a Hitachi message.
        }
    }
    uint16_t hmark;
    uint32_t hspace;
    if (nbits == kHitachiAc1Bits) 
    {
        hmark = kHitachiAc1HdrMark;
        hspace = kHitachiAc1HdrSpace;
    }
    else
    {
        /* careful with this */
        hmark = kHitachiAcHdrMark;
        hspace = kHitachiAcHdrSpace;
    }
    // Match Header + Data + Footer
    if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    hmark-600, hmark+600,
                    hspace-500, hspace+500,
                    kHitachiAcBitMark-300, kHitachiAcBitMark+300,
                    kHitachiAcOneSpace-500, kHitachiAcOneSpace+500,
                    kHitachiAcBitMark-300, kHitachiAcBitMark+300,
                    kHitachiAcZeroSpace-300, kHitachiAcZeroSpace+300,
                    kHitachiAcBitMark-300, kHitachiAcBitMark+300,
                    kHitachiAcMinGap-25000, kHitachiAcMinGap+25000,
                    true, k_tolerance, kMarkExcess, MSBfirst))
    {
        // blog_error("%s: Header data footer fail ", __func__);
        // os_trace("%s: Header data footer fail %d\r\n", __func__, nbits);
        return false;
    }

    // Compliance
    if (strict) 
    {
        // if (nbits / 8 == kHitachiAcStateLength &&
        //     !HitachiAC_validChecksum(results->state, kHitachiAcStateLength))
        // {
        //     return false;
        // }
        // if (nbits / 8 == kHitachiAc1StateLength &&
        //     !HitachiAC1_validChecksum(results->state, kHitachiAc1StateLength))
        // {
        //     return false;
        // }
        // if (nbits / 8 == kHitachiAc344StateLength &&
        //     !HitachiAC3_hasInvertedStates(results->state,
        //                                         kHitachiAc344StateLength))
        // {
        //     return false;
        // }
        const uint16_t nbytes = nbits / 8;
        switch (nbytes) {
        case kHitachiAcStateLength:
            if (!HitachiAC_validChecksum(results->state, nbytes)){
                os_trace("%s HitachiAC_validChecksum invalid\r\n", __func__);
                return false;
            }
            break;
        case kHitachiAc1StateLength:
            if (!HitachiAC1_validChecksum(results->state, nbytes)){
                os_trace("%s HitachiAC1_validChecksum invalid\r\n", __func__);
                return false;
            }
            break;
        case kHitachiAc264StateLength:
        case kHitachiAc344StateLength:
            if (!HitachiAC3_hasInvertedStates(results->state, nbytes)) {
                os_trace("%s HitachiAC3_hasInvertedStates invalid\r\n", __func__);
                return false;
            }
            break;
        }
    }

    // Success
    switch (nbits) {
    case kHitachiAc1Bits:
        results->decode_type = HITACHI_AC1;
        os_trace("%s success decode_type: HITACHI_AC1\r\n", __func__);
        break;
    case kHitachiAc2Bits:
        results->decode_type = HITACHI_AC2;
        os_trace("%s success decode_type: HITACHI_AC2\r\n", __func__);
        break;
    case kHitachiAc264Bits:
        results->decode_type = HITACHI_AC264;
        os_trace("%s success decode_type: HITACHI_AC264\r\n", __func__);
        break;
    case kHitachiAc344Bits:
        results->decode_type = HITACHI_AC344;
        os_trace("%s success decode_type: HITACHI_AC344\r\n", __func__);
        break;
    case kHitachiAcBits:
    default:
        results->decode_type = HITACHI_AC;
        os_trace("%s success decode_type: HITACHI_AC\r\n", __func__);
    }

    results->bits = nbits;
    // No need to record the state as we stored it as we decoded it.
    // As we use result->state, we don't record value, address, or command as it
    // is a union data type.
    return true;
}
#endif  // (DECODE_HITACHI_AC || DECODE_HITACHI_AC1 || DECODE_HITACHI_AC2 ||
        //  DECODE_HITACHI_AC344)

/// Class constructor
/// @param[in] pin GPIO to be used when sending.
/// @param[in] inverted Is the output signal to be inverted?
/// @param[in] use_modulation Is frequency modulation to be used?
// HitachiAC_IRHitachiAc(const uint16_t pin, const bool inverted,
//                          const bool use_modulation)
//     : _irsend(pin, inverted, use_modulation) { stateReset(); }

/// Reset the internal state to a fixed known good state.
void HitachiAC_stateReset(void)
{
    hitachi_protocol.raw[0] = 0x80;
    hitachi_protocol.raw[1] = 0x08;
    hitachi_protocol.raw[2] = 0x0C;
    hitachi_protocol.raw[3] = 0x02;
    hitachi_protocol.raw[4] = 0xFD;
    hitachi_protocol.raw[5] = 0x80;
    hitachi_protocol.raw[6] = 0x7F;
    hitachi_protocol.raw[7] = 0x88;
    hitachi_protocol.raw[8] = 0x48;
    hitachi_protocol.raw[9] = 0x10;
    for (uint8_t i = 10; i < kHitachiAcStateLength; i++) hitachi_protocol.raw[i] = 0x00;
    hitachi_protocol.raw[14] = 0x60;
    hitachi_protocol.raw[15] = 0x60;
    hitachi_protocol.raw[24] = 0x80;
    HitachiAC_setTemp(23);
}

/// Set up hardware to be able to send a message.
void HitachiAC_begin(void)
{
    IRsend_begin();
}

/// Calculate the checksum for a given state.
/// @param[in] state The value to calc the checksum of.
/// @param[in] length The size/length of the state.
/// @return The calculated checksum value.
uint8_t HitachiAC_calcChecksum(const uint8_t state[], const uint16_t length) 
{
    uint8_t sum = 62;
    for (uint16_t i = 0; i < length - 1; i++)
    {
        sum -= reverseBits(state[i], 8);
    }
    return reverseBits(sum, 8);
}

/// Calculate and set the checksum values for the internal state.
/// @param[in] length The size/length of the state.
void HitachiAC_checksum(const uint16_t length)
{
    hitachi_protocol.Sum = HitachiAC_calcChecksum(hitachi_protocol.raw, length);
}

/// Verify the checksum is valid for a given state.
/// @param[in] state The array to verify the checksum of.
/// @param[in] length The length of the state array.
/// @return true, if the state has a valid checksum. Otherwise, false.
static bool HitachiAC_validChecksum(const uint8_t state[], const uint16_t length)
{
    if (length < 2) return true;  // Assume true for lengths that are too short.
    return (state[length - 1] == HitachiAC_calcChecksum(state, length));
}

/// Get a PTR to the internal state/code for this protocol.
/// @return PTR to a code for this protocol based on the current internal state.
uint8_t *HitachiAC_getRaw(void) 
{
    HitachiAC_checksum(kHitachiAcStateLength);
    return hitachi_protocol.raw;
}

/// Set the internal state from a valid code for this protocol.
/// @param[in] new_code A valid code for this protocol.
/// @param[in] length The length of the new_code array.
void HitachiAC_setRaw(const uint8_t new_code[], const uint16_t length)
{
    memcpy(hitachi_protocol.raw, new_code, min(length, kHitachiAcStateLength));
}

#if SEND_HITACHI_AC
/// Send the current internal state as an IR message.
/// @param[in] repeat Nr. of times the message will be repeated.
void HitachiAC_send(const uint16_t repeat)
{
    _irsend.sendHitachiAC(getRaw(), kHitachiAcStateLength, repeat);
}
#endif  // SEND_HITACHI_AC

/// Get the value of the current power setting.
/// @return true, the setting is on. false, the setting is off.
bool HitachiAC_getPower(void)
{
    return hitachi_protocol.Power;
}

/// Change the power setting.
/// @param[in] on true, the setting is on. false, the setting is off.
void HitachiAC_setPower(const bool on)
{
    hitachi_protocol.Power = on;
}

/// Change the power setting to On.
void HitachiAC_on(void)
{
    HitachiAC_setPower(true);
}

/// Change the power setting to Off.
void HitachiAC_off(void) 
{
    HitachiAC_setPower(false);
}

/// Get the operating mode setting of the A/C.
/// @return The current operating mode setting.
uint8_t HitachiAC_getMode(void) 
{
    return reverseBits(hitachi_protocol.Mode, 8);
}

/// Set the operating mode of the A/C.
/// @param[in] mode The desired operating mode.
void HitachiAC_setMode(const uint8_t mode)
{
    uint8_t newmode = mode;
    switch (mode) {
    // Fan mode sets a special temp.
    case kHitachiAcFan: HitachiAC_setTemp(64); break;
    case kHitachiAcAuto:
    case kHitachiAcHeat:
    case kHitachiAcCool:
    case kHitachiAcDry: break;
    default: newmode = kHitachiAcAuto;
    }
    hitachi_protocol.Mode = reverseBits(newmode, 8);
    if (mode != kHitachiAcFan) HitachiAC_setTemp(_previoustemp);
    HitachiAC_setFan(HitachiAC_getFan());  // Reset the fan speed after the mode change.
}

/// Get the current temperature setting.
/// @return The current setting for temp. in degrees celsius.
uint8_t HitachiAC_getTemp(void)
{
    return reverseBits(hitachi_protocol.Temp, 8) >> 1;
}

/// Set the temperature.
/// @param[in] celsius The temperature in degrees celsius.
static void HitachiAC_setTemp(const uint8_t celsius) {
    uint8_t temp;
    if (celsius != 64) _previoustemp = celsius;
    switch (celsius) {
    case 64:
        temp = celsius;
        break;
    default:
        temp = min(celsius, kHitachiAcMaxTemp);
        temp = max(temp, kHitachiAcMinTemp);
    }
    hitachi_protocol.Temp = reverseBits(temp << 1, 8);
    if (temp == kHitachiAcMinTemp)
    hitachi_protocol.raw[9] = 0x90;
    else
    hitachi_protocol.raw[9] = 0x10;
}

/// Get the current fan speed setting.
/// @return The current fan speed.
static uint8_t HitachiAC_getFan(void) 
{
    return reverseBits(hitachi_protocol.Fan, 8);
}

/// Set the speed of the fan.
/// @param[in] speed The desired setting.
static void HitachiAC_setFan(const uint8_t speed)
{
    uint8_t fanmin = kHitachiAcFanAuto;
    uint8_t fanmax = kHitachiAcFanHigh;
    switch (HitachiAC_getMode()) {
    case kHitachiAcDry:  // Only 2 x low speeds in Dry mode.
        fanmin = kHitachiAcFanLow;
        fanmax = kHitachiAcFanLow + 1;
        break;
    case kHitachiAcFan:
        fanmin = kHitachiAcFanLow;  // No Auto in Fan mode.
        break;
    }
    uint8_t newspeed = max(speed, fanmin);
    newspeed = min(newspeed, fanmax);
    hitachi_protocol.Fan = reverseBits(newspeed, 8);
}

/// Get the Vertical Swing setting of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool HitachiAC_getSwingVertical(void)
{
    return hitachi_protocol.SwingV;
}

/// Set the Vertical Swing setting of the A/C.
/// @param[in] on true, the setting is on. false, the setting is off.
void HitachiAC_setSwingVertical(const bool on) {
  hitachi_protocol.SwingV = on;
}

/// Get the Horizontal Swing setting of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool HitachiAC_getSwingHorizontal(void)
{
    return hitachi_protocol.SwingH;
}

/// Set the Horizontal Swing setting of the A/C.
/// @param[in] on true, the setting is on. false, the setting is off.
void HitachiAC_setSwingHorizontal(const bool on)
{
    hitachi_protocol.SwingH = on;
}

/// Convert a stdAc::opmode_t enum into its native mode.
/// @param[in] mode The enum to be converted.
/// @return The native equivalent of the enum.
uint8_t HitachiAC_convertMode(uint8_t mode) {
    switch (mode) {
    case opmode_kCool:
        return kHitachiAcCool;
    case opmode_kHeat:
        return kHitachiAcHeat;
    case opmode_kDry:
        return kHitachiAcDry;
    case opmode_kFan:
        return kHitachiAcFan;
    default:
        return kHitachiAcAuto;
    }
}

/// Convert a stdAc::fanspeed_t enum into it's native speed.
/// @param[in] speed The enum to be converted.
/// @return The native equivalent of the enum.
uint8_t HitachiAC_convertFan(uint8_t speed)
{
    switch (speed) {
    case fanspeed_kMin:
    case fanspeed_kLow:    return kHitachiAcFanLow;
    case fanspeed_kMedium: return kHitachiAcFanLow + 1;
    case fanspeed_kHigh:   return kHitachiAcFanHigh - 1;
    case fanspeed_kMax:    return kHitachiAcFanHigh;
    default:    return kHitachiAcFanAuto;
    }
}

/// Convert a native mode into its stdAc equivalent.
/// @param[in] mode The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
uint8_t HitachiAC_toCommonMode(const uint8_t mode) {
    switch (mode) {
    case kHitachiAcCool: return opmode_kCool;
    case kHitachiAcHeat: return opmode_kHeat;
    case kHitachiAcDry:  return opmode_kDry;
    case kHitachiAcFan:  return opmode_kFan;
    default:             return opmode_kAuto;
    }
}

/// Convert a native fan speed into its stdAc equivalent.
/// @param[in] speed The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
uint8_t HitachiAC_toCommonFanSpeed(const uint8_t speed) {
    switch (speed) {
    case kHitachiAcFanHigh:     return fanspeed_kMax;
    case kHitachiAcFanHigh - 1: return fanspeed_kHigh;
    case kHitachiAcFanLow + 1:  return fanspeed_kMedium;
    case kHitachiAcFanLow:      return fanspeed_kLow;
    default:                    return fanspeed_kAuto;
    }
}

/// Convert the current internal state into its stdAc::state_t equivalent.
/// @return The stdAc equivalent of the native settings.
state_ir_t HitachiAC_toCommon(void)
{
    state_ir_t result;
    result.protocol = HITACHI_AC;
    result.model = -1;  // No models used.
    result.power = hitachi_protocol.Power;
    result.mode = HitachiAC_toCommonMode(HitachiAC_getMode());
    result.celsius = true;
    result.degrees = HitachiAC_getTemp();
    result.fanspeed = HitachiAC_toCommonFanSpeed(HitachiAC_getFan());
    result.swingv = (hitachi_protocol.SwingV ? swingv_kAuto : swingv_kOff);
    result.swingh = (hitachi_protocol.SwingH ? swingh_kAuto : swingh_kOff);

    // Not supported.
    result.quiet = false;
    result.turbo = false;
    result.clean = false;
    result.econo = false;
    result.filter = false;
    result.light = false;
    result.beep = false;
    result.sleep = -1;
    result.clock = -1;
    return result;
}

/* no need this function */
/// Convert the current internal state into a human readable string.
/// @return A human readable string.
// String HitachiAC_toString(void)



/// Calculate the checksum for a given state.
/// @param[in] state The value to calc the checksum of.
/// @param[in] length The size/length of the state.
/// @return The calculated checksum value.
static uint8_t HitachiAC1_calcChecksum(const uint8_t state[], const uint16_t length)
{
    uint8_t sum = 0;
    for (uint16_t i = kHitachiAc1ChecksumStartByte; i < length - 1; i++) {
    sum += reverseBits(GETBITS8(state[i], kLowNibble, kNibbleSize),
                        kNibbleSize);
    sum += reverseBits(GETBITS8(state[i], kHighNibble, kNibbleSize),
                        kNibbleSize);
    }
    return reverseBits(sum, 8);
}

/// Verify the checksum is valid for a given state.
/// @param[in] state The array to verify the checksum of.
/// @param[in] length The length of the state array, Default value is kHitachiAc1StateLength.
/// @return true, if the state has a valid checksum. Otherwise, false.
static bool HitachiAC1_validChecksum(const uint8_t state[], const uint16_t length)
{
    if (length < 2) return true;  // Assume true for lengths that are too short.
    return (state[length - 1] == HitachiAC1_calcChecksum(state, length));
}

/// Check if every second byte of the state, after the fixed header
///   is inverted to the previous byte.
/// @param[in] state The state array to be checked.
/// @param[in] length The size of the state array.
/// @note This is this protocols integrity check.
static bool HitachiAC3_hasInvertedStates(uint8_t state[],
                                     const uint16_t length)
{
    return (length <= 3 || checkInvertedBytePairs(state + 3, length - 3));
}



/// Class constructor
/// @param[in] pin GPIO to be used when sending.
/// @param[in] inverted Is the output signal to be inverted?
/// @param[in] use_modulation Is frequency modulation to be used?
// HitachiAC1_IRHitachiAc1(const uint16_t pin, const bool inverted,
//                            const bool use_modulation)
//     : _irsend(pin, inverted, use_modulation) { stateReset(); }

// /// Reset the internal state to a fixed known good state.
// void HitachiAC1_stateReset(void) {
//   for (uint8_t i = 0; i < kHitachiAc1StateLength; i++) hitachi1_protocol.raw[i] = 0x00;
//   // Copy in a known good state.
//   hitachi1_protocol.raw[0] = 0xB2;
//   hitachi1_protocol.raw[1] = 0xAE;
//   hitachi1_protocol.raw[2] = 0x4D;
//   hitachi1_protocol.raw[3] = 0x91;
//   hitachi1_protocol.raw[4] = 0xF0;
//   hitachi1_protocol.raw[5] = 0xE1;
//   hitachi1_protocol.raw[6] = 0xA4;
//   hitachi1_protocol.raw[11] = 0x61;
//   hitachi1_protocol.raw[12] = 0x24;
// }

/// Set up hardware to be able to send a message.
void HitachiAC1_begin(void) 
{
    IRsend_begin();
}


void HitachiAC1_checksum(const uint16_t length)
{
    hitachi1_protocol.Sum = HitachiAC1_calcChecksum(hitachi1_protocol.raw, length);
}
/// Get a PTR to the internal state/code for this protocol.
/// @return PTR to a code for this protocol based on the current internal state.
uint8_t *HitachiAC1_getRaw(void)
{
    HitachiAC1_checksum(kHitachiAc1StateLength);
    return hitachi1_protocol.raw;
}

/// Set the internal state from a valid code for this protocol.
/// @param[in] new_code A valid code for this protocol.
/// @param[in] length The length of the new_code array.
void HitachiAC1_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
    state_ir_t temp;
    memcpy(hitachi1_protocol.raw, new_code, min(length, kHitachiAc1StateLength));

    // os_trace("power: %d \r\n", HitachiAC1_getPower());    
    // os_trace("temp: %d \r\n", HitachiAC1_getTemp());
    // os_trace("mode: %d \r\n", HitachiAC1_getMode());
    // os_trace("fan: %d \r\n", HitachiAC1_getFan());

    // os_trace("model: %d \r\n", HitachiAC1_getModel());

    memset(&temp, 0, sizeof(temp));
    *ir_state = HitachiAC1_toCommon();
}

void HitachiAC1_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
    state_ir_t temp;
    memset(&hitachi1_protocol, 0, sizeof(hitachi1_protocol));
    memcpy(hitachi1_protocol.raw, new_code, min(length, kHitachiAc1StateLength));
    if (hitachi1_protocol.PowerToggle) {
        prev_hitachi1_power = !prev_hitachi1_power;
    }
    ir_state->power = prev_hitachi1_power;
    ir_state->degrees = (float)HitachiAC1_getTemp();
    ir_state->mode = HitachiAC1_getMode();
    switch ((uint8_t)ir_state->mode)
    {
    case kHitachiAc1Dry:
        ir_state->mode = 2;
        break;
    
    case kHitachiAc1Heat:
        ir_state->mode = 3;
        break;
    
    case kHitachiAc1Cool:
        ir_state->mode = 1;
        break;
    
    case kHitachiAc1Fan:
        ir_state->mode = 4;
        break;
    
    case kHitachiAc1Auto:
        ir_state->mode = 0;
        break;
    
    default:
        break;
    }
    ir_state->fanspeed = HitachiAC1_getFan();
    switch ((uint8_t)ir_state->fanspeed)
    {
    case kHitachiAc1FanLow:
        ir_state->fanspeed = 1;
        break;
    
    case kHitachiAc1FanMed:
        ir_state->fanspeed = 2;
        break;
    
    case kHitachiAc1FanHigh:
        ir_state->fanspeed = 3;
        break;
    
    case kHitachiAc1FanAuto:
        ir_state->fanspeed = 0;
        break;
    
    default:
        break;
    }


    // os_trace("power: %d \r\n", HitachiAC1_getPower());    
    // os_trace("temp: %d \r\n", HitachiAC1_getTemp());
    // os_trace("mode: %d \r\n", HitachiAC1_getMode());
    // os_trace("fan: %d \r\n", HitachiAC1_getFan());

    // os_trace("model: %d \r\n", HitachiAC1_getModel());
}

#if SEND_HITACHI_AC
/// Send the current internal state as an IR message.
/// @param[in] repeat Nr. of times the message will be repeated.
void HitachiAC1_send(const uint16_t repeat) {
    _irsend.sendHitachiAC1(getRaw(), kHitachiAc1StateLength, repeat);
    // Clear the toggle bits as we have actioned them by sending them.
    setPowerToggle(false);
    setSwingToggle(false);
}
#endif  // SEND_HITACHI_AC

/// Get/Detect the model of the A/C.
/// @return The enum of the compatible model.
static uint8_t HitachiAC1_getModel(void)
{
    switch (hitachi1_protocol.Model) {
    case kHitachiAc1Model_B: return R_LT0541_HTA_B;
    default:                 return R_LT0541_HTA_A;
    }
}

/// Set the model of the A/C to emulate.
/// @param[in] model The enum of the appropriate model.
void HitachiAC1_setModel(const hitachi_ac1_remote_model_t model) {
    uint8_t value = 0;
    switch (model) {
    case R_LT0541_HTA_B:
        value = kHitachiAc1Model_B;
        break;
    default:
        value = kHitachiAc1Model_A;  // i.e. 'A' mode.
    }
    hitachi1_protocol.Model = value;
}

/// Get the value of the current power setting.
/// @return true, the setting is on. false, the setting is off.
static bool HitachiAC1_getPower(void)
{
    return hitachi1_protocol.Power;
}

/// Change the power setting.
/// @param[in] on true, the setting is on. false, the setting is off.
void HitachiAC1_setPower(const bool on)
{
    // If the power changes, set the power toggle bit.
    if (on != hitachi1_protocol.Power)
        HitachiAC1_setPowerToggle(true);
    hitachi1_protocol.Power = on;
}

/// Get the value of the current power toggle setting.
/// @return true, the setting is on. false, the setting is off.
bool HitachiAC1_getPowerToggle(void)
{
    return hitachi1_protocol.PowerToggle;
}

/// Change the power toggle setting.
/// @param[in] on true, the setting is on. false, the setting is off.
static void HitachiAC1_setPowerToggle(const bool on)
{
    hitachi1_protocol.PowerToggle = on;
}

/// Change the power setting to On.
void HitachiAC1_on(void) { HitachiAC1_setPower(true); }

/// Change the power setting to Off.
void HitachiAC1_off(void) { HitachiAC1_setPower(false); }

/// Get the operating mode setting of the A/C.
/// @return The current operating mode setting.
static uint8_t HitachiAC1_getMode(void)
{
    return hitachi1_protocol.Mode;
}

/// Set the operating mode of the A/C.
/// @param[in] mode The desired operating mode.
void HitachiAC1_setMode(const uint8_t mode)
{
    switch (mode) {
    case kHitachiAc1Auto: {
        HitachiAC1_setTemp(kHitachiAc1TempAuto);
        hitachi1_protocol.Mode = kHitachiAc1Auto;
    } break;
        // FALL THRU
    case kHitachiAc1Fan:
    case kHitachiAc1Heat:
    case kHitachiAc1Cool:
    case kHitachiAc1Dry:
        hitachi1_protocol.Mode = mode;
        break;
    default:
        HitachiAC1_setTemp(kHitachiAc1TempAuto);
        hitachi1_protocol.Mode = kHitachiAc1Auto;
        break;
    }
    HitachiAC1_setSleep(hitachi1_protocol.Sleep);  // Correct the sleep mode if required.
    HitachiAC1_setFan(hitachi1_protocol.Fan);  // Correct the fan speed if required.
}

/// Get the current temperature setting.
/// @return The current setting for temp. in degrees celsius.
static uint8_t HitachiAC1_getTemp(void)
{
    return reverseBits(hitachi1_protocol.Temp, kHitachiAc1TempSize) + kHitachiAc1TempDelta;
}

/// Set the temperature.
/// @param[in] celsius The temperature in degrees celsius.
static void HitachiAC1_setTemp(const uint8_t celsius)
{
    if (hitachi1_protocol.Mode == kHitachiAc1Auto)
        return;  // Can't change temp in Auto mode.
    uint8_t temp = min(celsius, kHitachiAcMaxTemp);
    temp = max(temp, kHitachiAcMinTemp);
    temp -= kHitachiAc1TempDelta;
    temp = reverseBits(temp, kHitachiAc1TempSize);
    hitachi1_protocol.Temp = temp;
}

/// Get the current fan speed setting.
/// @return The current fan speed.
static uint8_t HitachiAC1_getFan(void)
{
    return hitachi1_protocol.Fan;
}

/// Set the speed of the fan.
/// @param[in] speed The desired setting.
/// @param[in] force Deprecated
static void HitachiAC1_setFan(const uint8_t speed/*force*/)
{
    // restrictions
    switch (hitachi1_protocol.Mode) {
    case kHitachiAc1Dry:
        hitachi1_protocol.Fan = kHitachiAc1FanLow;  // Dry is locked to Low speed.
        return;
    case kHitachiAc1Auto:
        hitachi1_protocol.Fan = kHitachiAc1FanAuto;  // Auto is locked to Auto speed.
        return;
    case kHitachiAc1Heat:
    case kHitachiAc1Fan:  // Auto speed not allowed in these modes.
        if (speed == kHitachiAc1FanAuto || hitachi1_protocol.Fan == kHitachiAc1FanAuto)
            hitachi1_protocol.Fan = kHitachiAc1FanLow;
        return;
    }

    switch (speed) {
    case kHitachiAc1FanAuto:
    case kHitachiAc1FanHigh:
    case kHitachiAc1FanMed:
    case kHitachiAc1FanLow:
        hitachi1_protocol.Fan = speed;
        break;
    default: hitachi1_protocol.Fan = kHitachiAc1FanAuto;
    }
}

/// Get the Swing Toggle setting of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool HitachiAC1_getSwingToggle(void)
{
    return hitachi1_protocol.SwingToggle;
}

/// Set the Swing toggle setting of the A/C.
/// @param[in] toggle true, the setting is on. false, the setting is off.
void HitachiAC1_setSwingToggle(const bool toggle)
{
    hitachi1_protocol.SwingToggle = toggle;
}

/// Get the Vertical Swing setting of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool HitachiAC1_getSwingV(void)
{
    return hitachi1_protocol.SwingV;
}

/// Set the Vertical Swing setting of the A/C.
/// @param[in] on true, the setting is on. false, the setting is off.
void HitachiAC1_setSwingV(const bool on)
{
    hitachi1_protocol.SwingV = on;
}

/// Get the Horizontal Swing setting of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool HitachiAC1_getSwingH(void)
{
    return hitachi1_protocol.SwingH;
}

/// Set the Horizontal Swing setting of the A/C.
/// @param[in] on true, the setting is on. false, the setting is off.
void HitachiAC1_setSwingH(const bool on)
{
    hitachi1_protocol.SwingH = on;
}

/// Get the Sleep setting of the A/C.
/// @return The currently configured sleep mode.
/// @note Sleep modes only available in Auto & Cool modes, otherwise it's off.
uint8_t HitachiAC1_getSleep(void)
{
    return hitachi1_protocol.Sleep;
}

/// Set the Sleep setting of the A/C.
/// @param[in] mode The mode of sleep to set the A/C to.
/// @note Sleep modes only available in Auto & Cool modes, otherwise it's off.
static void HitachiAC1_setSleep(uint8_t mode)
{
    switch (hitachi1_protocol.Mode) {
    case kHitachiAc1Auto:
    case kHitachiAc1Cool:
        hitachi1_protocol.Sleep = min(mode, kHitachiAc1Sleep4);
        break;
    default:
        hitachi1_protocol.Sleep = kHitachiAc1SleepOff;
    }
}

/// Set the On Timer time.
/// @param[in] mins The time expressed in total number of minutes.
void HitachiAC1_setOnTimer(const uint16_t mins)
{
    const uint16_t mins_lsb = reverseBits(mins, kHitachiAc1TimerSize);
    hitachi1_protocol.OnTimerLow = GETBITS16(mins_lsb, 8, 8);
    hitachi1_protocol.OnTimerHigh = GETBITS16(mins_lsb, 0, 8);
}

/// Get the On Timer vtime of the A/C.
/// @return Nr of minutes the timer is set to.
uint16_t HitachiAC1_getOnTimer(void)
{
    return reverseBits(
        (hitachi1_protocol.OnTimerLow << 8) | hitachi1_protocol.OnTimerHigh, kHitachiAc1TimerSize);
}

/// Set the Off Timer time.
/// @param[in] mins The time expressed in total number of minutes.
void HitachiAC1_setOffTimer(const uint16_t mins)
{
    const uint16_t mins_lsb = reverseBits(mins, kHitachiAc1TimerSize);
    hitachi1_protocol.OffTimerLow = GETBITS16(mins_lsb, 8, 8);
    hitachi1_protocol.OffTimerHigh = GETBITS16(mins_lsb, 0, 8);
}

/// Get the Off Timer vtime of the A/C.
/// @return Nr of minutes the timer is set to.
uint16_t HitachiAC1_getOffTimer(void)
{
    return reverseBits(
        (hitachi1_protocol.OffTimerLow << 8) | hitachi1_protocol.OffTimerHigh, kHitachiAc1TimerSize);
}

/// Convert a stdAc::opmode_t enum into its native mode.
/// @param[in] mode The enum to be converted.
/// @return The native equivalent of the enum.
uint8_t HitachiAC1_convertMode(uint8_t mode) {
    switch (mode) {
    case opmode_kCool:  return kHitachiAc1Cool;
    case opmode_kHeat:  return kHitachiAc1Heat;
    case opmode_kDry:   return kHitachiAc1Dry;
    case opmode_kFan:   return kHitachiAc1Fan;
    default:            return kHitachiAc1Auto;
    }
}

/// Convert a stdAc::fanspeed_t enum into it's native speed.
/// @param[in] speed The enum to be converted.
/// @return The native equivalent of the enum.
uint8_t HitachiAC1_convertFan(uint8_t speed) {
    switch (speed) {
    case fanspeed_kMin:
    case fanspeed_kLow:     return kHitachiAc1FanLow;
    case fanspeed_kMedium:  return kHitachiAc1FanMed;
    case fanspeed_kHigh:
    case fanspeed_kMax:     return kHitachiAc1FanHigh;
    default:                return kHitachiAc1FanAuto;
    }
}

/// Convert a native mode into its stdAc equivalent.
/// @param[in] mode The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
uint8_t HitachiAC1_toCommonMode(const uint8_t mode)
{
    switch (mode) {
    case kHitachiAc1Cool: return opmode_kCool;
    case kHitachiAc1Heat: return opmode_kHeat;
    case kHitachiAc1Dry:  return opmode_kDry;
    case kHitachiAc1Fan:  return opmode_kFan;
    default:              return opmode_kAuto;
    }
}

/// Convert a native fan speed into its stdAc equivalent.
/// @param[in] speed The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
uint8_t HitachiAC1_toCommonFanSpeed(const uint8_t speed)
{
    switch (speed) {
    case kHitachiAc1FanHigh:     return fanspeed_kMax;
    case kHitachiAc1FanMed:      return fanspeed_kMedium;
    case kHitachiAc1FanLow:      return fanspeed_kLow;
    default:                     return fanspeed_kAuto;
    }
}

/// Convert the current internal state into its stdAc::state_t equivalent.
/// @return The stdAc equivalent of the native settings.
static state_ir_t HitachiAC1_toCommon(void)
{
    state_ir_t result;
    result.protocol = HITACHI_AC1;
    result.model = HitachiAC1_getModel();
    result.power = hitachi1_protocol.Power;
    result.mode = HitachiAC1_toCommonMode(hitachi1_protocol.Mode);
    result.celsius = true;
    result.degrees = HitachiAC1_getTemp();
    result.fanspeed = HitachiAC1_toCommonFanSpeed(hitachi1_protocol.Fan);
    result.swingv = hitachi1_protocol.SwingV ? swingv_kAuto :
                                swingv_kOff;
    result.swingh = hitachi1_protocol.SwingH ? swingh_kAuto :
                                swingh_kOff;
    result.sleep = hitachi1_protocol.Sleep ? 0 : -1;
    
    // Not supported.
    result.quiet = false;
    result.turbo = false;
    result.clean = false;
    result.econo = false;
    result.filter = false;
    result.light = false;
    result.beep = false;
    result.clock = -1;
    return result;
}




#if DECODE_HITACHI_AC424
/// Decode the supplied Hitachi 53-byte/424-bit A/C message.
/// Status: STABLE / Reported as working.
/// @param[in,out] results Ptr to the data to decode & where to store the result
/// @param[in] offset The starting index to use when attempting to decode the
///   raw data. Typically/Defaults to kStartOffset.
/// @param[in] nbits The number of data bits to expect.
/// @param[in] strict Flag indicating if we should perform strict matching.
/// @return True if it can decode it, false if it can't.
/// @note This protocol is almost exactly the same as HitachiAC2 except this
///   variant has a leader section as well, and subtle timing differences.
///   It is also in LSBF order (per byte), rather than MSBF order.
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/973
/// @see (Japanese Manual) https://kadenfan.hitachi.co.jp/support/raj/item/docs/ras_aj22h_a_tori.pdf
bool decodeHitachiAC424(decode_results *results, uint16_t offset,
                                const uint16_t nbits,
                                const bool strict) 
{
    if (results->rawlen < 2 * nbits + kHeader + kHeader + kFooter - 1 + offset)
        return false;  // Too short a message to match.
    if (strict && nbits != kHitachiAc424Bits)
        return false;

    uint16_t used;

    // Leader
    if (!matchMark(results->rawbuf[offset++], kHitachiAc424LdrMark, kUseDefTol, kMarkExcess))
        return false;
    if (!matchSpace(results->rawbuf[offset++], kHitachiAc424LdrSpace, kUseDefTol, kMarkExcess))
        return false;

    // Header + Data + Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                        results->rawlen - offset, nbits,
                        kHitachiAc424HdrMark-300, kHitachiAc424HdrMark+300,
                        kHitachiAc424HdrSpace-300, kHitachiAc424HdrSpace+300,
                        kHitachiAc424BitMark-363, kHitachiAc424BitMark+300, 
                        kHitachiAc424OneSpace-300, kHitachiAc424OneSpace+442,
                        kHitachiAc424BitMark-363, kHitachiAc424BitMark+300, 
                        kHitachiAc424ZeroSpace-300, kHitachiAc424ZeroSpace+428,
                        kHitachiAc424BitMark-300, kHitachiAc424BitMark+300,
                        kHitachiAcMinGap-25000, kHitachiAcMinGap+25000,
                        true, kUseDefTol, 0, false);
    if (used == 0) 
    {
        blog_error("%s: match fail\r\n", __func__);
        return false;  // We failed to find any data.
    }
    os_trace("%s success\r\n", __func__);
    // Success
    results->decode_type = HITACHI_AC424;
    results->bits = nbits;
    return true;
}
#endif  // DECODE_HITACHI_AC424


/// Class constructor
/// @param[in] pin GPIO to be used when sending.
/// @param[in] inverted Is the output signal to be inverted?
/// @param[in] use_modulation Is frequency modulation to be used?
// HitachiAC424_IRHitachiAc424(const uint16_t pin, const bool inverted,
//                          const bool use_modulation)
//     : _irsend(pin, inverted, use_modulation) { stateReset(); }

/// Reset the internal state to a fixed known good state.
/// @note Reset to auto fan, cooling, 23° Celsius
void HitachiAC424_stateReset(void) {
    for (uint8_t i = 0; i < kHitachiAc424StateLength; i++)
        hitachi424_protocol.raw[i] = 0x00;

    hitachi424_protocol.raw[0]  = 0x01;
    hitachi424_protocol.raw[1]  = 0x10;
    hitachi424_protocol.raw[3]  = 0x40;
    hitachi424_protocol.raw[5]  = 0xFF;
    hitachi424_protocol.raw[7]  = 0xCC;
    hitachi424_protocol.raw[33] = 0x80;
    hitachi424_protocol.raw[35] = 0x03;
    hitachi424_protocol.raw[37] = 0x01;
    hitachi424_protocol.raw[39] = 0x88;
    hitachi424_protocol.raw[45] = 0xFF;
    hitachi424_protocol.raw[47] = 0xFF;
    hitachi424_protocol.raw[49] = 0xFF;
    hitachi424_protocol.raw[51] = 0xFF;

    HitachiAC424_setTemp(23, true);
    HitachiAC424_setPower(true);
    HitachiAC424_setMode(kHitachiAc424Cool);
    HitachiAC424_setFan(kHitachiAc424FanAuto);
}

/// Update the internal consistency check for the protocol.
void HitachiAC424_setInvertedStates(void)
{
    invertBytePairs(hitachi424_protocol.raw + 3, kHitachiAc424StateLength - 3);
}

/// Set up hardware to be able to send a message.
// void HitachiAC424_begin(void) { _irsend.begin(); }

/// Get a PTR to the internal state/code for this protocol.
/// @return PTR to a code for this protocol based on the current internal state.
uint8_t *HitachiAC424_getRaw(void) {
    HitachiAC424_setInvertedStates();
    return hitachi424_protocol.raw;
}

/// Set the internal state from a valid code for this protocol.
/// @param[in] new_code A valid code for this protocol.
/// @param[in] length The length of the new_code array.
void HitachiAC424_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
    memcpy(hitachi424_protocol.raw, new_code, min(length, kHitachiAc424StateLength));
    *ir_state = HitachiAC424_toCommon();
}

bool HitachiAC424_getPower(void);

void HitachiAC424_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
    memcpy(hitachi424_protocol.raw, new_code, min(length, kHitachiAc424StateLength));
    ir_state->protocol = HITACHI_AC424;
    ir_state->model = -1;  // No models used.
    ir_state->power = HitachiAC424_getPower();
    ir_state->mode = hitachi424_protocol.Mode;
    switch ((uint8_t)ir_state->mode)
    {
    case kHitachiAc424Fan:
        ir_state->mode = 4;
        break;
    
    case kHitachiAc424Cool:
        ir_state->mode = 1;
        break;
    
    case kHitachiAc424Dry:
        ir_state->mode = 2;
        break;
    
    case kHitachiAc424Heat:
        ir_state->mode = 3;
        break;
    
    default:
        break;
    }
    
    ir_state->degrees = hitachi424_protocol.Temp;
    ir_state->fanspeed = hitachi424_protocol.Fan;
    switch ((uint8_t)ir_state->fanspeed)
    {
    case kHitachiAc424FanMin:
        ir_state->fanspeed = 1;
        break;
    
    case kHitachiAc424FanMedium:
        ir_state->fanspeed = 2;
        break;
    
    case kHitachiAc424FanMax:
        ir_state->fanspeed = 3;
        break;
    
    case kHitachiAc424FanAuto:
        ir_state->fanspeed = 0;
        break;
    
    default:
        break;
    }
}

#if SEND_HITACHI_AC424
/// Send the current internal state as an IR message.
/// @param[in] repeat Nr. of times the message will be repeated.
void HitachiAC424_send(const uint16_t repeat) {
  _irsend.sendHitachiAc424(getRaw(), kHitachiAc424StateLength, repeat);
}
#endif  // SEND_HITACHI_AC424

/// Get the value of the current power setting.
/// @return true, the setting is on. false, the setting is off.
bool HitachiAC424_getPower(void) 
{
    return hitachi424_protocol.Power == kHitachiAc424PowerOn;
}

/// Change the power setting.
/// @param[in] on true, the setting is on. false, the setting is off.
static void HitachiAC424_setPower(const bool on)
{
    HitachiAC424_setButton(kHitachiAc424ButtonPowerMode);
    hitachi424_protocol.Power = (on ? kHitachiAc424PowerOn : kHitachiAc424PowerOff);
}

/// Change the power setting to On.
void HitachiAC424_on(void) { HitachiAC424_setPower(true); }

/// Change the power setting to Off.
void HitachiAC424_off(void) { HitachiAC424_setPower(false); }

/// Get the operating mode setting of the A/C.
/// @return The current operating mode setting.
uint8_t HitachiAC424_getMode(void)
{
    return hitachi424_protocol.Mode;
}

/// Set the operating mode of the A/C.
/// @param[in] mode The desired operating mode.
static void HitachiAC424_setMode(const uint8_t mode)
{
    uint8_t newMode = mode;
    switch (mode) {
    // Fan mode sets a special temp.
    case kHitachiAc424Fan: {
        HitachiAC424_setTemp(kHitachiAc424FanTemp, false); 
    } break;
    case kHitachiAc424Heat:
    case kHitachiAc424Cool:
    case kHitachiAc424Dry: break;
    default: newMode = kHitachiAc424Cool;
    }
    hitachi424_protocol.Mode = newMode;
    if (newMode != kHitachiAc424Fan)
        HitachiAC424_setTemp(_previoustemp, true);
    HitachiAC424_setFan(hitachi424_protocol.Fan);  // Reset the fan speed after the mode change.
    HitachiAC424_setButton(kHitachiAc424ButtonPowerMode);
}

/// Get the current temperature setting.
/// @return The current setting for temp. in degrees celsius.
uint8_t HitachiAC424_getTemp(void)
{
    return hitachi424_protocol.Temp;
}

/* port done */
/// Set the temperature.
/// @param[in] celsius The temperature in degrees celsius.
/// @param[in] setPrevious true, remember this if we change mode. false, don't.
static void HitachiAC424_setTemp(const uint8_t celsius, bool setPrevious)
{
    uint8_t temp;
    temp = min(celsius, kHitachiAc424MaxTemp);
    temp = max(temp, kHitachiAc424MinTemp);
    hitachi424_protocol.Temp = temp;
    if (_previoustemp > temp)
        HitachiAC424_setButton(kHitachiAc424ButtonTempDown);
    else if (_previoustemp < temp)
        HitachiAC424_setButton(kHitachiAc424ButtonTempUp);
    if (setPrevious) _previoustemp = temp;
}

/* port done */
/// Get the current fan speed setting.
/// @return The current fan speed.
uint8_t HitachiAC424_getFan(void)
{
    return hitachi424_protocol.Fan;
}

/* port done */
/// Set the speed of the fan.
/// @param[in] speed The desired setting.
static void HitachiAC424_setFan(const uint8_t speed)
{
    uint8_t newSpeed = max(speed, kHitachiAc424FanMin);
    uint8_t fanMax = kHitachiAc424FanMax;

    // Only 2 x low speeds in Dry mode or Auto
    if (hitachi424_protocol.Mode == kHitachiAc424Dry && speed == kHitachiAc424FanAuto) {
    fanMax = kHitachiAc424FanAuto;
    } else if (hitachi424_protocol.Mode == kHitachiAc424Dry) {
    fanMax = kHitachiAc424FanMaxDry;
    } else if (hitachi424_protocol.Mode == kHitachiAc424Fan && speed == kHitachiAc424FanAuto) {
    // Fan Mode does not have auto. Set to safe low
    newSpeed = kHitachiAc424FanMin;
    }

    newSpeed = min(newSpeed, fanMax);
    // Handle the setting the button value if we are going to change the value.
    if (newSpeed != hitachi424_protocol.Fan)
        HitachiAC424_setButton(kHitachiAc424ButtonFan);
    // Set the values
    hitachi424_protocol.Fan = newSpeed;
    hitachi424_protocol.raw[9] = 0x92;
    hitachi424_protocol.raw[29] = 0x00;

    // When fan is at min/max, additional bytes seem to be set
    if (newSpeed == kHitachiAc424FanMin)
        hitachi424_protocol.raw[9] = 0x98;
    if (newSpeed == kHitachiAc424FanMax) {
        hitachi424_protocol.raw[9] = 0xA9;
        hitachi424_protocol.raw[29] = 0x30;
    }
}

/// Get the Button/Command setting of the A/C.
/// @return The value of the button/command that was pressed.
uint8_t HitachiAC424_getButton(void)
{
    return hitachi424_protocol.Button;
}

/* port done */
/// Set the Button/Command pressed setting of the A/C.
/// @param[in] button The value of the button/command that was pressed.
static void HitachiAC424_setButton(const uint8_t button)
{
    hitachi424_protocol.Button = button;
}

/* port done */
/// Set the Vertical Swing toggle setting of the A/C.
/// @param[in] on true, the setting is on. false, the setting is off.
/// @note The remote does not keep state of the vertical swing.
///   A byte is sent indicating the swing button is pressed on the remote
void HitachiAC424_setSwingVToggle(const bool on)
{
    uint8_t button = hitachi424_protocol.Button;  // Get the current button value.
    if (on)
        button = kHitachiAc424ButtonSwingV;  // Set the button to SwingV.
    else if (button == kHitachiAc424ButtonSwingV)  // Asked to unset it
    // It was set previous, so use Power as a default
        button = kHitachiAc424ButtonPowerMode;
    HitachiAC424_setButton(button);
}

/// Get the Vertical Swing toggle setting of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool HitachiAC424_getSwingVToggle(void)
{
    return hitachi424_protocol.Button == kHitachiAc424ButtonSwingV;
}

/// Convert a stdAc::opmode_t enum into its native mode.
/// @param[in] mode The enum to be converted.
/// @return The native equivalent of the enum.
uint8_t HitachiAC424_convertMode(uint8_t mode) {
    switch (mode) {
    case opmode_kCool: return kHitachiAc424Cool;
    case opmode_kHeat: return kHitachiAc424Heat;
    case opmode_kDry:  return kHitachiAc424Dry;
    case opmode_kFan:  return kHitachiAc424Fan;
    default:                     return kHitachiAc424Cool;
    }
}

/// Convert a stdAc::fanspeed_t enum into it's native speed.
/// @param[in] speed The enum to be converted.
/// @return The native equivalent of the enum.
uint8_t HitachiAC424_convertFan(uint8_t speed)
{
    switch (speed) {
    case fanspeed_kMin:    return kHitachiAc424FanMin;
    case fanspeed_kLow:    return kHitachiAc424FanLow;
    case fanspeed_kMedium: return kHitachiAc424FanMedium;
    case fanspeed_kHigh:   return kHitachiAc424FanHigh;
    case fanspeed_kMax:    return kHitachiAc424FanMax;
    default:                         return kHitachiAc424FanAuto;
    }
}

/// Convert a native mode into its stdAc equivalent.
/// @param[in] mode The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
uint8_t HitachiAC424_toCommonMode(const uint8_t mode) {
    switch (mode) {
    case kHitachiAc424Cool: return opmode_kCool;
    case kHitachiAc424Heat: return opmode_kHeat;
    case kHitachiAc424Dry:  return opmode_kDry;
    case kHitachiAc424Fan:  return opmode_kFan;
    default:                return opmode_kCool;
    }
}

/// Convert a native fan speed into its stdAc equivalent.
/// @param[in] speed The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
uint8_t HitachiAC424_toCommonFanSpeed(const uint8_t speed) {
    switch (speed) {
    case kHitachiAc424FanMax:    return fanspeed_kMax;
    case kHitachiAc424FanHigh:   return fanspeed_kHigh;
    case kHitachiAc424FanMedium: return fanspeed_kMedium;
    case kHitachiAc424FanLow:    return fanspeed_kLow;
    case kHitachiAc424FanMin:    return fanspeed_kMin;
    default:                     return fanspeed_kAuto;
    }
}

/// Convert the current internal state into its stdAc::state_t equivalent.
/// @return The stdAc equivalent of the native settings.
static state_ir_t HitachiAC424_toCommon(void)
{
    state_ir_t result;
    result.protocol = HITACHI_AC424;
    result.model = -1;  // No models used.
    result.power = HitachiAC424_getPower();
    result.mode = HitachiAC424_toCommonMode(hitachi424_protocol.Mode);
    result.celsius = true;
    result.degrees = hitachi424_protocol.Temp;
    result.fanspeed = HitachiAC424_toCommonFanSpeed(hitachi424_protocol.Fan);
    result.swingv = HitachiAC424_getSwingVToggle() ? swingv_kAuto
                                        : swingv_kOff;
    // Not supported.
    result.swingh = swingh_kOff;
    result.quiet = false;
    result.turbo = false;
    result.clean = false;
    result.econo = false;
    result.filter = false;
    result.light = false;
    result.beep = false;
    result.sleep = -1;
    result.clock = -1;
    return result;
}








/// Convert the internal state into a human readable string.
/// @return A string containing the settings in human-readable form.
// String HitachiAC424_toString(void)

/// Class constructor for handling detailed Hitachi_AC344 43 byte A/C messages.
/// @param[in] pin GPIO to be used when sending.
/// @param[in] inverted Is the output signal to be inverted?
/// @param[in] use_modulation Is frequency modulation to be used?
// HitachiAC344_IRHitachiAc344(const uint16_t pin, const bool inverted,
//                                const bool use_modulation)
//     : IRHitachiAc424(pin, inverted, use_modulation) { stateReset(); }


/// @param[in] pin GPIO to be used when sending.
/// @param[in] inverted Is the output signal to be inverted?
/// @param[in] use_modulation Is frequency modulation to be used?
// HitachiAC424_IRHitachiAc424(const uint16_t pin, const bool inverted,
//                          const bool use_modulation)
//     : _irsend(pin, inverted, use_modulation) { stateReset(); }

/// Reset the internal state to a fixed known good state.
/// @note Reset to auto fan, cooling, 23° Celsius
void HitachiAC344_stateReset(void) {
    for (uint8_t i = 0; i < kHitachiAc344StateLength; i++)
        hitachi344_protocol.raw[i] = 0x00;

    hitachi344_protocol.raw[0]  = 0x01;
    hitachi344_protocol.raw[1]  = 0x10;
    hitachi344_protocol.raw[3]  = 0x40;
    hitachi344_protocol.raw[5]  = 0xFF;
    hitachi344_protocol.raw[7]  = 0xCC;
    hitachi344_protocol.raw[33] = 0x80;
    hitachi344_protocol.raw[35] = 0x03;
    hitachi344_protocol.raw[37] = 0x00; /* diff with ac 424 */
    hitachi344_protocol.raw[39] = 0x00; /* diff with ac 424 */
    hitachi344_protocol.raw[45] = 0xFF;
    hitachi344_protocol.raw[47] = 0xFF;
    hitachi344_protocol.raw[49] = 0xFF;
    hitachi344_protocol.raw[51] = 0xFF;

    HitachiAC344_setTemp(23, true);
    HitachiAC344_setPower(true);
    HitachiAC344_setMode(kHitachiAc344Cool);
    HitachiAC344_setFan(kHitachiAc344FanAuto);
}

#if SEND_HITACHI_AC344
/// Create and send the IR message to the A/C.
/// @param[in] repeat Nr. of times to repeat the message.
void HitachiAC344_send(const uint16_t repeat)
{
    _irsend.sendHitachiAc344(getRaw(), kHitachiAc344StateLength, repeat);
}
#endif  // SEND_HITACHI_AC344

void HitachiAC344_setInvertedStates(void)
{
    /* use kHitachiAc424StateLength instead of kHitachiAc344StateLength*/
    invertBytePairs(hitachi344_protocol.raw + 3, kHitachiAc424StateLength- 3);
}

/// Get a PTR to the internal state/code for this protocol.
/// @return PTR to a code for this protocol based on the current internal state.
uint8_t *HitachiAC344_getRaw(void)
{
    HitachiAC344_setInvertedStates();
    return hitachi344_protocol.raw;
}

/// Set the internal state from a valid code for this protocol.
/// @param[in] new_code A valid code for this protocol.
/// @param[in] length Size (in bytes) of the code for this protocol.
void HitachiAC344_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
    HitachiAC344_stateReset();
    memcpy(hitachi344_protocol.raw, new_code, min(length, kHitachiAc344StateLength));

    /* check data */
    // os_trace("temp: %d \r\n", HitachiAC344_getTemp());
    // os_trace("mode: %d \r\n", HitachiAC344_getMode());
    // os_trace("fan: %d \r\n", HitachiAC344_getFan());
    // os_trace("power: %d \r\n", HitachiAC344_getPower());
    *ir_state = HitachiAC344_toCommon();
}

void HitachiAC264_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
    Hitachi424Protocol _;
    memset(&_, 0, sizeof(Hitachi424Protocol));
    memcpy(_.raw, new_code, min(length, kHitachiAc264StateLength));

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp;
    
}

void HitachiAC344_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
    HitachiAC344_stateReset();
    memcpy(hitachi344_protocol.raw, new_code, min(length, kHitachiAc344StateLength));

    ir_state->degrees = (float)HitachiAC344_getTemp();
    ir_state->mode = HitachiAC344_getMode();
    switch ((uint8_t)ir_state->mode)
    {
    case kHitachiAc344Fan:
        ir_state->mode = 4;
        break;
    
    case kHitachiAc344Cool:
        ir_state->mode = 1;
        break;
    
    case kHitachiAc344Dry:
        ir_state->mode = 2;
        break;
    
    case kHitachiAc344Heat:
        ir_state->mode = 3;
        break;
    
    default:
        break;
    }
    ir_state->fanspeed = HitachiAC344_getFan();
    switch ((uint8_t)ir_state->fanspeed)
    {
    case kHitachiAc344FanMin:
        ir_state->fanspeed = 1;
        break;
    
    case kHitachiAc344FanMedium:
        ir_state->fanspeed = 2;
        break;
    
    case kHitachiAc344FanMax:
        ir_state->fanspeed = 3;
        break;
    
    case kHitachiAc344FanAuto:
        ir_state->fanspeed = 0;
        break;
    
    default:
        break;
    }
    ir_state->power = HitachiAC344_getPower();

    /* check data */
    // os_trace("temp: %d \r\n", HitachiAC344_getTemp());
    // os_trace("mode: %d \r\n", HitachiAC344_getMode());
    // os_trace("fan: %d \r\n", HitachiAC344_getFan());
    // os_trace("power: %d \r\n", HitachiAC344_getPower());
    // *ir_state = HitachiAC344_toCommon();
}

static bool HitachiAC344_getPower(void) 
{
    return hitachi344_protocol.Power == kHitachiAc424PowerOn;
}

/* set power */
/// Change the power setting.
/// @param[in] on true, the setting is on. false, the setting is off.
static void HitachiAC344_setPower(const bool on)
{
    HitachiAC344_setButton(kHitachiAc344ButtonPowerMode);
    hitachi344_protocol.Power = (on ? kHitachiAc424PowerOn : kHitachiAc424PowerOff);
}

/// Get the current temperature setting.
/// @return The current setting for temp. in degrees celsius.
static uint8_t HitachiAC344_getTemp(void)
{
    return hitachi344_protocol.Temp;
}

/// Set the temperature.
/// @param[in] celsius The temperature in degrees celsius.
/// @param[in] setPrevious true, remember this if we change mode. false, don't.
static void HitachiAC344_setTemp(const uint8_t celsius, bool setPrevious)
{
    uint8_t temp;
    temp = min(celsius, kHitachiAc344MaxTemp);
    temp = max(temp, kHitachiAc344MinTemp);
    hitachi344_protocol.Temp = temp;
    if (_previoustemp > temp)
        HitachiAC344_setButton(kHitachiAc344ButtonTempDown);
    else if (_previoustemp < temp)
        HitachiAC344_setButton(kHitachiAc344ButtonTempUp);
    if (setPrevious) _previoustemp = temp;
}

/// Get the operating mode setting of the A/C.
/// @return The current operating mode setting.
static uint8_t HitachiAC344_getMode(void)
{
    return hitachi344_protocol.Mode;
}

/// Set the operating mode of the A/C.
/// @param[in] mode The desired operating mode.
static void HitachiAC344_setMode(const uint8_t mode)
{
    uint8_t newMode = mode;
    switch (mode) {
    // Fan mode sets a special temp.
    case kHitachiAc344Fan: {
        HitachiAC344_setTemp(kHitachiAc424FanTemp, false); 
    } break;
    case kHitachiAc344Heat:
    case kHitachiAc344Cool:
    case kHitachiAc344Dry: break;
    default: newMode = kHitachiAc344Cool;
    }
    hitachi344_protocol.Mode = newMode;
    if (newMode != kHitachiAc344Fan)
        HitachiAC344_setTemp(_previoustemp, true);
    HitachiAC344_setFan(hitachi344_protocol.Fan);  // Reset the fan speed after the mode change.
    HitachiAC344_setButton(kHitachiAc344ButtonPowerMode);
}

static uint8_t HitachiAC344_getFan(void)
{
    return hitachi344_protocol.Fan;
}

/// Set the speed of the fan.
/// @param[in] speed The desired setting.
static void HitachiAC344_setFan(const uint8_t speed)
{
    uint8_t newSpeed = max(speed, kHitachiAc344FanMin);
    uint8_t fanMax = kHitachiAc344FanMax;

    // Only 2 x low speeds in Dry mode or Auto
    if (hitachi344_protocol.Mode == kHitachiAc344Dry && speed == kHitachiAc344FanAuto) {
        fanMax = kHitachiAc424FanAuto;
    } else if (hitachi344_protocol.Mode == kHitachiAc344Dry) {
        fanMax = kHitachiAc424FanMaxDry;
    } else if (hitachi344_protocol.Mode == kHitachiAc344Fan && speed == kHitachiAc344FanAuto) {
        // Fan Mode does not have auto. Set to safe low
        newSpeed = kHitachiAc344FanMin;
    }

    newSpeed = min(newSpeed, fanMax);
    // Handle the setting the button value if we are going to change the value.
    if (newSpeed != hitachi344_protocol.Fan)
        HitachiAC424_setButton(kHitachiAc344ButtonFan);
    // Set the values
    hitachi344_protocol.Fan = newSpeed;
    hitachi344_protocol.raw[9] = 0x92;
    hitachi344_protocol.raw[29] = 0x00;

    // When fan is at min/max, additional bytes seem to be set
    if (newSpeed == kHitachiAc344FanMin)
        hitachi344_protocol.raw[9] = 0x98;
    if (newSpeed == kHitachiAc344FanMax) {
        hitachi344_protocol.raw[9] = 0xA9;
        hitachi344_protocol.raw[29] = 0x30;
    }
}

/// Set the Vertical Swing toggle setting of the A/C.
/// @param[in] on true, the setting is on. false, the setting is off.
/// @note The remote does not keep state of the vertical swing.
///   A byte is sent indicating the swing button is pressed on the remote
void HitachiAC344_setSwingVToggle(const bool on)
{
    uint8_t button = hitachi344_protocol.Button;  // Get the current button value.
    if (on)
        button = kHitachiAc344ButtonSwingV;  // Set the button to SwingV.
    else if (button == kHitachiAc344ButtonSwingV)  // Asked to unset it
    // It was set previous, so use Power as a default
        button = kHitachiAc344ButtonPowerMode;
    HitachiAC344_setButton(button);
}

/// Control the vertical swing setting.
/// @param[in] on True, turns on the feature. False, turns off the feature.
void HitachiAC344_setSwingV(const bool on)
{
    /* alike AC424 */
    HitachiAC344_setSwingVToggle(on);  // Set the button value.
    hitachi344_protocol.SwingV = on;
}

/// Get the current vertical swing setting.
/// @return True, if the setting is on. False, it is off.
bool HitachiAC344_getSwingV(void)
{
    return hitachi344_protocol.SwingV;
}

/// Control the horizontal swing setting.
/// @param[in] position The position to set the horizontal swing to.
void HitachiAC344_setSwingH(const uint8_t position)
{
    if (position > kHitachiAc344SwingHLeftMax)
    hitachi344_protocol.SwingH = kHitachiAc344SwingHMiddle;
    else
    hitachi344_protocol.SwingH = position;
    
    HitachiAC344_setButton(kHitachiAc344ButtonSwingH);
}

/// Get the current horizontal swing setting.
/// @return The current position horizontal swing is set to.
uint8_t HitachiAC344_getSwingH(void)
{
    return hitachi344_protocol.SwingH;
}

/// Convert a standard A/C horizontal swing into its native setting.
/// @param[in] position A stdAc::swingh_t position to convert.
/// @return The equivilent native horizontal swing position.
uint8_t HitachiAC344_convertSwingH(uint8_t position)
{
    switch (position) {
    case swingh_kAuto:     return kHitachiAc344SwingHAuto;
    case swingh_kLeftMax:  return kHitachiAc344SwingHLeftMax;
    case swingh_kLeft:     return kHitachiAc344SwingHLeft;
    case swingh_kRight:    return kHitachiAc344SwingHRight;
    case swingh_kRightMax: return kHitachiAc344SwingHRightMax;
    default:                         return kHitachiAc344SwingHMiddle;
    }
}

/// Convert a native horizontal swing postion to it's common equivalent.
/// @param[in] pos A native position to convert.
/// @return The common horizontal swing position.
uint8_t HitachiAC344_toCommonSwingH(const uint8_t pos)
{
    switch (pos) {
    case kHitachiAc344SwingHLeftMax:  return swingh_kLeftMax;
    case kHitachiAc344SwingHLeft:     return swingh_kLeft;
    case kHitachiAc344SwingHRight:    return swingh_kRight;
    case kHitachiAc344SwingHRightMax: return swingh_kRightMax;
    case kHitachiAc344SwingHAuto:     return swingh_kAuto;
    default:                          return swingh_kOff;
    }
}

/// Convert a native mode into its stdAc equivalent.
/// @param[in] mode The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
uint8_t HitachiAC344_toCommonMode(const uint8_t mode) {
    switch (mode) {
    case kHitachiAc344Cool: return opmode_kCool;
    case kHitachiAc344Heat: return opmode_kHeat;
    case kHitachiAc344Dry:  return opmode_kDry;
    case kHitachiAc344Fan:  return opmode_kFan;
    default:                return opmode_kCool;
    }
}

state_ir_t HitachiAC344_toCommon(void)
{
    state_ir_t result;
    result.protocol = HITACHI_AC344;
    result.model = -1;  // No models used.
    result.power = HitachiAC344_getPower();
    result.mode = HitachiAC344_toCommonMode(hitachi344_protocol.Mode);
    result.celsius = true;
    result.degrees = hitachi344_protocol.Temp;
    result.fanspeed = HitachiAC424_toCommonFanSpeed(hitachi344_protocol.Fan);
    result.swingv = hitachi344_protocol.SwingV ? swingv_kAuto : swingv_kOff;
    result.swingh = HitachiAC344_toCommonSwingH(hitachi344_protocol.SwingH);

    // Not supported.
    result.swingh = swingh_kOff;
    result.quiet = false;
    result.turbo = false;
    result.clean = false;
    result.econo = false;
    result.filter = false;
    result.light = false;
    result.beep = false;
    result.sleep = -1;
    result.clock = -1;
    return result;
}

/// Set the Button/Command pressed setting of the A/C.
/// @param[in] button The value of the button/command that was pressed.
static void HitachiAC344_setButton(const uint8_t button)
{
    hitachi344_protocol.Button = button;
}

/// Convert the internal state into a human readable string.
/// @return A string containing the settings in human-readable form.
// String HitachiAC344_toString(void)

/// Send a Hitachi 28-byte/224-bit A/C formatted message. (HITACHI_AC)
/// Status: STABLE / Working.
/// @param[in] data The message to be sent.
/// @param[in] nbytes The number of bytes of message to be sent.
/// @param[in] repeat The number of times the command is to be repeated.
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/417
void sendHitachiAC(const unsigned char data[], const uint16_t nbytes,
                           const uint16_t repeat) 
{
    if (nbytes < kHitachiAcStateLength)
        return;  // Not enough bytes to send a proper message.

    bool MSBfirst = true;
    switch (nbytes) {
        case kHitachiAc264StateLength:
        case kHitachiAc296StateLength:
        case kHitachiAc344StateLength:
        MSBfirst = false;
    }

    vTaskEnterCritical();
    sendGenericBytes(kHitachiAcHdrMark, kHitachiAcHdrSpace, kHitachiAcBitMark,
                kHitachiAcOneSpace, kHitachiAcBitMark, kHitachiAcZeroSpace,
                kHitachiAcBitMark, kHitachiAcMinGap, data, nbytes, 38, MSBfirst,
                repeat, 50);
    vTaskExitCritical();
}

/// Send a Hitachi 13 byte/224-bit A/C formatted message. (HITACHI_AC1)
/// Status: STABLE / Confirmed Working.
/// @param[in] data The message to be sent.
/// @param[in] nbytes The number of bytes of message to be sent.
/// @param[in] repeat The number of times the command is to be repeated.
/// @note Basically the same as sendHitatchiAC() except different size & header.
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/453
void sendHitachiAC1(const unsigned char data[], const uint16_t nbytes,
                            const uint16_t repeat)
{
    if (nbytes < kHitachiAc1StateLength)
        return;  // Not enough bytes to send a proper message.

    vTaskEnterCritical();
    sendGenericBytes(kHitachiAc1HdrMark, kHitachiAc1HdrSpace, kHitachiAcBitMark,
                kHitachiAcOneSpace, kHitachiAcBitMark, kHitachiAcZeroSpace,
                kHitachiAcBitMark, kHitachiAcMinGap, data, nbytes, kHitachiAcFreq,
                true, repeat, kDutyDefault);
    vTaskExitCritical();
}

/// Send a Hitachi A/C 43-byte/344-bit message. (HITACHI_AC344)
///  Basically the same as sendHitatchiAC() except different size.
/// Status: Beta / Probably works.
/// @param[in] data An array of bytes containing the IR command.
/// @param[in] nbytes Nr. of bytes of data in the array.
/// @param[in] repeat Nr. of times the message is to be repeated. (Default = 0).
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1134
void sendHitachiAc344(const unsigned char data[], const uint16_t nbytes,
                              const uint16_t repeat)
{
    if (nbytes < kHitachiAc344StateLength)
        return;  // Not enough bytes to send a proper message.
    sendHitachiAC(data, nbytes, repeat);
}

/// Send a Hitachi 53-byte/424-bit A/C formatted message. (HITACHI_AC424)
/// Status: STABLE / Reported as working.
/// @param[in] data The message to be sent.
/// @param[in] nbytes The number of bytes of message to be sent.
/// @param[in] repeat The number of times the command is to be repeated.
/// @note This protocol is almost exactly the same as HitachiAC2 except this
///   variant has a leader section as well, and subtle timing differences.
///   It is also in LSBF order (per byte), rather than MSBF order.
void sendHitachiAc424(const uint8_t data[], const uint16_t nbytes,
                              const uint16_t repeat) {
    // enableIROut(kHitachiAcFreq);
    
    vTaskEnterCritical();
    for (uint16_t r = 0; r <= repeat; r++)
    {
        // Leader
        mark(kHitachiAc424LdrMark);
        space(kHitachiAc424LdrSpace);
        // Header + Data + Footer
        sendGeneric(kHitachiAc424HdrMark, kHitachiAc424HdrSpace,
                    kHitachiAc424BitMark, kHitachiAc424OneSpace,
                    kHitachiAc424BitMark, kHitachiAc424ZeroSpace,
                    kHitachiAc424BitMark, kHitachiAcMinGap,
                    data, nbytes,  // Bytes
                    kHitachiAcFreq, false, kNoRepeat, kDutyDefault);
    }
    vTaskExitCritical();
}

/* send AC */
void Hitachi_AC_send(state_ir_t state_ir)
{
    /* set ir state */
    HitachiAC_setPower(state_ir.power);
    HitachiAC_setTemp(state_ir.degrees);
    HitachiAC_setMode(state_ir.mode);
    HitachiAC_setFan(state_ir.fanspeed);
    HitachiAC_calcChecksum(HitachiAC_getRaw(), kHitachiAcStateLength);
    /* send */
    sendHitachiAC(HitachiAC_getRaw(), kHitachiAcStateLength, kHitachiAcDefaultRepeat);
}

/* send AC1 */
void Hitachi_AC1_send(state_ir_t state_ir)
{
    switch ((uint8_t)state_ir.mode)
    {
    case 4:
        state_ir.mode = kHitachiAc1Fan;
        break;
    
    case 1:
        state_ir.mode = kHitachiAc1Cool;
        break;
    
    case 2:
        state_ir.mode = kHitachiAc1Dry;
        break;
    
    case 3:
        state_ir.mode = kHitachiAc1Heat;
        break;
    
    default:
        break;
    }
    switch ((uint8_t)state_ir.fanspeed)
    {
    case 1:
        state_ir.fanspeed = kHitachiAc1FanLow;
        break;
    
    case 2:
        state_ir.fanspeed = kHitachiAc1FanMed;
        break;
    
    case 3:
        state_ir.fanspeed = kHitachiAc1FanHigh;
        break;
    
    case 0:
        state_ir.fanspeed = kHitachiAc1FanAuto;
        break;
    
    default:
        break;
    }
    /* set ir_state */
    HitachiAC1_setPower(state_ir.power);
    HitachiAC1_setTemp(state_ir.degrees);
    HitachiAC1_setMode(state_ir.mode);
    HitachiAC1_setFan(state_ir.fanspeed);

    HitachiAC1_calcChecksum(HitachiAC1_getRaw(), kHitachiAc1StateLength);

    /* send */
    sendHitachiAC1(HitachiAC1_getRaw(), kHitachiAc1StateLength, kHitachiAcDefaultRepeat);
}

/* send AC424 */
void Hitachi_AC424_send(state_ir_t state_ir)
{
    switch ((uint8_t)state_ir.mode)
    {
    case 4:
        state_ir.mode = kHitachiAc424Fan;
        break;
    
    case 1:
        state_ir.mode = kHitachiAc424Cool;
        break;
    
    case 2:
        state_ir.mode = kHitachiAc424Dry;
        break;
    
    case 3:
        state_ir.mode = kHitachiAc424Heat;
        break;
    
    default:
        break;
    }
    switch ((uint8_t)state_ir.fanspeed)
    {
    case 1:
        state_ir.fanspeed = kHitachiAc424FanMin;
        break;
    
    case 2:
        state_ir.fanspeed = kHitachiAc424FanMedium;
        break;
    
    case 3:
        state_ir.fanspeed = kHitachiAc424FanMax;
        break;
    
    case 0:
        state_ir.fanspeed = kHitachiAc424FanAuto;
        break;
    
    default:
        break;
    }

    /* set ir_state */
    HitachiAC424_setPower(state_ir.power);
    HitachiAC424_setTemp(state_ir.degrees, true);
    HitachiAC424_setMode(state_ir.mode);
    HitachiAC424_setFan(state_ir.fanspeed);

    /* send */
    sendHitachiAc424(HitachiAC424_getRaw(), kHitachiAc424StateLength, kHitachiAcDefaultRepeat);
}

/* send AC344 */
void Hitachi_AC344_send(state_ir_t state_ir)
{
    switch ((uint8_t)state_ir.mode)
    {
    case 4:
        state_ir.mode = kHitachiAc344Fan;
        break;
    
    case 1:
        state_ir.mode = kHitachiAc344Cool;
        break;
    
    case 2:
        state_ir.mode = kHitachiAc344Dry;
        break;
    
    case 3:
        state_ir.mode = kHitachiAc344Heat;
        break;
    
    default:
        break;
    }
    switch ((uint8_t)state_ir.fanspeed)
    {
    case 1:
        state_ir.fanspeed = kHitachiAc344FanMin;
        break;
    
    case 2:
        state_ir.fanspeed = kHitachiAc344FanMedium;
        break;
    
    case 3:
        state_ir.fanspeed = kHitachiAc344FanMax;
        break;
    
    case 0:
        state_ir.fanspeed = kHitachiAc344FanAuto;
        break;
    
    default:
        break;
    }
    /* set ir_state */
    HitachiAC344_setPower(state_ir.power);
    HitachiAC344_setTemp(state_ir.degrees, true);
    HitachiAC344_setMode(state_ir.mode);
    HitachiAC344_setFan(state_ir.fanspeed);

    /* send */
    sendHitachiAc344(HitachiAC344_getRaw(), kHitachiAc344StateLength, kHitachiAcDefaultRepeat);
}


void HitachiAC264_send(state_ir_t state_ir)
{
    Hitachi424Protocol _;
    memset(&_, 0, sizeof(Hitachi424Protocol)); 
    for (uint8_t i = 0; i < kHitachiAc424StateLength; i++)
        _.raw[i] = 0x00;

    _.raw[0]  = 0x01;
    _.raw[1]  = 0x10;
    _.raw[3]  = 0x40;
    _.raw[5]  = 0xFF;
    _.raw[7]  = 0xCC;
    _.raw[27] = 0xE1;
    _.raw[33] = 0x80;
    _.raw[35] = 0x03;
    _.raw[37] = 0x01;
    _.raw[39] = 0x88;
    _.raw[45] = 0xFF;
    _.raw[47] = 0xFF;
    _.raw[49] = 0xFF;
    _.raw[51] = 0xFF;

    _.raw[9] = 0x92;
    _.raw[27] = 0xC1;

    // set power
    _.Power = state_ir.power;
    _.Button = kHitachiAc424ButtonPowerMode;

    // set temp
    uint8_t temp;
    temp = min((uint8_t)state_ir.degrees, kHitachiAc424MaxTemp);
    temp = max(temp, kHitachiAc424MinTemp);
    _.Temp = temp;

    // set mode
    uint8_t newMode = state_ir.mode;
    switch ((uint8_t)state_ir.mode) {
        // Fan mode sets a special temp.
        case kHitachiAc424Fan: {
            state_ir.degrees = kHitachiAc424FanTemp;
            _.Temp = kHitachiAc424FanTemp; 
        } break;
        case kHitachiAc424Heat:
        case kHitachiAc424Cool:
        case kHitachiAc424Dry: break;
        default: newMode = kHitachiAc424Cool;
    }
    _.Mode = newMode;

    // set fan
    uint8_t speed = (uint8_t)state_ir.fanspeed;
    uint8_t newSpeed = max(speed, kHitachiAc424FanMin);
    uint8_t fanMax = kHitachiAc424FanMax;

    // Only 2 x low speeds in Dry mode or Auto
    if (_.Mode == kHitachiAc424Dry && speed == kHitachiAc424FanAuto) {
        fanMax = kHitachiAc424FanAuto;
    } else if (_.Mode == kHitachiAc424Dry) {
        fanMax = kHitachiAc424FanMaxDry;
    } else if (_.Mode == kHitachiAc424Fan && speed == kHitachiAc424FanAuto) {
        // Fan Mode does not have auto. Set to safe low
        newSpeed = kHitachiAc424FanMin;
    }

    newSpeed = min(newSpeed, fanMax);
    // Handle the setting the button value if we are going to change the value.
    //   if (newSpeed != _.Fan) setButton(kHitachiAc424ButtonFan);
    // Set the values
    _.Fan = newSpeed;
    _.raw[9] = 0x92;
    _.raw[29] = 0x00;

    // When fan is at min/max, additional bytes seem to be set
    if (newSpeed == kHitachiAc424FanMin) _.raw[9] = 0x98;
    if (newSpeed == kHitachiAc424FanMax) {
        _.raw[9] = 0xA9;
        _.raw[29] = 0x30;
    }

    // set temp
    invertBytePairs(_.raw + 3, kHitachiAc424StateLength - 3);

    sendHitachiAC(_.raw, kHitachiAc264StateLength, kHitachiAcDefaultRepeat);
}

static bool IRHitachiAc296_hasInvertedStates(const uint8_t state[],
                                       const uint16_t length) {
  return HitachiAC3_hasInvertedStates(state, length);
}

bool decodeHitachiAc296(decode_results *results, uint16_t offset,
                                const uint16_t nbits,
                                const bool strict) {

  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    kHitachiAcHdrMark-700, kHitachiAcHdrMark+700, kHitachiAcHdrSpace-400, kHitachiAcHdrSpace+400,
                    kHitachiAcBitMark-350, kHitachiAcBitMark+350, kHitachiAcOneSpace-450, kHitachiAcOneSpace+450,
                    kHitachiAcBitMark-350, kHitachiAcBitMark+350, kHitachiAcZeroSpace-350, kHitachiAcZeroSpace+350,
                    kHitachiAcBitMark-350, kHitachiAcBitMark+350, kHitachiAcMinGap-25000, kHitachiAcMinGap+25000, true,
                    kUseDefTol, 0, false)) return false;

  // Compliance
  if (strict && !IRHitachiAc296_hasInvertedStates(results->state, nbits / 8))
  {
    os_trace("%s invalid checksum\r\n", __func__);
    return false;
  }

  // Success
  os_trace("%s success\r\n", __func__);
  results->decode_type = HITACHI_AC296;
  results->bits = nbits;
  return true;
}

void HitachiAc296_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state) {
    HitachiAC296Protocol _;
    memset(&_, 0, sizeof(HitachiAC296Protocol));
    memcpy(_.raw, new_code, min(length, kHitachiAc296StateLength));

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp;
}

void HitachiAc296_send(state_ir_t state_ir)
{
    HitachiAC296Protocol _;
    memset(&_, 0, sizeof(HitachiAC296Protocol));
    // Header
    _.raw[0] = 0x01;
    _.raw[1] = 0x10;
    _.raw[2] = 0x00;

    // Every next byte is a parity byte
    _.raw[3] = 0x40;
    _.raw[5] = 0xFF;
    _.raw[7] = 0xCC;
    _.raw[9] = 0x92;
    _.raw[11] = 0x43;
    // 13-14 is Temperature and parity
    _.raw[15] = 0x00;
    _.raw[17] = 0x00;  // Off timer LSB
    _.raw[19] = 0x00;  // Off timer cont
    _.raw[21] = 0x00;  // On timer LSB
    _.raw[23] = 0x00;  // On timer cont
    // 25-26 is Mode and fan
    _.raw[27] = 0xF1;  // Power on
    _.raw[29] = 0x00;
    _.raw[31] = 0x00;
    _.raw[33] = 0x00;
    _.raw[35] = 0x03;  // Humidity

    // power
    _.Power = state_ir.power;
    // mode
    switch ((uint8_t)state_ir.mode) {
        case kHitachiAc296Heat:
        case kHitachiAc296Cool:
        case kHitachiAc296Dehumidify:
        case kHitachiAc296AutoDehumidifying:
        case kHitachiAc296Auto:
        _.Mode = state_ir.mode;
        break;
        default:
            _.Mode = kHitachiAc296Auto;
            break;
    }
    // fan
    uint8_t newSpeed = max((uint8_t)state_ir.fanspeed, kHitachiAc296FanSilent);
    _.Fan = min(newSpeed, kHitachiAc296FanAuto);
    uint8_t temp = (uint8_t)state_ir.degrees;
    if (_.Mode == kHitachiAc296Auto) {  // Special temp for auto mode
        temp = kHitachiAc296TempAuto;
    } else {  // Normal temp setting.
        temp = min(temp, kHitachiAc296MaxTemp);
        temp = max(temp, kHitachiAc296MinTemp);
    }
    _.Temp = temp;
    state_ir.degrees = temp;
    // checksum
    invertBytePairs(_.raw + 3, kHitachiAc296StateLength - 3);
    sendHitachiAC(_.raw, kHitachiAc296StateLength, kHitachiAcDefaultRepeat);
}