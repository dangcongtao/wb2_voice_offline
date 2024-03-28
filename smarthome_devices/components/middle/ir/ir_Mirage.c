#include "ir_Mirage.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

// Constants
const uint16_t kMirageHdrMark = 8360;            ///< uSeconds
const uint16_t kMirageBitMark = 554;             ///< uSeconds
const uint16_t kMirageHdrSpace = 4248;           ///< uSeconds
const uint16_t kMirageOneSpace = 1592;           ///< uSeconds
const uint16_t kMirageZeroSpace = 545;           ///< uSeconds
const uint32_t kMirageGap = kDefaultMessageGap;  ///< uSeconds (just a guess)
const uint16_t kMirageFreq = 38000;              ///< Hz. (Just a guess)

const uint8_t kMirageAcKKG29AC1PowerOn  = 0b00;  // 0
const uint8_t kMirageAcKKG29AC1PowerOff = 0b11;  // 3

static mirage_ac_remote_model_t _model;

static uint8_t IRMirageAc_calculateChecksum(const uint8_t *data) {
  return sumNibblesByte(data, kMirageStateLength - 1, 0);
}

static bool IRMirageAc_validChecksum(const uint8_t *data) {
  return IRMirageAc_calculateChecksum(data) == data[kMirageStateLength - 1];
}

bool decodeMirage(decode_results *results, uint16_t offset,
                          const uint16_t nbits, const bool strict) {
  if (strict && nbits != kMirageBits) return false;  // Compliance.

  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    kMirageHdrMark-2000, kMirageHdrMark+2000, kMirageHdrSpace-1000, kMirageHdrSpace+1000,
                    kMirageBitMark-400, kMirageBitMark+400, kMirageOneSpace-500, kMirageOneSpace+500,
                    kMirageBitMark-400, kMirageBitMark+400, kMirageZeroSpace-400, kMirageZeroSpace+400,
                    kMirageBitMark-400, kMirageBitMark+400, kMirageGap-25000, kMirageGap+25000, true,
                    kUseDefTol, kMarkExcess, false)) return false;
  // Compliance
  if (strict && !IRMirageAc_validChecksum(results->state)) return false;

  // Success
  os_trace("%s success\r\n", __func__);
  results->decode_type = MIRAGE;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

static mirage_ac_remote_model_t getModel(const uint8_t *state) {
  Mirage120Protocol p;
  memcpy(p.raw, state, kMirageStateLength);
  // Check for KKG29AC1 specific settings.
  if (p.RecycleHeat || p.Filter || p.Sleep_Kkg29ac1 || p.CleanToggle ||
      p.IFeel || p.OffTimerEnable || p.OnTimerEnable)
    return KKG29AC1;
  // Check for things specific to KKG9AC1
  if ((p.Minutes || p.Seconds) ||  // Is part of the clock set?
      // Are the timer times set, but not enabled? (enable check filtered above)
      (p.OffTimerHours || p.OffTimerMins) ||
      (p.OnTimerHours || p.OnTimerMins))
    return KKG9AC1;
  // As the above test has a 1 in 3600+ (for 1 second an hour) chance of a false
  // negative in theory, we are going assume that anything left should be a
  // KKG29AC1 model.
  return KKG29AC1;  // Default.
}

void IRMirageAc_setInfo(const uint8_t *data, state_ir_t *ir_state) {
    Mirage120Protocol _;
    memset(&_, 0, sizeof(Mirage120Protocol));
    memcpy(_.raw, data, kMirageStateLength);
    _model = getModel(_.raw);

    switch (_model) {
        case KKG29AC1:
            ir_state->power =  (_.Power == kMirageAcKKG29AC1PowerOn);
        default:
            ir_state->power = (_.SwingAndPower < kMirageAcPowerOff);
    }

    ir_state->model = _model;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp - kMirageAcTempOffset;

}
