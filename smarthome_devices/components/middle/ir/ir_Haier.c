#include "ir_Haier.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

// Constants
const uint16_t kHaierAcHdr = 3000;
const uint16_t kHaierAcHdrGap = 4300;
const uint16_t kHaierAcBitMark = 520;
const uint16_t kHaierAcOneSpace = 1650;
const uint16_t kHaierAcZeroSpace = 650;
const uint32_t kHaierAcMinGap = 150000; // Completely made up value.
static uint8_t pre_HaierACYRW02_Temp = 0;

static bool IRHaierAC_validChecksum(uint8_t state[], const uint16_t length)
{
  if (length < 2)
    return false; // 1 byte of data can't have a checksum.
  return (state[length - 1] == sumBytesState(state, length - 1, 0));
}

bool decodeHaierAC(decode_results *results, uint16_t offset,
                   const uint16_t nbits, const bool strict)
{
  if (strict)
  {
    if (nbits != kHaierACBits)
      return false; // Not strictly a HAIER_AC message.
  }

  if (results->rawlen <= (2 * nbits + kHeader) + kFooter - 1 + offset)
    return false; // Can't possibly be a valid HAIER_AC message.

  // Pre-Header
  if (!matchLimit(results->rawbuf[offset++], kHaierAcHdr - 750, kHaierAcHdr + 750))
    return false;
  if (!matchLimit(results->rawbuf[offset++], kHaierAcHdr - 750, kHaierAcHdr + 750))
    return false;

  // Match Header + Data + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                              results->rawlen - offset, nbits,
                              kHaierAcHdr - 750, kHaierAcHdr + 750, kHaierAcHdrGap - 1075, kHaierAcHdrGap + 1075,
                              kHaierAcBitMark - 420, kHaierAcBitMark + 420, kHaierAcOneSpace - 550, kHaierAcOneSpace + 550,
                              kHaierAcBitMark - 420, kHaierAcBitMark + 420, kHaierAcZeroSpace - 420, kHaierAcZeroSpace + 420,
                              kHaierAcBitMark - 420, kHaierAcBitMark + 420, kHaierAcMinGap - 100000, kHaierAcMinGap + 37500, true,
                              _tolerance, kMarkExcess, true))
    return false;

  // Compliance
  if (strict)
  {
    if (results->state[0] != kHaierAcPrefix)
      return false;
    if (!IRHaierAC_validChecksum(results->state, nbits / 8))
    {
      os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }
  os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = HAIER_AC;
  results->bits = nbits;
  return true;
}

static bool IRHaierAC176_validChecksum(const uint8_t state[], const uint16_t length)
{
  if (length < 2)
    return false; // 1 byte of data can't have a checksum.
  if (length < kHaierAC160StateLength)
  { // Is it too short?
    // Then it is just a checksum of the whole thing.
    return (state[length - 1] == sumBytesState(state, length - 1, 0));
  }
  else
  { // It is long enough for two checksums.
    return (state[kHaierACYRW02StateLength - 1] ==
            sumBytesState(state, kHaierACYRW02StateLength - 1, 0)) &&
           (state[length - 1] ==
            sumBytesState(state + kHaierACYRW02StateLength,
                          length - kHaierACYRW02StateLength - 1, 0));
  }
}

static bool IRHaierACYRW02_validChecksum(const uint8_t state[],
                                         const uint16_t length)
{
  return IRHaierAC176_validChecksum(state, length);
}

bool decodeHaierACYRW02(decode_results *results, uint16_t offset,
                        const uint16_t nbits, const bool strict)
{
  if (strict)
  {
    if (nbits != kHaierACYRW02Bits)
      return false; // Not strictly a HAIER_AC_YRW02 message.
  }

  // The protocol is almost exactly the same as HAIER_AC
  if (!decodeHaierAC(results, offset, nbits, false))
    return false;

  // Compliance
  if (strict)
  {
    if (results->state[0] != kHaierAcYrw02ModelA)
      return false;
    if (!IRHaierACYRW02_validChecksum(results->state, nbits / 8))
    {
      os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }
  os_trace("%s success\r\n", __func__);
  // Success
  // It looks correct, but we haven't check the checksum etc.
  results->decode_type = HAIER_AC_YRW02;
  return true;
}

void IRHaierACYRW02_setRaw(const uint8_t new_code[], state_ir_t *ir_state)
{
  HaierAc176Protocol _;
  memset(&_, 0, sizeof(HaierAc176Protocol));
  memcpy(_.raw, new_code, kHaierACYRW02StateLength);

  ir_state->power = _.Power;
  ir_state->mode = _.Mode;
  ir_state->model = _.Model;
  ir_state->fanspeed = _.Fan;
  ir_state->degrees = _.Temp + kHaierAcYrw02MinTempC;
  ir_state->swingv = _.SwingV;
  ir_state->swingh = _.SwingH;
  pre_HaierACYRW02_Temp = ir_state->degrees;
}

static void sendHaierAC(const unsigned char data[], const uint16_t nbytes,
                        const uint16_t repeat)
{
  if (nbytes < kHaierACStateLength)
    return;
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++)
  {
    enableIROut(38000, 50);
    mark(kHaierAcHdr);
    space(kHaierAcHdr);
    sendGenericBytes(kHaierAcHdr, kHaierAcHdrGap, kHaierAcBitMark, kHaierAcOneSpace,
                     kHaierAcBitMark, kHaierAcZeroSpace, kHaierAcBitMark,
                     kHaierAcMinGap, data, nbytes, 38, true,
                     0, // Repeats handled elsewhere
                     50);
  }
  vTaskExitCritical();
}

