#include "ir_Sanyo.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

// Constants
// Sanyo SA 8650B
const uint16_t kSanyoSa8650bHdrMark = 3500;  // seen range 3500
const uint16_t kSanyoSa8650bHdrSpace = 950;  // seen 950
const uint16_t kSanyoSa8650bOneMark = 2400;  // seen 2400
const uint16_t kSanyoSa8650bZeroMark = 700;  // seen 700
// usually see 713 - not using ticks as get number wrapround
const uint16_t kSanyoSa8650bDoubleSpaceUsecs = 800;
const uint16_t kSanyoSa8650bRptLength = 45000;

// Sanyo LC7461
const uint16_t kSanyoLc7461AddressMask = (1 << kSanyoLC7461AddressBits) - 1;
const uint16_t kSanyoLc7461CommandMask = (1 << kSanyoLC7461CommandBits) - 1;
const uint16_t kSanyoLc7461HdrMark = 9000;
const uint16_t kSanyoLc7461HdrSpace = 4500;
const uint16_t kSanyoLc7461BitMark = 560;    // 1T
const uint16_t kSanyoLc7461OneSpace = 1690;  // 3T
const uint16_t kSanyoLc7461ZeroSpace = 560;  // 1T
const uint32_t kSanyoLc7461MinCommandLength = 108000;

const uint16_t kSanyoLc7461MinGap =
    kSanyoLc7461MinCommandLength -
    (kSanyoLc7461HdrMark + kSanyoLc7461HdrSpace +
     kSanyoLC7461Bits * (kSanyoLc7461BitMark +
                         (kSanyoLc7461OneSpace + kSanyoLc7461ZeroSpace) / 2) +
     kSanyoLc7461BitMark);

const uint16_t kSanyoAcHdrMark = 8500;   ///< uSeconds
const uint16_t kSanyoAcHdrSpace = 4200;  ///< uSeconds
const uint16_t kSanyoAcBitMark = 500;    ///< uSeconds
const uint16_t kSanyoAcOneSpace = 1600;  ///< uSeconds
const uint16_t kSanyoAcZeroSpace = 550;  ///< uSeconds
const uint32_t kSanyoAcGap = kDefaultMessageGap;  ///< uSeconds (Guess only)
const uint16_t kSanyoAcFreq = 38000;  ///< Hz. (Guess only)

const uint16_t kSanyoAc88HdrMark = 5400;   ///< uSeconds
const uint16_t kSanyoAc88HdrSpace = 2000;  ///< uSeconds
const uint16_t kSanyoAc88BitMark = 500;    ///< uSeconds
const uint16_t kSanyoAc88OneSpace = 1500;  ///< uSeconds
const uint16_t kSanyoAc88ZeroSpace = 750;  ///< uSeconds
const uint32_t kSanyoAc88Gap = 3675;       ///< uSeconds
const uint16_t kSanyoAc88Freq = 38000;     ///< Hz. (Guess only)
const uint8_t  kSanyoAc88ExtraTolerance = 5;  /// (%) Extra tolerance to use.

const uint16_t kSanyoAc152HdrMark = 3300;   ///< uSeconds
const uint16_t kSanyoAc152BitMark = 440;    ///< uSeconds
const uint16_t kSanyoAc152HdrSpace = 1725;  ///< uSeconds
const uint16_t kSanyoAc152OneSpace = 1290;  ///< uSeconds
const uint16_t kSanyoAc152ZeroSpace = 405;  ///< uSeconds
const uint16_t kSanyoAc152Freq = 38000;     ///< Hz. (Guess only)
const uint8_t  kSanyoAc152ExtraTolerance = 13;  /// (%) Extra tolerance to use.

static uint8_t IRSanyoAc_calcChecksum(const uint8_t state[],
                                const uint16_t length) {
  return length ? sumNibblesByte(state, length - 1, 0) : 0;
}

static bool IRSanyoAc_validChecksum(const uint8_t state[], const uint16_t length) {
  return length && state[length - 1] == IRSanyoAc_calcChecksum(state, length);
}

bool decodeSanyoAc(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict) {
  if (strict && nbits != kSanyoAcBits)
    return false;

  // Header + Data + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    kSanyoAcHdrMark-500, kSanyoAcHdrMark+500, kSanyoAcHdrSpace-400,kSanyoAcHdrSpace+400,
                    kSanyoAcBitMark-400,kSanyoAcBitMark+350, kSanyoAcOneSpace-500,kSanyoAcOneSpace+500,
                    kSanyoAcBitMark-400,kSanyoAcBitMark+350, kSanyoAcZeroSpace-400, kSanyoAcZeroSpace+400,
                    kSanyoAcBitMark-400,kSanyoAcBitMark+350, kSanyoAcGap-1000, kSanyoAcGap+1000,
                    true, kUseDefTol, kMarkExcess, false)) return false;
  // Compliance
  if (strict)
    if (!IRSanyoAc_validChecksum(results->state, nbits / 8)) return false;

    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = SANYO_AC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRSanyoAc_setInfo(const uint8_t newState[], state_ir_t *ir_state) {
    SanyoProtocol _;
    memset(&_, 0, sizeof(SanyoProtocol));
    memcpy(_.raw, newState, kSanyoAcStateLength);

    ir_state->power = (_.Power == kSanyoAcPowerOn);
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kSanyoAcTempDelta;
}

