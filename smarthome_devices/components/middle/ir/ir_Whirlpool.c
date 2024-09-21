#include "ir_Whirlpool.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"


// Constants
const uint16_t kWhirlpoolAcHdrMark = 8950;
const uint16_t kWhirlpoolAcHdrSpace = 4484;
const uint16_t kWhirlpoolAcBitMark = 597;
const uint16_t kWhirlpoolAcOneSpace = 1649;
const uint16_t kWhirlpoolAcZeroSpace = 533;
const uint16_t kWhirlpoolAcGap = 7920;
const uint32_t kWhirlpoolAcMinGap = kDefaultMessageGap;  // Just a guess.
const uint8_t kWhirlpoolAcSections = 3;


static bool IRWhirlpoolAc_validChecksum(const uint8_t state[],
                                  const uint16_t length) {
  if (length > kWhirlpoolAcChecksumByte1 &&
      state[kWhirlpoolAcChecksumByte1] !=
          xorBytes(state + 2, kWhirlpoolAcChecksumByte1 - 1 - 2, 0)) {
    os_trace("%s DEBUG: First Whirlpool AC checksum failed.\r\n", __func__);
    return false;
  }
  if (length > kWhirlpoolAcChecksumByte2 &&
      state[kWhirlpoolAcChecksumByte2] !=
          xorBytes(state + kWhirlpoolAcChecksumByte1 + 1,
                   kWhirlpoolAcChecksumByte2 - kWhirlpoolAcChecksumByte1 - 1, 0)) {
    os_trace("%s DEBUG: Second Whirlpool AC checksum failed.\r\n", __func__);
    return false;
  }
  // State is too short to have a checksum or everything checked out.
  return true;
}

bool decodeWhirlpoolAC(decode_results *results, uint16_t offset,
                               const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits + 4 + kHeader + kFooter - 1 + offset)
    return false;  // Can't possibly be a valid Whirlpool A/C message.
  if (strict) {
    if (nbits != kWhirlpoolAcBits) return false;
  }

  uint8_t sectionSize[] = {6, 8, 7};

  // Header
  if (!matchLimit(results->rawbuf[offset++], kWhirlpoolAcHdrMark-2250, kWhirlpoolAcHdrMark+2250)) return false;
  if (!matchLimit(results->rawbuf[offset++], kWhirlpoolAcHdrSpace-1125, kWhirlpoolAcHdrSpace+1125))
    return false;

  if (results->rawlen == 688)
  {
    memcpy(results->rawbuf, &results->rawbuf[344], 344*sizeof(uint16_t));
    results->rawlen = 344;
  }

  // Data Sections
  uint16_t pos = 0;
  for (uint8_t section = 0; section < kWhirlpoolAcSections;
       section++) {
    uint16_t used;
    // Section Data
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, sectionSize[section] * 8,
                        0, 0, 0, 0,
                        kWhirlpoolAcBitMark-450, kWhirlpoolAcBitMark+450, kWhirlpoolAcOneSpace-500, kWhirlpoolAcOneSpace+500,
                        kWhirlpoolAcBitMark-450, kWhirlpoolAcBitMark+450, kWhirlpoolAcZeroSpace- 433, kWhirlpoolAcZeroSpace+437,
                        kWhirlpoolAcBitMark-450, kWhirlpoolAcBitMark+450, kWhirlpoolAcGap-2000, kWhirlpoolAcGap+2000,
                        section >= kWhirlpoolAcSections - 1,
                        _tolerance, kMarkExcess, false);
    if (used == 0) return false;
    offset += used;
    pos += sectionSize[section];
  }

  // Compliance
  if (strict) {
    // Re-check we got the correct size/length due to the way we read the data.
    if (pos * 8 != nbits) return false;
    if (!IRWhirlpoolAc_validChecksum(results->state, nbits / 8)){
        os_trace("%s invalid checksum\r\n", __func__);
        return false;
    }
  }

    WhirlpoolProtocol _;
    memset(&_, 0, sizeof(WhirlpoolProtocol));
    memcpy(_.raw, results->state, kWhirlpoolAcStateLength);

//   state_ir_t ir_state;
//   IRWhirlpoolAc_setRaw(results->state, kWhirlpoolAcStateLength, &ir_state);
    if (_.isCasper && _.J191) {
        os_trace("%s It is Casper AC\r\n");
        return false;
    }

  os_trace("%s success\r\n", __func__);

  // Success
  results->decode_type = WHIRLPOOL_AC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

static bool pre_power = true;

static int8_t getTempOffset(whirlpool_ac_remote_model_t _model){
  switch (_model) {
    case DG11J191:  return -2;
    default:        return 0;
  }
}

