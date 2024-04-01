#include "ir_Voltas.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

// Constants
const uint16_t kVoltasBitMark = 1026;   ///< uSeconds.
const uint16_t kVoltasOneSpace = 2553;  ///< uSeconds.
const uint16_t kVoltasZeroSpace = 554;  ///< uSeconds.
const uint16_t kVoltasFreq = 38000;     ///< Hz.

/// Calculate the checksum is valid for a given state.
/// @param[in] state The array to calculate the checksum of.
/// @param[in] length The length of the state array.
/// @return The valid checksum value for the state.
static uint8_t calcChecksum(const uint8_t state[], const uint16_t length) {
  uint8_t result = 0;
  if (length)
    result = sumBytesState(state, length - 1, 0);
  return ~result;
}

static bool IRVoltas_validChecksum(const uint8_t state[], const uint16_t length) {
  if (length) return state[length - 1] == calcChecksum(state, length);
  return true;
}



bool decodeVoltas(decode_results *results, uint16_t offset,
                          const uint16_t nbits, const bool strict) {
  if (strict && nbits != kVoltasBits) return false;

  // Data + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    0, 0, 0, 0,  // No header
                    kVoltasBitMark-500, kVoltasBitMark+500, kVoltasOneSpace-600, kVoltasOneSpace+600,
                    kVoltasBitMark-500, kVoltasBitMark+500, kVoltasZeroSpace-450, kVoltasZeroSpace+450,
                    kVoltasBitMark-500, kVoltasBitMark+500, kDefaultMessageGap-25000, kDefaultMessageGap+25000, true, kUseDefTol, kMarkExcess, true)) return false;

  // Compliance
  if (strict && !IRVoltas_validChecksum(results->state, nbits / 8))
  {
    os_trace("%s invalid checksum\r\n", __func__);
    return false;
  }
  // Success
  os_trace("%s success\r\n", __func__);
  results->decode_type = VOLTAS;
  results->bits = nbits;
  return true;
}

void IRVoltas_setRaw(const uint8_t new_code[], state_ir_t *ir_state) {
    VoltasProtocol _;
    memset(&_, 0, sizeof(VoltasProtocol));
    memcpy(_.raw, new_code, kVoltasStateLength);
    switch (_.SwingHChange) {
      case kVoltasSwingHNoChange:
        ir_state->model = kVoltas122LZF;
      default:
        ir_state->model = kVoltasUnknown;
    }
    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.FanSpeed;
    ir_state->degrees = _.Temp + kVoltasMinTemp;
    ir_state->swingv = (_.SwingV == 0b111);
    if (ir_state->model != kVoltas122LZF) {
        ir_state->swingh = _.SwingH;
    }
    ir_state->turbo = _.Turbo;
    ir_state->econo = _.Econo;
}
