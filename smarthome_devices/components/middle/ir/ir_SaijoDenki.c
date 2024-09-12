#include "ir_SaijoDenki.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

// Constants
const uint16_t kSaijoDenkiHdrMark = 9000;
const uint16_t kSaijoDenkiHdrSpace = 4500;
const uint16_t kSaijoDenkiBitMark = 540;
const uint16_t kSaijoDenkiOneSpace = 1740;
const uint16_t kSaijoDenkiZeroSpace = 590;

static uint8_t IRSaijoDenki_calcChecksum(const uint8_t state[],
                                         const uint16_t length)
{
  return sumBytesState(state, length - 1, 0);
}

static bool IRSaijoDenki_validChecksum(const uint8_t state[], const uint16_t length)
{
  return state[length - 1] == IRSaijoDenki_calcChecksum(state, length);
}

bool decodeSaijoDenki(decode_results *results, uint16_t offset,
                      const uint16_t nbits, const bool strict)
{
  if (strict && nbits != kSaijoDenkiBits)
    return false;

  uint16_t used = 0;

  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                                results->rawlen - offset, nbits,
                                kSaijoDenkiHdrMark - 1000, kSaijoDenkiHdrMark + 1000, kSaijoDenkiHdrSpace - 500, kSaijoDenkiHdrSpace + 500,
                                kSaijoDenkiBitMark - 400, kSaijoDenkiBitMark + 400, kSaijoDenkiOneSpace - 500, kSaijoDenkiOneSpace + 500,
                                kSaijoDenkiBitMark - 400, kSaijoDenkiBitMark + 400, kSaijoDenkiZeroSpace - 400, kSaijoDenkiZeroSpace + 400,
                                kSaijoDenkiBitMark - 400, kSaijoDenkiBitMark + 400, kDefaultMessageGap - 25000, kDefaultMessageGap + 25000,
                                false, _tolerance, kMarkExcess,
                                true); // LSB
  if (used == 0)
    return false; // We failed to find any data.

  // if (strict && !IRSaijoDenki_validChecksum(results->state, kSaijoDenkiStateLength)){
  //   os_trace("%s invalid checksum\r\n", __func__);
  //   return false;
  // }
  // os_trace("%s checksum: %02X %02X\r\n", __func__, (uint8_t)(~IRSaijoDenki_calcChecksum(results->state, kSaijoDenkiStateLength)), results->state[kSaijoDenkiStateLength-1]);

  os_trace("%s success\r\n", __func__);
  // Succes
  results->decode_type = SAIJODENKI;
  results->bits = nbits;

  return true;
}

void SaijoDenki_setInfo(const uint8_t state[], state_ir_t *ir_state)
{
  SaijoDenkiProtocol _;
  memset(&_, 0, sizeof(SaijoDenkiProtocol));
  memcpy(_.raw, state, kSaijoDenkiStateLength);

  ir_state->protocol = SAIJODENKI;
  ir_state->power = _.Power;
  // ir_state->celsius = !_.useFahrenheit;
  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;
  ir_state->degrees = (float)_.Temp / 2;
  os_trace("%s Power:%u, Mode: %u, Fan: %u, Swing: %u, temp: %f\r\n", __func__, _.Power, _.Mode, _.Fan, _.SwingV, ir_state->degrees);
}

uint8_t sendSaijoDenkiAC(const uint8_t data[], const uint16_t nbytes)
{
  if (nbytes != kSaijoDenkiStateLength)
  {
    os_trace("%s length data invalid\r\n", __func__);
    return 0;
  }
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= kSaijoDenkiDefaultRepeat; r++)
  {
    sendGenericBytes(kSaijoDenkiHdrMark, kSaijoDenkiHdrSpace, kSaijoDenkiBitMark,
                     kSaijoDenkiOneSpace, kSaijoDenkiBitMark, kSaijoDenkiZeroSpace,
                     kSaijoDenkiBitMark, kDefaultMessageGap, data, nbytes,
                     38000, // Complete guess of the modulation frequency.
                     false, // Send data in LSB order per byte
                     0, 50);
  }
  vTaskExitCritical();
  return 1;
}