static void sendHaierACYRW02(const unsigned char data[], const uint16_t nbytes,
                             const uint16_t repeat)
{
  if (nbytes >= kHaierACYRW02StateLength)
    sendHaierAC(data, nbytes, repeat);
}

void HaierACYRW02_send(state_ir_t state_ir)
{
  HaierAc176Protocol _;
  memset(&_, 0, sizeof(HaierAc176Protocol));
  _.Model = kHaierAcYrw02ModelA;
  _.Prefix2 = kHaierAc176Prefix;
  // _.Health = true;

  _.Temp = (uint8_t)state_ir.degrees - kHaierAcYrw02MinTempC;
  _.Fan = state_ir.fanspeed;
  _.Power = state_ir.power;
  if (state_ir.swingv != swingv_kOff)
    _.SwingV = state_ir.swingv;
  if (state_ir.swingh != swingh_kOff)
    _.SwingH = state_ir.swingh;
  _.Button = kHaierAcYrw02ButtonPower;

  if (pre_HaierACYRW02_Temp != state_ir.degrees)
  {
    if (state_ir.degrees > pre_HaierACYRW02_Temp)
    {
      _.Button = kHaierAcYrw02ButtonTempUp;
    }
    else
    {
      _.Button = kHaierAcYrw02ButtonTempDown;
    }
    pre_HaierACYRW02_Temp = state_ir.degrees;
  }

  // calc checksum
  _.Sum = sumBytesState(_.raw, kHaierACYRW02StateLength - 1, 0);

#if DEBUG_ENABLE == 1
  os_trace("%s: 0x ", __func__);
  for (uint8_t i = 0; i < kHaierACYRW02StateLength; i++)
  {
    os_trace("%02X-", _.raw[i]);
  }
  os_trace("\r\n");
#endif

  sendHaierACYRW02(_.raw, kHaierACYRW02StateLength, kHaierAcYrw02DefaultRepeat);
}

bool decodeHaierAC176(decode_results *results, uint16_t offset,
                      const uint16_t nbits, const bool strict)
{
  if (strict)
  {
    if (nbits != kHaierAC176Bits)
      return false; // Not strictly a HAIER_AC176 message.
  }

  // The protocol is almost exactly the same as HAIER_AC
  if (!decodeHaierAC(results, offset, nbits, false))
    return false;

  // Compliance
  if (strict)
  {
    if ((results->state[0] != kHaierAcYrw02ModelA) &&
        (results->state[0] != kHaierAcYrw02ModelB))
      return false;
    if (!IRHaierAC176_validChecksum(results->state, nbits / 8))
      return false;
  }
  os_trace("%s success\r\n", __func__);
  // Success
  // It looks correct, but we haven't check the checksum etc.
  results->decode_type = HAIER_AC176;
  return true;
}

bool decodeHaierAC160(decode_results *results, uint16_t offset,
                      const uint16_t nbits, const bool strict)
{
  if (strict)
  {
    if (nbits != kHaierAC160Bits)
      return false; // Not strictly a HAIER_AC160 message.
  }

  // The protocol is almost exactly the same as HAIER_AC
  if (!decodeHaierAC(results, offset, nbits, false))
    return false;

  // Compliance
  if (strict)
  {
    if (!IRHaierAC176_validChecksum(results->state, nbits / 8))
      return false;
  }
  os_trace("%s success\r\n", __func__);
  // Success
  // It looks correct, but we haven't check the checksum etc.
  results->decode_type = HAIER_AC160;
  return true;
}
