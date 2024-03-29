#include "ir_Amcor.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

// Constants
const uint16_t kAmcorHdrMark = 8200;
const uint16_t kAmcorHdrSpace = 4200;
const uint16_t kAmcorOneMark = 1500;
const uint16_t kAmcorZeroMark = 600;
const uint16_t kAmcorOneSpace = kAmcorZeroMark;
const uint16_t kAmcorZeroSpace = kAmcorOneMark;
const uint16_t kAmcorFooterMark = 1900;
const uint16_t kAmcorGap = 34300;
const uint8_t  kAmcorTolerance = 40;


static uint8_t IRAmcorAc_calcChecksum(const uint8_t state[], const uint16_t length) {
  return sumNibblesByte(state, length - 1, 0);
}

static bool IRAmcorAc_validChecksum(const uint8_t state[], const uint16_t length) {
  return (state[length - 1] == IRAmcorAc_calcChecksum(state, length));
}

bool decodeAmcor(decode_results *results, uint16_t offset,
                         const uint16_t nbits, const bool strict) {
  if (results->rawlen <= 2 * nbits + kHeader - 1 + offset)
    return false;  // Can't possibly be a valid Amcor message.
  if (strict && nbits != kAmcorBits)
    return false;  // We expect Amcor to be 64 bits of message.

  uint16_t used;
  // Header + Data Block (64 bits) + Footer
  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, 64,
                      kAmcorHdrMark-2000, kAmcorHdrMark+2000, kAmcorHdrSpace-1000, kAmcorHdrSpace+1000,
                      kAmcorOneMark-500, kAmcorOneMark+500, kAmcorOneSpace-400, kAmcorOneSpace+400,
                      kAmcorZeroMark-400, kAmcorZeroMark+400, kAmcorZeroSpace-500, kAmcorZeroSpace+500,
                      kAmcorFooterMark-500, kAmcorFooterMark+500, kAmcorGap-7000, kAmcorGap+7000, true,
                      kAmcorTolerance, 0, false);
  if (!used) return false;
  offset += used;

  if (strict) {
    if (!IRAmcorAc_validChecksum(results->state, kAmcorStateLength)){
        os_trace("%s invalid checksum\r\n", __func__);
        return false;
    }
  }
    os_trace("%s success\r\n", __func__);
  // Success
  results->bits = nbits;
  results->decode_type = AMCOR;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRAmcorAc_setInfo(const uint8_t state[], state_ir_t *ir_state)
{
    AmcorProtocol _;
    memset(&_, 0, sizeof(AmcorProtocol));
    memcpy(_.raw, state, kAmcorStateLength);

    ir_state->power = (_.Power == kAmcorPowerOn);
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp;
}