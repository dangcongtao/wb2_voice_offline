#include "ir_Teco.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

// Constants
// using SPACE modulation.
const uint16_t kTecoHdrMark = 9000;
const uint16_t kTecoHdrSpace = 4440;
const uint16_t kTecoBitMark = 620;
const uint16_t kTecoOneSpace = 1650;
const uint16_t kTecoZeroSpace = 580;
const uint32_t kTecoGap = kDefaultMessageGap;  // Made-up value. Just a guess.


bool decodeTeco(decode_results* results, uint16_t offset,
                        const uint16_t nbits, const bool strict) {
  if (strict && nbits != kTecoBits) return false;  // Not what is expected

  uint64_t data = 0;
  // Match Header + Data + Footer
  if (!matchGenericDataLimit(results->rawbuf + offset, &data,
                    results->rawlen - offset, nbits,
                    kTecoHdrMark-2250, kTecoHdrMark+2250, kTecoHdrSpace - 1000, kTecoHdrSpace + 1000,
                    kTecoBitMark-370, kTecoBitMark+370, kTecoOneSpace-500, kTecoOneSpace+500,
                    kTecoBitMark-370, kTecoBitMark+370, kTecoZeroSpace-400, kTecoZeroSpace+400,
                    kTecoBitMark-370, kTecoBitMark+370, kTecoGap-25000, kTecoGap+25000, true,
                    _tolerance, kMarkExcess, false)) return false;

  // Success
  os_trace("%s success\r\n", __func__);
  results->decode_type = TECO;
  results->bits = nbits;
  results->value = data;
  results->address = 0;
  results->command = 0;
  return true;
}

void IRTecoAc_setInfo(const uint64_t new_code, state_ir_t *ir_state)
{ 
    TecoProtocol _;
    memset(&_, 0, sizeof(TecoProtocol));
    _.raw = new_code; 

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kTecoMinTemp;
    ir_state->swingv = _.Swing ? 1 : 0;
}

void sendTeco(const uint64_t data, const uint16_t nbits,
                      const uint16_t repeat) {

    vTaskEnterCritical();
    sendGeneric(kTecoHdrMark, kTecoHdrSpace, kTecoBitMark, kTecoOneSpace,
              kTecoBitMark, kTecoZeroSpace, kTecoBitMark, kTecoGap,
              data, nbits, 38000, false, repeat, kDutyDefault);
    vTaskExitCritical();
}

void IRTecoAc_send(state_ir_t state_ir)
{
    TecoProtocol _;
    memset(&_, 0, sizeof(TecoProtocol));
    _.raw = kTecoReset;
    _.Power = state_ir.power;
    _.Mode = state_ir.mode;
    _.Fan = state_ir.fanspeed;
    _.Swing = state_ir.swingv;

    sendTeco(_.raw, kTecoBits, kTecoDefaultRepeat);

}
