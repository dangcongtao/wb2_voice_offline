#include "ir_Daikin.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

static bool IRDaikin_validChecksum(uint8_t state[], const uint16_t length) {
  // Data #1
  if (length < kDaikinSection1Length ||
      state[kDaikinByteChecksum1] != sumBytesState(state, kDaikinSection1Length - 1, 0))
    return false;
  // Data #2
  if (length < kDaikinSection1Length + kDaikinSection2Length ||
      state[kDaikinByteChecksum2] != sumBytesState(state + kDaikinSection1Length,
                                              kDaikinSection2Length - 1, 0))
    return false;
  // Data #3
  if (length < kDaikinSection1Length + kDaikinSection2Length + 2 ||
      state[length - 1] != sumBytesState(state + kDaikinSection1Length +
                                    kDaikinSection2Length,
                                    length - (kDaikinSection1Length +
                                              kDaikinSection2Length) - 1, 0))
    return false;
  return true;
}

static void sendDaikin(const unsigned char data[], const uint16_t nbytes,
                        const uint16_t repeat) {
  if (nbytes < kDaikinStateLengthShort)
    return;  // Not enough bytes to send a proper message.
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    uint16_t offset = 0;
    // Send the header, 0b00000
    sendGeneric(0, 0,  // No header for the header
                kDaikinBitMark, kDaikinOneSpace, kDaikinBitMark,
                kDaikinZeroSpace, kDaikinBitMark, kDaikinZeroSpace + kDaikinGap,
                (uint64_t)0b00000, kDaikinHeaderLength, 38, false, 0, 50);
    // Data #1
    if (nbytes < kDaikinStateLength) {  // Are we using the legacy size?
      // Do this as a constant to save RAM and keep in flash memory
      sendGeneric(kDaikinHdrMark, kDaikinHdrSpace, kDaikinBitMark,
                  kDaikinOneSpace, kDaikinBitMark, kDaikinZeroSpace,
                  kDaikinBitMark, kDaikinZeroSpace + kDaikinGap,
                  kDaikinFirstHeader64, 64, 38, false, 0, 50);
    } else {  // We are using the newer/more correct size.
      sendGenericBytes(kDaikinHdrMark, kDaikinHdrSpace, kDaikinBitMark,
                  kDaikinOneSpace, kDaikinBitMark, kDaikinZeroSpace,
                  kDaikinBitMark, kDaikinZeroSpace + kDaikinGap,
                  data, kDaikinSection1Length, 38, false, 0, 50);
      offset += kDaikinSection1Length;
    } 
    // Data #2
    sendGenericBytes(kDaikinHdrMark, kDaikinHdrSpace, kDaikinBitMark,
                kDaikinOneSpace, kDaikinBitMark, kDaikinZeroSpace,
                kDaikinBitMark, kDaikinZeroSpace + kDaikinGap,
                data + offset, kDaikinSection2Length, 38, false, 0, 50);
    offset += kDaikinSection2Length;
    // Data #3
    sendGenericBytes(kDaikinHdrMark, kDaikinHdrSpace, kDaikinBitMark,
                kDaikinOneSpace, kDaikinBitMark, kDaikinZeroSpace,
                kDaikinBitMark, kDaikinZeroSpace + kDaikinGap,
                data + offset, nbytes - offset, 38, false, 0, 50);
  }
  vTaskExitCritical();
}

