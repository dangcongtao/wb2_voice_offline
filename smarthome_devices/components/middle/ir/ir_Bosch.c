#include "ir_Bosch.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

bool decodeBosch144(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits +
                        kBosch144NrOfSections * (kHeader + kFooter) -
                        1 + offset)
    return false;  // Can't possibly be a valid BOSCH144 message.
  if (strict && nbits != kBosch144Bits)
    return false;      // Not strictly a BOSCH144 message.
  if (nbits % 8 != 0)  // nbits has to be a multiple of nr. of bits in a byte.
    return false;
  if (nbits % kBosch144NrOfSections != 0)
    return false;  // nbits has to be a multiple of kBosch144NrOfSections.
  const uint16_t kSectionBits = nbits / kBosch144NrOfSections;
  const uint16_t kSectionBytes = kSectionBits / 8;
  const uint16_t kNBytes = kSectionBytes * kBosch144NrOfSections;
  // Capture each section individually
  for (uint16_t pos = 0, section = 0;
       pos < kNBytes;
       pos += kSectionBytes, section++) {
    uint16_t used = 0;
    // Section Header + Section Data + Section Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, kSectionBits,
                        kBoschHdrMark-1000, kBoschHdrMark+1000, kBoschHdrSpace-1000, kBoschHdrSpace+1000,
                        kBoschBitMark-350, kBoschBitMark+350, kBoschOneSpace-450, kBoschOneSpace+450,
                        kBoschBitMark-350, kBoschBitMark+350, kBoschZeroSpace-400, kBoschZeroSpace+400,
                        kBoschBitMark-350, kBoschBitMark+350, kBoschFooterSpace-1200, kBoschFooterSpace+1200,
                        section >= kBosch144NrOfSections - 1,
                        _tolerance, kMarkExcess, true);
    if (!used) return false;  // Didn't match.
    offset += used;
  }

  // Compliance

  // Success
  os_trace("%s success\r\n", __func__);
  results->decode_type = BOSCH144;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRBosch144AC_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state) {
  const uint16_t len = min(length, kBosch144StateLength);
  const uint16_t lenOff = sizeof(kBosch144Off);
// Is it an off message?
    bool powerFlag;
    Bosch144Protocol _;
    memset(&_, 0, sizeof(Bosch144Protocol));
    if (memcmp(kBosch144Off, new_code, min(lenOff, len)) == 0)
        powerFlag = false;  // It is.
    else
        powerFlag = true;
    memcpy(_.raw, new_code, len);

    ir_state->power = powerFlag;
    ir_state->mode = (_.ModeS1 << 1) + _.ModeS3;
    ir_state->fanspeed = (_.FanS1 << 6) + _.FanS3;

    uint8_t temp = (_.TempS1 << 1) + _.TempS3;
    uint8_t retemp = 25;
    for (uint8_t i = 0; i < kBosch144TempRange; i++) {
        if (temp == kBosch144TempMap[i]) {
            retemp = kBosch144TempMin + i;
        }
    }
    ir_state->degrees = retemp;
}

