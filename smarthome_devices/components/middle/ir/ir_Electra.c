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
const uint32_t kElectraAcMessageGap = kDefaultMessageGap; // Just a guess.

static uint8_t IRElectraAc_calcChecksum(const uint8_t state[],
                                        const uint16_t length)
{
  if (length == 0)
    return state[0];
  return sumBytesState(state, length - 1, 0);
}

static bool IRElectraAc_validChecksum(const uint8_t state[], const uint16_t length)
{
  if (length < 2)
    return true; // No checksum to compare with. Assume okay.
  return (state[length - 1] == IRElectraAc_calcChecksum(state, length));
}

bool decodeElectraAC(decode_results *results, uint16_t offset,
                     const uint16_t nbits,
                     const bool strict)
{
  if (strict)
  {
    if (nbits != kElectraAcBits)
      return false; // Not strictly a ELECTRA_AC message.
  }

  // Match Header + Data + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                              results->rawlen - offset, nbits,
                              kElectraAcHdrMark - 2000, kElectraAcHdrMark + 2000, kElectraAcHdrSpace - 1000, kElectraAcHdrSpace + 1000,
                              kElectraAcBitMark - 500, kElectraAcBitMark + 400, kElectraAcOneSpace - 400, kElectraAcOneSpace + 400,
                              kElectraAcBitMark - 500, kElectraAcBitMark + 400, kElectraAcZeroSpace - 400, kElectraAcZeroSpace + 400,
                              kElectraAcBitMark - 500, kElectraAcBitMark + 400, kElectraAcMessageGap - 25000, kElectraAcMessageGap + 25000, true,
                              _tolerance, 0, false))
    return false;

  // Compliance
  if (strict)
  {
    // Verify the checksum.
    if (!IRElectraAc_validChecksum(results->state, kElectraAcStateLength))
      return false;
  }

  // Success
  if (results->state[0] == 0xCA)
  {
    os_trace("%s CASPER 2024 success\r\n", __func__);
    results->decode_type = CASPER2024_AC;
  }
  else
  {
    os_trace("%s Electra success\r\n", __func__);
    results->decode_type = ELECTRA_AC;
  }
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
  ir_state->swingv = !_.SwingV;
  ir_state->swingh = !_.SwingH;
}

void Casper2024Ac_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
  ElectraProtocol _;
  memset(&_, 0, sizeof(ElectraProtocol));
  memcpy(_.raw, new_code, min(length, kElectraAcStateLength));
  os_trace("%s button: %u, silence: %u, turbo: %u, swingv: %u, swingh: %u\r\n", __func__, _.button, _.silence, _.turbo, _.SwingV2, _.SwingH2);
  ir_state->power = _.Power;
  ir_state->mode = _.Mode2;
  ir_state->fanspeed = _.Fan2 | ((_.silence | _.turbo) << 3); /*11: silence, 13: turbo*/
  ir_state->degrees = _.Temp2 + kElectraAcTempDelta;

  ir_state->swingv = _.SwingV2;
  ir_state->swingh = _.SwingH2;
}

static void sendElectraAC(const uint8_t data[], const uint16_t nbytes,
                          const uint16_t repeat)
{
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++)
    sendGenericBytes(kElectraAcHdrMark, kElectraAcHdrSpace, kElectraAcBitMark,
                     kElectraAcOneSpace, kElectraAcBitMark, kElectraAcZeroSpace,
                     kElectraAcBitMark, kElectraAcMessageGap, data, nbytes,
                     38000, // Complete guess of the modulation frequency.
                     false, // Send data in LSB order per byte
                     0, 50);
  vTaskExitCritical();
}