bool decodeDaikin(decode_results *results, uint16_t offset,
                          const uint16_t nbits, const bool strict) 
{
  // Is there enough data to match successfully?
  if (results->rawlen < (2 * (nbits + kDaikinHeaderLength) +
                         kDaikinSections * (kHeader + kFooter) + kFooter - 1) +
                         offset) 
    {
        // os_trace("%s Is there enough data to match successfully\r\n", __func__);
        return false;
    }

  // Compliance
  if (strict && nbits != kDaikinBits) {
    // os_trace("%s Compliance\r\n", __func__);
    return false;
  }

  match_result_t data_result;

  // Header #1 - Doesn't count as data.
  data_result = matchDataLimit(&(results->rawbuf[offset]), kDaikinHeaderLength,
                          kDaikinBitMark - 300, kDaikinBitMark + 300, kDaikinOneSpace - 300, kDaikinOneSpace + 500,
                          kDaikinBitMark - 300, kDaikinBitMark + 300, kDaikinZeroSpace - 300, kDaikinZeroSpace + 300,
                          kDaikinTolerance, kDaikinMarkExcess, false, true);
  offset += data_result.used;
  if (data_result.success == false) {
    os_trace("%s Fail\r\n", __func__);
    return false;  // Fail
  }
  if (data_result.data) {
    os_trace("%s The header bits should be zero.\r\n", __func__);
    return false;  // The header bits should be zero.
  }
  // Footer
  if (!matchLimit(results->rawbuf[offset++], kDaikinBitMark - 300, kDaikinBitMark + 300)) {
                    os_trace("%s Footer 1\r\n", __func__);
                    return false;
                 }
  if (!matchLimit(results->rawbuf[offset++], kDaikinZeroSpace + kDaikinGap - 7000, kDaikinZeroSpace + kDaikinGap + 7000 )) {
                    os_trace("%s Footer 2\r\n", __func__);
                    return false;
                  }
  // Sections
  uint8_t ksectionSize[] = {
      kDaikinSection1Length, kDaikinSection2Length, kDaikinSection3Length};
  uint16_t pos = 0;
  for (uint8_t section = 0; section < kDaikinSections; section++) {
    uint16_t used;
    // Section Header + Section Data (7 bytes) + Section Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, ksectionSize[section] * 8,
                        kDaikinHdrMark-400, kDaikinHdrMark+500, kDaikinHdrSpace-500, kDaikinHdrSpace+400,
                        kDaikinBitMark-300, kDaikinBitMark+300, kDaikinOneSpace-300, kDaikinOneSpace+300,
                        kDaikinBitMark-300, kDaikinBitMark+300, kDaikinZeroSpace-300, kDaikinZeroSpace+400,
                        kDaikinBitMark-300, kDaikinBitMark+300, kDaikinZeroSpace + kDaikinGap-7000, kDaikinZeroSpace + kDaikinGap+7000,
                        section >= kDaikinSections - 1,
                        45, kDaikinMarkExcess, false);
    if (used == 0) {
        os_trace("%s used == 0, section %u\r\n", __func__, section);
        return false;
    }
    offset += used;
    pos += ksectionSize[section];
  }
  // Compliance
  if (strict) {
    // Re-check we got the correct size/length due to the way we read the data.
    if (pos * 8 != kDaikinBits) return false;
    // Validate the checksum.
    if (!IRDaikin_validChecksum(results->state, kDaikinStateLength)) {
        os_trace("%s Validate checksum failed\r\n", __func__);
        return false;
    }
  }

  // Success
  results->decode_type = DAIKIN;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  os_trace("%s success\r\n", __func__);
  return true;
}

static void IRDaikinchecksum(DaikinProtocol *_) {
  _->Sum1 = sumBytesState(_->raw, kDaikinSection1Length - 1, 0);
  _->Sum2 = sumBytesState(_->raw + kDaikinSection1Length, kDaikinSection2Length - 1, 0);
  _->Sum3 = sumBytesState(_->raw + kDaikinSection1Length + kDaikinSection2Length,
                    kDaikinSection3Length - 1, 0);
}

static void IRDaikinstateReset(DaikinProtocol *_) {
  for (uint8_t i = 0; i < kDaikinStateLength; i++) _->raw[i] = 0x0;

  _->raw[0] = 0x11;
  _->raw[1] = 0xDA;
  _->raw[2] = 0x27;
  _->raw[4] = 0xC5;
  // _.raw[7] is a checksum byte, it will be set by checksum().
  _->raw[8] = 0x11;
  _->raw[9] = 0xDA;
  _->raw[10] = 0x27;
  _->raw[12] = 0x42;
  // _.raw[15] is a checksum byte, it will be set by checksum().
  _->raw[16] = 0x11;
  _->raw[17] = 0xDA;
  _->raw[18] = 0x27;
  _->raw[21] = 0x49;
  _->raw[22] = 0x1E;
  _->raw[24] = 0xB0;
  _->raw[27] = 0x06;
  _->raw[28] = 0x60;
  _->raw[31] = 0xC0;
  // _.raw[34] is a checksum byte, it will be set by checksum().
  IRDaikinchecksum(_);
}