void IRWhirlpoolAc_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
    WhirlpoolProtocol _;
    memset(&_, 0, sizeof(WhirlpoolProtocol));
    memcpy(_.raw, new_code, min(length, kWhirlpoolAcStateLength));

    if (_.Power) {
        pre_power = !pre_power;
    }

    if (_.J191)
        ir_state->model = DG11J191;
    else
        ir_state->model = DG11J13A;

    ir_state->power = pre_power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kWhirlpoolAcMinTemp; // + getTempOffset(ir_state->model);
    ir_state->swingv = (_.Swing1 && _.Swing2);

}

static void sendWhirlpoolAC(const unsigned char data[], const uint16_t nbytes,
                             const uint16_t repeat) {
  if (nbytes < kWhirlpoolAcStateLength)
    return;  // Not enough bytes to send a proper message.
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    // Section 1
    sendGenericBytes(kWhirlpoolAcHdrMark, kWhirlpoolAcHdrSpace, kWhirlpoolAcBitMark,
                kWhirlpoolAcOneSpace, kWhirlpoolAcBitMark,
                kWhirlpoolAcZeroSpace, kWhirlpoolAcBitMark, kWhirlpoolAcGap,
                data, 6,  // 6 bytes == 48 bits
                38000,    // Complete guess of the modulation frequency.
                false, 0, 50);
    // Section 2
    sendGenericBytes(0, 0, kWhirlpoolAcBitMark, kWhirlpoolAcOneSpace,
                kWhirlpoolAcBitMark, kWhirlpoolAcZeroSpace, kWhirlpoolAcBitMark,
                kWhirlpoolAcGap, data + 6, 8,  // 8 bytes == 64 bits
                38000,  // Complete guess of the modulation frequency.
                false, 0, 50);
    // Section 3
    sendGenericBytes(0, 0, kWhirlpoolAcBitMark, kWhirlpoolAcOneSpace,
                kWhirlpoolAcBitMark, kWhirlpoolAcZeroSpace, kWhirlpoolAcBitMark,
                kWhirlpoolAcMinGap, data + 14, 7,  // 7 bytes == 56 bits
                38000,  // Complete guess of the modulation frequency.
                false, 0, 50);
  }
  vTaskExitCritical();
}

void Whirlpool_send(state_ir_t state_ir)
{
  WhirlpoolProtocol _;
  memset(&_, 0, sizeof(WhirlpoolProtocol));
  for (uint8_t i = 2; i < kWhirlpoolAcStateLength; i++) _.raw[i] = 0x0;
  _.raw[0] = 0x83;
  _.raw[1] = 0x06;
  _.raw[6] = 0x80;

  // _.Swing1 = 1;
  // _.Swing2 = 1;
  // _.Cmd = kWhirlpoolAcCommandSwing;

  state_ir.model = DG11J191;
    switch ((uint8_t)state_ir.mode) {
    case kWhirlpoolAcAuto:
      // setFan(kWhirlpoolAcFanAuto);
      // _setTemp(kWhirlpoolAcAutoTemp, false);
      // setSleep(false);  // Cancel sleep mode when in auto/6thsense mode.
      // FALL THRU
    case kWhirlpoolAcHeat:
    case kWhirlpoolAcCool:
    case kWhirlpoolAcDry:
    case kWhirlpoolAcFan:
      _.Mode = state_ir.mode;
      break;
    default:
      return;
  }
  // if (mode == kWhirlpoolAcAuto) _.Cmd = kWhirlpoolAcCommand6thSense;
  // int8_t offset = getTempOffset(state_ir.model);
  int8_t offset = 0;
  uint8_t newtemp = max((uint8_t)(kWhirlpoolAcMinTemp + offset), (uint8_t)state_ir.degrees);
  newtemp = min((uint8_t)(kWhirlpoolAcMaxTemp + offset), newtemp);
  _.Temp = newtemp - (kWhirlpoolAcMinTemp + offset);
  _.Cmd = kWhirlpoolAcCommandTemp;

  switch ((uint8_t)state_ir.fanspeed) {
    case kWhirlpoolAcFanAuto:
    case kWhirlpoolAcFanLow:
    case kWhirlpoolAcFanMedium:
    case kWhirlpoolAcFanHigh:
      _.Fan = state_ir.fanspeed;
      break;
  }

  if (pre_power != state_ir.power) {
    _.Power = 1;
    _.Cmd = kWhirlpoolAcCommandPower;
    pre_power = !pre_power;
  }

  _.Sum1 = xorBytes(_.raw + 2, kWhirlpoolAcChecksumByte1 - 1 - 2, 0);
  _.Sum2 = xorBytes(_.raw + kWhirlpoolAcChecksumByte1 + 1,
                kWhirlpoolAcChecksumByte2 - kWhirlpoolAcChecksumByte1 - 1, 0);
  
  sendWhirlpoolAC(_.raw, kWhirlpoolAcStateLength, kWhirlpoolAcDefaultRepeat);
}