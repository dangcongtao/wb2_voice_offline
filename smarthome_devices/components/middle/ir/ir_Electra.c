#include "ir_Electra.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

// Constants
const uint16_t kElectraAcHdrMark = 9166;
const uint16_t kElectraAcBitMark = 646;
const uint16_t kElectraAcHdrSpace = 4470;
const uint16_t kElectraAcOneSpace = 1647;
const uint16_t kElectraAcZeroSpace = 547;
const uint32_t kElectraAcMessageGap = kDefaultMessageGap;  // Just a guess.

static uint8_t IRElectraAc_calcChecksum(const uint8_t state[],
                                  const uint16_t length) {
  if (length == 0) return state[0];
  return sumBytesState(state, length - 1, 0);
}

static bool IRElectraAc_validChecksum(const uint8_t state[], const uint16_t length) {
  if (length < 2)
    return true;  // No checksum to compare with. Assume okay.
  return (state[length - 1] == IRElectraAc_calcChecksum(state, length));
}

bool decodeElectraAC(decode_results *results, uint16_t offset,
                             const uint16_t nbits,
                             const bool strict) {
  if (strict) {
    if (nbits != kElectraAcBits)
      return false;  // Not strictly a ELECTRA_AC message.
  }

  // Match Header + Data + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    kElectraAcHdrMark-2000, kElectraAcHdrMark+2000, kElectraAcHdrSpace-1000, kElectraAcHdrSpace+1000,
                    kElectraAcBitMark-500,kElectraAcBitMark+400, kElectraAcOneSpace-400,kElectraAcOneSpace+400,
                    kElectraAcBitMark-500,kElectraAcBitMark+400, kElectraAcZeroSpace-400,kElectraAcZeroSpace+400,
                    kElectraAcBitMark-500,kElectraAcBitMark+400, kElectraAcMessageGap-2000,kElectraAcMessageGap+2000, true,
                    _tolerance, 0, false)) return false;

  // Compliance
  if (strict) {
    // Verify the checksum.
    if (!IRElectraAc_validChecksum(results->state, kElectraAcStateLength)) return false;
  }

    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = ELECTRA_AC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void ElectraAc_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state) 
{
    ElectraProtocol _;
    memset(&_, 0, sizeof(ElectraProtocol));
    memcpy(_.raw, new_code, min(length, kElectraAcStateLength));

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kElectraAcTempDelta;
    ir_state->turbo = _.Turbo;
    ir_state->clean = _.Clean;
    ir_state->quiet = _.Quiet;
}

static void sendElectraAC(const uint8_t data[], const uint16_t nbytes,
                           const uint16_t repeat) {
    vTaskEnterCritical();                          
    for (uint16_t r = 0; r <= repeat; r++)
        sendGenericBytes(kElectraAcHdrMark, kElectraAcHdrSpace, kElectraAcBitMark,
                    kElectraAcOneSpace, kElectraAcBitMark, kElectraAcZeroSpace,
                    kElectraAcBitMark, kElectraAcMessageGap, data, nbytes,
                    38000,  // Complete guess of the modulation frequency.
                    false,  // Send data in LSB order per byte
                    0, 50);
    vTaskExitCritical();
}

void ElectraAc_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);
    ElectraProtocol _;
    for (uint8_t i = 1; i < kElectraAcStateLength - 2; i++) _.raw[i] = 0;
    _.raw[0] = 0xC3;
    _.LightToggle = kElectraAcLightToggleOff;

    // Power
    _.Power = state_ir.power;
    // Mode
    switch ((uint8_t)state_ir.mode) {
        case kElectraAcAuto:
        case kElectraAcDry:
        case kElectraAcCool:
        case kElectraAcHeat:
        case kElectraAcFan:
        _.Mode = state_ir.mode;
        break;
        default:
        // If we get an unexpected mode, default to AUTO.
        _.Mode = kElectraAcAuto;
    }
  // fanspeed
  switch ((uint8_t)state_ir.fanspeed) {
    case kElectraAcFanAuto:
    case kElectraAcFanHigh:
    case kElectraAcFanMed:
    case kElectraAcFanLow:
      _.Fan = state_ir.fanspeed;
      break;
    default:
      // If we get an unexpected speed, default to Auto.
      _.Fan = kElectraAcFanAuto;
  }
  // set Temp
  uint8_t newtemp = max(kElectraAcMinTemp, (uint8_t)state_ir.degrees);
  newtemp = min(kElectraAcMaxTemp, newtemp) - kElectraAcTempDelta;
  _.Temp = newtemp;
    // calculate checksum
    _.Sum = IRElectraAc_calcChecksum(_.raw, kElectraAcStateLength);

    sendElectraAC(_.raw, kElectraAcStateLength, kElectraAcMinRepeat);
}