static opmode_t DaikintoCommonMode(const uint8_t mode) {
  switch (mode) {
    case kDaikinCool: return opmode_kCool;
    case kDaikinHeat: return opmode_kHeat;
    case kDaikinDry: return opmode_kDry;
    case kDaikinFan: return opmode_kFan;
    default: return opmode_kAuto;
  }
}

fanspeed_t DaikintoCommonFanSpeed(const uint8_t speed) {

  switch (speed) {
    case kDaikinFanMax: return fanspeed_kMax;
    case kDaikinFanMax - 1: return fanspeed_kHigh;
    case kDaikinFanMed:
    case kDaikinFanMin + 1: return fanspeed_kMedium;
    case kDaikinFanMin: return fanspeed_kLow;
    case kDaikinFanQuiet: return fanspeed_kMin;
    default: return fanspeed_kAuto;
  }
}

static void Daikin_toCommon(DaikinProtocol _, state_ir_t *ir_state)
{
  ir_state->power = _.Power;
  ir_state->degrees = _.Temp / 2.0f;

  ir_state->mode = DaikintoCommonMode(_.Mode);

  uint8_t fan = _.Fan;
  if (fan != kDaikinFanQuiet && fan != kDaikinFanAuto) fan -= 2;
  ir_state->fanspeed = DaikintoCommonFanSpeed(fan);

  ir_state->quiet = _.Quiet;
  ir_state->turbo = _.Powerful;
  ir_state->clean = _.Mold;
  ir_state->econo = _.Econo;
  ir_state->swingv = _.SwingV ? swingv_kAuto : swingv_kOff;
  ir_state->swingh = _.SwingH ? swingh_kAuto : swingh_kOff;
}

static void DaikinsetTemp(DaikinProtocol *_, const float temp) {
  float degrees = max(temp, (float)(kDaikinMinTemp));
  degrees = min(degrees, (float)(kDaikinMaxTemp));
  _->Temp = degrees * 2.0f;
}

static void DaikinsetMode(DaikinProtocol *_, const uint8_t mode) {
  switch (mode) {
    case kDaikinAuto:
    case kDaikinCool:
    case kDaikinHeat:
    case kDaikinFan:
    case kDaikinDry:
      _->Mode = mode;
      break;
    default:
      _->Mode = kDaikinAuto;
  }
}

void DaikinsetFan(DaikinProtocol *_, const uint8_t fan) {
  // Set the fan speed bits, leave low 4 bits alone
  uint8_t fanset;
  if (fan == kDaikinFanQuiet || fan == kDaikinFanAuto)
    fanset = fan;
  else if (fan < kDaikinFanMin || fan > kDaikinFanMax)
    fanset = kDaikinFanAuto;
  else
    fanset = 2 + fan;
  _->Fan = fanset;
}

void Daikin_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  DaikinProtocol _;

  uint8_t offset = 0;
  if (length == kDaikinStateLengthShort) {  // Handle the "short" length case.
    offset = kDaikinStateLength - kDaikinStateLengthShort;
    IRDaikinstateReset(&_);
  }
  for (uint8_t i = 0; i < length && i < kDaikinStateLength; i++)
    _.raw[i + offset] = newState[i];

  Daikin_toCommon(_, ir_state);
}

void Daikin_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);
    DaikinProtocol _;
    IRDaikinstateReset(&_);
    DaikinsetTemp(&_, state_ir.degrees);
    DaikinsetMode(&_, state_ir.mode);
    DaikinsetFan(&_, state_ir.fanspeed);

    _.Power = state_ir.power;

    IRDaikinchecksum(&_);

    sendDaikin(_.raw, kDaikinStateLength, kNoRepeat);

    
}