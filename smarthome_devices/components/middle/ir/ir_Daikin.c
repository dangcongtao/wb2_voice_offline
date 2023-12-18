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

static void DaikinsetFan(DaikinProtocol *_, const uint8_t fan) {
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

static bool IRDaikin2validChecksum(uint8_t state[], const uint16_t length) {
  // Validate the checksum of section #1.
  if (length <= kDaikin2Section1Length - 1 ||
      state[kDaikin2Section1Length - 1] != sumBytesState(state,
                                                    kDaikin2Section1Length - 1, 0))
    return false;
  // Validate the checksum of section #2 (a.k.a. the rest)
  if (length <= kDaikin2Section1Length + 1 ||
      state[length - 1] != sumBytesState(state + kDaikin2Section1Length,
                                    length - kDaikin2Section1Length - 1, 0))
    return false;
  return true;
}

bool decodeDaikin2(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * (nbits + kHeader + kFooter) + kHeader - 1 + offset)
    return false;

  // Compliance
  if (strict && nbits != kDaikin2Bits) return false;

  uint8_t ksectionSize[] = {kDaikin2Section1Length,
                                                  kDaikin2Section2Length};

  // Leader
  if (!matchLimit(results->rawbuf[offset++], kDaikin2LeaderMark - 200, kDaikin2LeaderMark + 200)) return false;
  if (!matchLimit(results->rawbuf[offset++], kDaikin2LeaderSpace-500, kDaikin2LeaderSpace+500)) return false;

  // Sections
  uint16_t pos = 0;
  for (uint8_t section = 0; section < kDaikin2Sections; section++) {
    uint16_t used;
    // Section Header + Section Data + Section Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, ksectionSize[section] * 8,
                        kDaikin2HdrMark - 300, kDaikin2HdrMark + 300, kDaikin2HdrSpace - 300, kDaikin2HdrSpace + 300,
                        kDaikin2BitMark-360, kDaikin2BitMark+340, kDaikin2OneSpace-370, kDaikin2OneSpace + 430,
                        kDaikin2BitMark-360, kDaikin2BitMark+340, kDaikin2ZeroSpace -320, kDaikin2ZeroSpace+380,
                        kDaikin2BitMark-360, kDaikin2BitMark+340, kDaikin2Gap-4000, kDaikin2Gap+3000,
                        section >= kDaikin2Sections - 1,
                        40, kDaikinMarkExcess,
                        false);
    if (used == 0) {
      os_trace("%s matchGenericState failed %u\r\n", __func__, section);
      return false;
    }
    offset += used;
    pos += ksectionSize[section];
  }
  // Compliance
  if (strict) {
    // Re-check we got the correct size/length due to the way we read the data.
    if (pos * 8 != kDaikin2Bits) return false;
    // Validate the checksum.
    if (!IRDaikin2validChecksum(results->state, kDaikin2StateLength)) {
      os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }

  // Success
  results->decode_type = DAIKIN2;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  os_trace("%s success\r\n", __func__);
  return true;
}

void Daikin2_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin2Protocol _;
  memcpy(_.raw, newState, kDaikin2StateLength);

  ir_state->protocol = DAIKIN2;
  ir_state->power = (_.Power && !_.Power2);
  ir_state->mode = DaikintoCommonMode(_.Mode);
  ir_state->fanspeed = DaikintoCommonFanSpeed(_.Fan);
  ir_state->degrees = _.Temp;
  ir_state->quiet = _.Quiet;
  ir_state->light = _.Light != 3;
  ir_state->turbo = _.Powerful;
  ir_state->clean = _.Mold;
  ir_state->econo = _.Econo;
  ir_state->filter = _.Purify;
  ir_state->beep = (_.Beep != 3);
}

void sendDaikin2(const unsigned char data[], const uint16_t nbytes,
                         const uint16_t repeat) {
  if (nbytes < kDaikin2Section1Length)
    return;  // Not enough bytes to send a partial message.
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    // Leader
    sendGeneric(kDaikin2LeaderMark, kDaikin2LeaderSpace,
                0, 0, 0, 0, 0, 0, (uint64_t) 0,  // No data payload.
                0, kDaikin2Freq, false, 0, 50);
    // Section #1
    sendGenericBytes(kDaikin2HdrMark, kDaikin2HdrSpace, kDaikin2BitMark,
                kDaikin2OneSpace, kDaikin2BitMark, kDaikin2ZeroSpace,
                kDaikin2BitMark, kDaikin2Gap, data, kDaikin2Section1Length,
                kDaikin2Freq, false, 0, 50);
    // Section #2
    sendGenericBytes(kDaikin2HdrMark, kDaikin2HdrSpace, kDaikin2BitMark,
                kDaikin2OneSpace, kDaikin2BitMark, kDaikin2ZeroSpace,
                kDaikin2BitMark, kDaikin2Gap, data + kDaikin2Section1Length,
                nbytes - kDaikin2Section1Length,
                kDaikin2Freq, false, 0, 50);
  }
  vTaskExitCritical();
}


static void IRDaikin2clearSleepTimerFlag(Daikin2Protocol *_) { _->SleepTimer = false; }

static void IRDaikin2clearOnTimerFlag(Daikin2Protocol *_) { _->OnTimer = false; }

static void IRDaikin2disableOnTimer(Daikin2Protocol *_) {
  _->OnTime = kDaikinUnusedTime;
  IRDaikin2clearOnTimerFlag(_);
  IRDaikin2clearSleepTimerFlag(_);
}

static void IRDaikin2disableOffTimer(Daikin2Protocol *_) {
  _->OffTime = kDaikinUnusedTime;
  // Clear the Off Timer flag.
  _->OffTimer = false;
}

