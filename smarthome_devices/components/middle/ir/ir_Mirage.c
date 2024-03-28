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
    
    switch (_model) {
        case KKG29AC1:
        ir_state->swingv = (_.SwingV ? kMirageAcSwingVAuto : kMirageAcSwingVOff);
        default:
        ir_state->swingv = (_.SwingAndPower - (ir_state->power ? 0 : kMirageAcPowerOff));
    }
    switch (_model) {
    case KKG29AC1: ir_state->swingh = _.SwingH;
    default: 
        ir_state->swingh = false;
        break;
  }
}

static void sendMirage(const uint8_t data[], const uint16_t nbytes,
                        const uint16_t repeat) {
    vTaskEnterCritical();
    sendGenericBytes(kMirageHdrMark, kMirageHdrSpace,
              kMirageBitMark, kMirageOneSpace,
              kMirageBitMark, kMirageZeroSpace,
              kMirageBitMark, kMirageGap,
              data, nbytes, kMirageFreq, false,  // LSB
              repeat, kDutyDefault);
    vTaskExitCritical();
}

void IRMirageAc_send(state_ir_t state_ir)
{
    uint8_t kReset[kMirageStateLength] = {
      0x56, 0x6C, 0x00, 0x00, 0x20, 0x1A, 0x00, 0x00,
      0x0C, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x42};

    Mirage120Protocol _;
    memset(&_, 0, sizeof(Mirage120Protocol));
    memcpy(_.raw, kReset, kMirageStateLength);
    _model = getModel(_.raw);
    _model = KKG9AC1;

    switch (_model) {
        case KKG29AC1:
        _.Power = state_ir.power ? kMirageAcKKG29AC1PowerOn : kMirageAcKKG29AC1PowerOff;
        break;
        default:
        // In order to change the power setting, it seems must be less than
        // kMirageAcPowerOff. kMirageAcPowerOff is larger than half of the
        // possible value stored in the allocated bit space.
        // Thus if the value is larger than kMirageAcPowerOff the power is off.
        // Less than, then power is on.
        // We can't just aribitarily add or subtract the value (which analysis
        // indicates is how the power status changes. Very weird, I know!) as that
        // is not an idempotent action, we must check if the addition or
        // substraction is needed first. e.g. via getPower()
        // i.e. If we added or subtracted twice, we would cause a wrap of the
        // integer and not get the desired result.
        if (state_ir.power)
            _.SwingAndPower -= (_.SwingAndPower < kMirageAcPowerOff) ? 0 : kMirageAcPowerOff;
        else
            _.SwingAndPower += (_.SwingAndPower < kMirageAcPowerOff) ? kMirageAcPowerOff : 0;
    }

    switch ((uint8_t)state_ir.mode) {
        case kMirageAcCool:
        case kMirageAcDry:
        case kMirageAcHeat:
        case kMirageAcFan:
        case kMirageAcRecycle:
        _.Mode = state_ir.mode;
        // Reset turbo if we have to.
        //   setTurbo(getTurbo());
        break;
        default:  // Default to cool mode for anything else.
        _.Mode = kMirageAcCool;
    }

    _.Fan = ((uint8_t)state_ir.fanspeed <= kMirageAcFanLow) ? (uint8_t)state_ir.fanspeed : kMirageAcFanAuto;

    uint8_t celsius = max((uint8_t)state_ir.degrees, kMirageAcMinTemp);
    _.Temp = min(celsius, kMirageAcMaxTemp) + kMirageAcTempOffset;
    // swingv
    switch ((uint8_t)state_ir.swingv) {
        case kMirageAcSwingVOff:
        case kMirageAcSwingVLowest:
        case kMirageAcSwingVLow:
        case kMirageAcSwingVMiddle:
        case kMirageAcSwingVHigh:
        case kMirageAcSwingVHighest:
        case kMirageAcSwingVAuto:
        switch (_model) {
            case KKG29AC1:
            _.SwingV = (state_ir.swingv != kMirageAcSwingVOff);
            break;
            default:
            //   const bool power = getPower();
              _.SwingAndPower = state_ir.swingv;
            //   // Power needs to be reapplied after overwriting SwingAndPower
            if (state_ir.power)
                _.SwingAndPower -= (_.SwingAndPower < kMirageAcPowerOff) ? 0 : kMirageAcPowerOff;
            else
                _.SwingAndPower += (_.SwingAndPower < kMirageAcPowerOff) ? kMirageAcPowerOff : 0;
            //   setPower(power);
            break;
        }
        break;
        default:  // Default to Auto for anything else.
        //   setSwingV(kMirageAcSwingVAuto);
            break;
    }

    switch (_model) {
    case KKG29AC1:
      _.SwingH = state_ir.swingh;
      break;
    default:
      break;
  }

    _.Sum = IRMirageAc_calculateChecksum(_.raw);
    sendMirage(_.raw, kMirageStateLength, kMirageMinRepeat);
}