void ElectraAc_send(state_ir_t state_ir)
{
  os_trace("%s\r\n", __func__);
  ElectraProtocol _;
  for (uint8_t i = 1; i < kElectraAcStateLength - 2; i++)
    _.raw[i] = 0;
  _.raw[0] = 0xC3;
  _.LightToggle = 0x45;

  if (state_ir.swingv != swingv_kOff)
  {
    _.SwingV = ((state_ir.swingv == 1) ? kElectraAcSwingOn : kElectraAcSwingOff);
  }
  if (state_ir.swingh != swingh_kOff)
  {
    _.SwingH = ((state_ir.swingh == 1) ? kElectraAcSwingOn : kElectraAcSwingOff);
  }
  // Power
  _.Power = state_ir.power;
  // Mode
  switch ((uint8_t)state_ir.mode)
  {
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
  switch ((uint8_t)state_ir.fanspeed)
  {
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

#if DEBUG_ENABLE == 1
  os_trace("%s: 0x ", __func__);
  for (uint8_t i = 0; i < kElectraAcStateLength; i++)
  {
    os_trace("%02X-", _.raw[i]);
  }
  os_trace("\r\n");
#endif

  sendElectraAC(_.raw, kElectraAcStateLength, kElectraAcMinRepeat);
}

#define SWINGV_AUTO 0
#define SWINGV_OFF 7
#define SWINGH_AUTO 0
#define SWINGH_OFF 7
void Casper2024Ac_send(state_ir_t state_ir)
{
  os_trace("%s\r\n", __func__);
  ElectraProtocol _;
  uint8_t kState[kElectraAcStateLength] = {0xCA, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0x20, 0x00, 0x45, 0xC3};
  for (uint8_t i = 0; i < kElectraAcStateLength - 1; i++)
    _.raw[i] = kState[i];

  if (state_ir.swingv != swingv_kOff)
  {
    _.SwingV2 = state_ir.swingv;
  }
  if (state_ir.swingh != swingh_kOff)
  {
    _.SwingH2 = state_ir.swingh;
  }
  // Power
  _.Power = state_ir.power;
  // set Temp
  uint8_t newtemp = max(kElectraAcMinTemp, (uint8_t)state_ir.degrees);
  newtemp = min(kElectraAcMaxTemp, newtemp) - kElectraAcTempDelta;
  _.Temp2 = newtemp;
  // fanspeed
  switch ((uint8_t)state_ir.fanspeed)
  {
  case kCasperAcFanLow:
  case kCasperAcFanMed:
  case kCasperAcFanHigh:
  case kCasperAcFanAuto:
    _.Fan2 = state_ir.fanspeed;
    break;
  case kCasperAcFanSilence:
    _.Fan2 = kCasperAcFanLow;
    _.silence = 1;
    break;
  case kCasperAcFanTurbo:
    _.Fan2 = kCasperAcFanHigh;
    _.turbo = 1;
    break;
  default:
    // If we get an unexpected speed, default to Auto.
    _.Fan2 = kCasperAcFanAuto;
  }
  // Mode
  switch ((uint8_t)state_ir.mode)
  {
  case kElectraAcFan:
    _.Temp2 = 0;
    // _.Temp_disable = 0b111;
    _.Mode2 = kElectraAcFan;
    break;
  case kElectraAcAuto:
    _.Temp2 = 0;
    // _.Temp_disable = 0b111;
    _.Mode2 = kElectraAcAuto;
    break;
  case kElectraAcDry:
    _.Fan2 = kCasperAcFanLow;
  case kElectraAcCool:
  case kElectraAcHeat:
    _.Mode2 = state_ir.mode;
    break;
  default:
    // If we get an unexpected mode, default to Cool.
    _.Mode2 = kElectraAcCool;
  }

  // calculate checksum
  _.Sum = IRElectraAc_calcChecksum(_.raw, kElectraAcStateLength);
#if DEBUG_ENABLE == 1
  os_trace("%s: 0x ", __func__);
  for (uint8_t i = 0; i < kElectraAcStateLength; i++)
  {
    os_trace("%02X-", _.raw[i]);
  }
  os_trace("\r\n");
#endif
  sendElectraAC(_.raw, kElectraAcStateLength, kElectraAcMinRepeat);
}