static void IRDaikin2checksum(Daikin2Protocol *_) {
  _->Sum1 = sumBytesState(_->raw, kDaikin2Section1Length - 1, 0);
  _->Sum2 = sumBytesState(_->raw + kDaikin2Section1Length, kDaikin2Section2Length - 1, 0);
}

static void IRDaikin2stateReset(Daikin2Protocol *_) {
  for (uint8_t i = 0; i < kDaikin2StateLength; i++) _->raw[i] = 0x0;

  _->raw[0] = 0x11;
  _->raw[1] = 0xDA;
  _->raw[2] = 0x27;
  _->raw[4] = 0x01;
  _->raw[6] = 0xC0;
  _->raw[7] = 0x70;
  _->raw[8] = 0x08;
  _->raw[9] = 0x0C;
  _->raw[10] = 0x80;
  _->raw[11] = 0x04;
  _->raw[12] = 0xB0;
  _->raw[13] = 0x16;
  _->raw[14] = 0x24;
  _->raw[17] = 0xBE;
  _->raw[18] = 0xD0;
  // _.raw[19] is a checksum byte, it will be set by checksum().
  _->raw[20] = 0x11;
  _->raw[21] = 0xDA;
  _->raw[22] = 0x27;
  _->raw[25] = 0x08;
  _->raw[28] = 0xA0;
  _->raw[35] = 0xC1;
  _->raw[36] = 0x80;
  _->raw[37] = 0x60;
  // _.raw[38] is a checksum byte, it will be set by checksum().
  IRDaikin2disableOnTimer(&_);
  IRDaikin2disableOffTimer(&_);
  IRDaikin2disableOnTimer(&_);
  IRDaikin2checksum(&_);
}

static void IRDaikin2setTemp(Daikin2Protocol *_, const uint8_t desired) {
  // The A/C has a different min temp if in cool mode.
  uint8_t temp = max(
      (_->Mode == kDaikinCool) ? kDaikin2MinCoolTemp : kDaikinMinTemp,
      desired);
  _->Temp = min(kDaikinMaxTemp, temp);
  // If the humidity setting is in use, the temp is a fixed value.
  if (_->HumidOn) _->Temp = kDaikinMaxTemp;
}

static void IRDaikin2setHumidity(Daikin2Protocol *_, const uint8_t percent) {
  _->Humidity = kDaikin2HumidityOff;  // Default to off.
  switch (_->Mode) {
    case kDaikinHeat:
      switch (percent) {
        case kDaikin2HumidityOff:
        case kDaikin2HumidityHeatLow:
        case kDaikin2HumidityHeatMedium:
        case kDaikin2HumidityHeatHigh:
        case kDaikin2HumidityAuto:
          _->Humidity = percent;
      }
      break;
    case kDaikinDry:
      switch (percent) {
        case kDaikin2HumidityOff:
        case kDaikin2HumidityDryLow:
        case kDaikin2HumidityDryMedium:
        case kDaikin2HumidityDryHigh:
        case kDaikin2HumidityAuto:
          _->Humidity = percent;
      }
      break;
  }
  _->HumidOn = (_->Humidity != kDaikin2HumidityOff);  // Enabled?
  IRDaikin2setTemp(_, _->Temp);  // Adjust the temperature if we need to.
}

static void IRDaikin2setMode(Daikin2Protocol *_, const uint8_t desired_mode) {
  uint8_t mode = desired_mode;
  switch (mode) {
    case kDaikinCool:
    case kDaikinHeat:
    case kDaikinFan:
    case kDaikinDry: break;
    default: mode = kDaikinAuto;
  }
  _->Mode = mode;
  // Redo the temp setting as Cool mode has a different min temp.
  if (mode == kDaikinCool) IRDaikin2setTemp(_, _->Temp);
  IRDaikin2setHumidity(_, _->Humidity);  // Make sure the humidity is okay for this mode.
}

void IRDaikin2setFan(Daikin2Protocol *_, const uint8_t fan) {
  uint8_t fanset;
  if (fan == kDaikinFanQuiet || fan == kDaikinFanAuto)
    fanset = fan;
  else if (fan < kDaikinFanMin || fan > kDaikinFanMax)
    fanset = kDaikinFanAuto;
  else
    fanset = 2 + fan;
  _->Fan = fanset;
}

void IRDaikin2setPower(Daikin2Protocol *_, const bool on) {
  _->Power = on;
  _->Power2 = !on;
}

void Daikin2_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);

    switch ((uint8_t)state_ir.mode)
    {
    case 10:
      state_ir.mode = kDaikinAuto;
      break;
    
    case 1:
      state_ir.mode = kDaikinDry;
      break;
    
    case 2:
      state_ir.mode = kDaikinCool;
      break;
    
    case 8:
      state_ir.mode = kDaikinHeat;
      break;
    
    default:
      break;
    }

    switch ((uint8_t)state_ir.fanspeed)
    {
    case 1:
      state_ir.fanspeed = kDaikinFanAuto;
      break;
    
    case 4:
      state_ir.fanspeed = kDaikinFanMin;
      break;
    
    case 3:
      state_ir.fanspeed = kDaikinFanMed;
      break;
    
    case 2:
      state_ir.fanspeed = kDaikinFanMax;
      break;
    
    default:
      break;
    }

    Daikin2Protocol _;
    IRDaikin2stateReset(&_);

    IRDaikin2setMode(&_, state_ir.mode);
    IRDaikin2setFan(&_, state_ir.fanspeed);
    IRDaikin2setPower(&_, state_ir.power);
    IRDaikin2setTemp(&_, state_ir.degrees);

    IRDaikin2checksum(&_);

    sendDaikin2(_.raw, kDaikin2StateLength, kDaikin2DefaultRepeat); 
}