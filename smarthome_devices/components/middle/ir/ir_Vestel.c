#include "ir_Vestel.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

static uint8_t IRVestelAc_calcChecksum(const uint64_t state) {
  // Just counts the set bits +1 on stream and take inverse after mask
  return 0xFF - countBits64(GETBITS64(state, 20, 44), 44, true, 2);
}

static bool IRVestelAc_validChecksum(const uint64_t state) {
  VestelProtocol vp;
  vp.cmdState = state;
  return vp.CmdSum == IRVestelAc_calcChecksum(state);
}

bool decodeVestelAc(decode_results* results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  if (nbits % 8 != 0)  // nbits has to be a multiple of nr. of bits in a byte.
    return false;

  if (strict)
    if (nbits != kVestelAcBits)
      return false;  // Not strictly a Vestel AC message.

  uint64_t data = 0;

  if (nbits > sizeof(data) * 8)
    return false;  // We can't possibly capture a Vestel packet that big.

  // Match Header + Data + Footer
  if (!matchGenericDataLimit(results->rawbuf + offset, &data,
                    results->rawlen - offset, nbits,
                    kVestelAcHdrMark-410,kVestelAcHdrMark+390, kVestelAcHdrSpace-500, kVestelAcHdrSpace+500,
                    kVestelAcBitMark-370, kVestelAcBitMark+380, kVestelAcOneSpace-535, kVestelAcOneSpace+535,
                    kVestelAcBitMark-370, kVestelAcBitMark+380, kVestelAcZeroSpace - 380, kVestelAcZeroSpace + 420,
                    kVestelAcBitMark-370, kVestelAcBitMark+380, 0, 0, false,
                    kVestelAcTolerance, kMarkExcess, false)) return false;
  // Compliance
  if (strict)
    if (!IRVestelAc_validChecksum(data)) return false;

    os_trace("%s success\r\n", __func__);

  // Success
  results->decode_type = VESTEL_AC;
  results->bits = nbits;
  results->value = data;
  results->address = 0;
  results->command = 0;

  return true;
}

void VestelAc_setInfo(const uint64_t newState, state_ir_t *ir_state)
{
    VestelProtocol _;
    _.cmdState = newState;
    _.timeState = newState;
    if (!_.UseCmd) {
        _.cmdState = kVestelAcStateDefault;
        _.UseCmd = false;
    } else {
        _.timeState = kVestelAcTimeStateDefault;
    }

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kVestelAcMinTempH;
}

static void sendVestelAc(const uint64_t data, const uint16_t nbits,
                          const uint16_t repeat) {
  if (nbits % 8 != 0) return;  // nbits is required to be a multiple of 8.
vTaskEnterCritical();
  sendGeneric(kVestelAcHdrMark, kVestelAcHdrSpace,   // Header
              kVestelAcBitMark, kVestelAcOneSpace,   // Data
              kVestelAcBitMark, kVestelAcZeroSpace,  // Data
              kVestelAcBitMark, 100000,              // Footer + repeat gap
              data, nbits, 38, false, repeat, 50);
vTaskExitCritical();
}

void VestelAc_send(state_ir_t state_ir)
{
    VestelProtocol _;
    _.cmdState = kVestelAcStateDefault;
    _.timeState = kVestelAcTimeStateDefault;
    // set power
    _.Power = (state_ir.power ? 0b11 : 0b00);
    // set mode
    switch ((uint8_t)state_ir.mode) {
    case kVestelAcAuto:
    case kVestelAcCool:
    case kVestelAcHeat:
    case kVestelAcDry:
    case kVestelAcFan:
      _.Mode = state_ir.mode;
      break;
    default:
      _.Mode = kVestelAcAuto;
  }

    switch ((uint8_t)state_ir.fanspeed) {
    case kVestelAcFanLow:
    case kVestelAcFanMed:
    case kVestelAcFanHigh:
    case kVestelAcFanAutoCool:
    case kVestelAcFanAutoHot:
    case kVestelAcFanAuto:
      _.Fan = state_ir.fanspeed;
      break;
    default:
      _.Fan = kVestelAcFanAuto;
  }

  uint8_t new_temp = max(kVestelAcMinTempC, (uint8_t)state_ir.degrees);
  new_temp = min(kVestelAcMaxTemp, new_temp);
  _.Temp = new_temp - kVestelAcMinTempH;
  _.UseCmd = true;

    // checksum
    // Stored the checksum value in the last byte.
  _.CmdSum = IRVestelAc_calcChecksum(_.cmdState);
  _.TimeSum = IRVestelAc_calcChecksum(_.timeState);

  sendVestelAc(((!_.UseCmd) ? _.timeState : _.cmdState), kVestelAcBits, kNoRepeat);

}