static void sendSanyoAc(const uint8_t data[], const uint16_t nbytes,
                         const uint16_t repeat) {
    // Header + Data + Footer
    vTaskEnterCritical();
    sendGenericBytes(kSanyoAcHdrMark, kSanyoAcHdrSpace,
              kSanyoAcBitMark, kSanyoAcOneSpace,
              kSanyoAcBitMark, kSanyoAcZeroSpace,
              kSanyoAcBitMark, kSanyoAcGap,
              data, nbytes, kSanyoAcFreq, false, repeat, kDutyDefault);
    vTaskExitCritical();
}

void SanyoAc_send(state_ir_t state_ir)
{
    SanyoProtocol _;
    memset(&_, 0, sizeof(SanyoProtocol));
    uint8_t kReset[kSanyoAcStateLength] = {
    0x6A, 0x6D, 0x51, 0x00, 0x10, 0x45, 0x00, 0x00, 0x33};
    memcpy(_.raw, kReset, kSanyoAcStateLength);

    _.Power = (state_ir.power ? kSanyoAcPowerOn : kSanyoAcPowerOff);
    switch ((uint8_t)state_ir.mode) {
        case kSanyoAcAuto:
        case kSanyoAcCool:
        case kSanyoAcDry:
        case kSanyoAcHeat:
        _.Mode = state_ir.mode;
        break;
        default: _.Mode = kSanyoAcAuto;
    }

    _.Fan = state_ir.fanspeed;
    uint8_t temp = max((uint8_t)kSanyoAcTempMin, (uint8_t)state_ir.degrees);
    temp = min((uint8_t)kSanyoAcTempMax, temp);
    _.Temp = temp - kSanyoAcTempDelta;
    _.SwingV = kSanyoAcSwingVAuto;

    _.Sum = IRSanyoAc_calcChecksum(_.raw, kSanyoAcStateLength);
    sendSanyoAc(_.raw, kSanyoAcStateLength, kNoRepeat);

}


bool decodeSanyoAc88(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict) {
  if (strict && nbits != kSanyoAc88Bits)
    return false;

  uint16_t used = 0;
  // Compliance
  const uint16_t expected_repeats = strict ? kSanyoAc88MinRepeat : 0;

  // Handle the expected nr of repeats.
  for (uint16_t r = 0; r <= expected_repeats; r++) {
    // Header + Data + Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                        results->rawlen - offset, nbits,
                        kSanyoAc88HdrMark-400, kSanyoAc88HdrMark+400, kSanyoAc88HdrSpace-400, kSanyoAc88HdrSpace+400,
                        kSanyoAc88BitMark-350,kSanyoAc88BitMark+350, kSanyoAc88OneSpace-400,kSanyoAc88OneSpace+400,
                        kSanyoAc88BitMark-350,kSanyoAc88BitMark+350, kSanyoAc88ZeroSpace-350, kSanyoAc88ZeroSpace+350,
                        kSanyoAc88BitMark-350,kSanyoAc88BitMark+350,
                        // Expect an inter-message gap, or just the end of msg?
                        (r < expected_repeats) ? kSanyoAc88Gap
                                               : (kDefaultMessageGap-1000), (r < expected_repeats) ? kSanyoAc88Gap
                                               : (kDefaultMessageGap+1000),
                        r == expected_repeats,
                        _tolerance + kSanyoAc88ExtraTolerance,
                        kMarkExcess, false);
    if (!used) return false;  // No match!
    offset += used;
  }

    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = SANYO_AC88;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}



bool decodeSanyoAc152(decode_results *results, uint16_t offset,
                              const uint16_t nbits, const bool strict) {
  if (strict && nbits != kSanyoAc152Bits)
    return false;

  // Header + Data + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    kSanyoAc152HdrMark-400, kSanyoAc152HdrMark+400, kSanyoAc152HdrSpace-400, kSanyoAc152HdrSpace+400,
                    kSanyoAc152BitMark-340, kSanyoAc152BitMark+360, kSanyoAc152OneSpace-390, kSanyoAc152OneSpace+410,
                    kSanyoAc152BitMark-340, kSanyoAc152BitMark+360, kSanyoAc152ZeroSpace-305, kSanyoAc152ZeroSpace+305,
                    kSanyoAc152BitMark-340, kSanyoAc152BitMark+360,
                    kDefaultMessageGap-1000, kDefaultMessageGap+1000,  // Just a guess.
                    false, _tolerance + kSanyoAc152ExtraTolerance,
                    kMarkExcess, false))
    return false;  // No match!
    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = SANYO_AC152;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}
