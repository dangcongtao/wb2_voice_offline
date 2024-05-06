#include "ir_Daikin.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------Daikin----------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
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
    // os_trace("%s Fail\r\n", __func__);
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

void Daikin_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  DaikinProtocol _;

  uint8_t offset = 0;
  if (length == kDaikinStateLengthShort) {  // Handle the "short" length case.
    offset = kDaikinStateLength - kDaikinStateLengthShort;
    IRDaikinstateReset(&_);
  }
  for (uint8_t i = 0; i < length && i < kDaikinStateLength; i++)
    _.raw[i + offset] = newState[i];

  ir_state->power = _.Power;
  ir_state->degrees = _.Temp / 2.0f;

  ir_state->mode = _.Mode;

  uint8_t fan = _.Fan;
  if (fan != kDaikinFanQuiet && fan != kDaikinFanAuto) fan -= 2;
  ir_state->fanspeed = fan;

  ir_state->quiet = _.Quiet;
  ir_state->turbo = _.Powerful;
  ir_state->clean = _.Mold;
  ir_state->econo = _.Econo;
  ir_state->swingv = _.SwingV;
  ir_state->swingh = _.SwingH;
}

void Daikin_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);
    DaikinProtocol _;
    IRDaikinstateReset(&_);
    _.SwingV = kDaikinSwingOn;
    _.SwingH = kDaikinSwingOn;
    DaikinsetTemp(&_, state_ir.degrees);
    DaikinsetMode(&_, state_ir.mode);
    DaikinsetFan(&_, state_ir.fanspeed);

    _.Power = state_ir.power;

    IRDaikinchecksum(&_);

    sendDaikin(_.raw, kDaikinStateLength, kNoRepeat); 
}

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------Daikin2---------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/

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
                        kDaikin2BitMark-360, kDaikin2BitMark+340, kDaikin2Gap-8750, kDaikin2Gap+8750,
                        section >= kDaikin2Sections - 1,
                        40, kDaikinMarkExcess,
                        false);
    if (used == 0) {
      // os_trace("%s matchGenericState failed %u\r\n", __func__, section);
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

void Daikin2_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin2Protocol _;
  memcpy(_.raw, newState, kDaikin2StateLength);

  ir_state->protocol = DAIKIN2;
  ir_state->power = (_.Power && !_.Power2);
  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;
  ir_state->degrees = _.Temp;
  ir_state->quiet = _.Quiet;
  ir_state->light = _.Light != 3;
  ir_state->turbo = _.Powerful;
  ir_state->clean = _.Mold;
  ir_state->econo = _.Econo;
  ir_state->filter = _.Purify;
  ir_state->swingv = _.SwingV;
  ir_state->swingh = _.SwingH;

  ir_state->beep = (_.Beep != 3);

  switch ((uint8_t)ir_state->mode)
    {
    case kDaikinAuto:
      ir_state->mode = 10;
      break;
    
    case kDaikinDry:
      ir_state->mode = 1;
      break;
    
    case kDaikinCool:
      ir_state->mode = 2;
      break;
    
    case kDaikinFan:
      ir_state->mode = 4;
      break;
    
    case kDaikinHeat:
      ir_state->mode = 8;
      break;
    
    default:
      break;
    }

    switch ((uint8_t)ir_state->fanspeed)
    {
    case kDaikinFanAuto:
      ir_state->fanspeed = 1;
      break;
    
    case kDaikinFanMin:
      ir_state->fanspeed = 4;
      break;
    
    case kDaikinFanMed:
      ir_state->fanspeed = 3;
      break;
    
    case kDaikinFanMax:
      ir_state->fanspeed = 2;
      break;
    
    default:
      break;
    }
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
    
    case 4:
      state_ir.mode = kDaikinFan;
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

    _.SwingV = kDaikin2SwingVAuto;
    _.SwingH = kDaikin2SwingHAuto;

    IRDaikin2setMode(&_, state_ir.mode);
    IRDaikin2setFan(&_, state_ir.fanspeed);
    IRDaikin2setPower(&_, state_ir.power);
    IRDaikin2setTemp(&_, state_ir.degrees);

    IRDaikin2checksum(&_);

    sendDaikin2(_.raw, kDaikin2StateLength, kDaikin2DefaultRepeat); 
}

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------Daikin152-------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static bool IRDaikin152validChecksum(uint8_t state[], const uint16_t length) {
  // Validate the checksum of the given state.
  if (length <= 1 || state[length - 1] != sumBytesState(state, length - 1, 0))
    return false;
  else
    return true;
}

bool decodeDaikin152(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * (5 + nbits + kFooter) + kHeader - 1 + offset)
    return false;
  if (nbits / 8 < kDaikin152StateLength) return false;

  // Compliance
  if (strict && nbits != kDaikin152Bits) return false;

  uint16_t used;

  // Leader
  uint64_t leader = 0;
  used = matchGenericDataLimit(results->rawbuf + offset, &leader,
                      results->rawlen - offset, kDaikin152LeaderBits,
                      0, 0, 0, 0,  // No Header
                     kDaikin152BitMark-363, kDaikin152BitMark+367, kDaikin152OneSpace-500, kDaikin152OneSpace+400,
                      kDaikin152BitMark-363, kDaikin152BitMark+367, kDaikin152ZeroSpace-363, kDaikin152ZeroSpace+367,
                      kDaikin152BitMark-363, kDaikin152BitMark+367, kDaikin152Gap - 6250, kDaikin152Gap + 6250,  // Footer gap
                      false, 45, kMarkExcess, false);
  if (used == 0 || leader != 0) {
    // os_trace("%s matchGenericData failed\r\n", __func__);
    return false;
  }
  offset += used;

  // Header + Data + Footer
  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, nbits,
                      kDaikin152HdrMark-300, kDaikin152HdrMark+300, kDaikin152HdrSpace-300, kDaikin152HdrSpace+300,
                      kDaikin152BitMark-363, kDaikin152BitMark+367, kDaikin152OneSpace-500, kDaikin152OneSpace+400,
                      kDaikin152BitMark-363, kDaikin152BitMark+367, kDaikin152ZeroSpace-363, kDaikin152ZeroSpace+367,
                      kDaikin152BitMark-363, kDaikin152BitMark+367, kDaikin152Gap - 6500, kDaikin152Gap + 6500,
                      true, 45, kMarkExcess, false);
  if (used == 0) {
    // os_trace("%s matchGenericState failed\r\n", __func__);
    return false;
  }

  // Compliance
  if (strict) {
    if (!IRDaikin152validChecksum(results->state, kDaikin152StateLength)) {
      os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }

  // Success
  results->decode_type = DAIKIN152;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  os_trace("%s success\r\n", __func__);
  return true;
}

static uint8_t IRDaikin152getFan(Daikin152Protocol _)
{
  const uint8_t fan = _.Fan;
  switch (fan) {
    case kDaikinFanAuto:
    case kDaikinFanQuiet: return fan;
    default: return fan - 2;
  }
}

void Daikin152_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin152Protocol _;
  memcpy(_.raw, newState, kDaikin152StateLength);

  ir_state->protocol = DAIKIN152;
  ir_state->power = _.Power;

  ir_state->mode = DaikintoCommonMode(_.Mode);
  ir_state->fanspeed = DaikintoCommonFanSpeed(IRDaikin152getFan(_));

  ir_state->swingv = _.SwingV ? swingv_kAuto : swingv_kOff;

  ir_state->degrees = _.Temp;
  ir_state->quiet = _.Quiet;
  ir_state->turbo = _.Powerful;
  ir_state->econo = _.Econo;
}

void Daikin152_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin152Protocol _;
  memcpy(_.raw, newState, kDaikin152StateLength);

  ir_state->protocol = DAIKIN152;
  ir_state->power = _.Power;

  ir_state->mode = _.Mode;
  ir_state->fanspeed = IRDaikin152getFan(_);

  ir_state->swingv = _.SwingV;

  ir_state->degrees = _.Temp;
  ir_state->quiet = _.Quiet;
  ir_state->turbo = _.Powerful;
  ir_state->econo = _.Econo;

  switch ((uint8_t)ir_state->mode)
    {
    case kDaikinAuto:
      ir_state->mode = 10;
      break;
    
    case kDaikinDry:
      ir_state->mode = 1;
      break;
    
    case kDaikinCool:
      ir_state->mode = 2;
      break;
    
    case kDaikinFan:
      ir_state->mode = 4;
      break;
    
    case kDaikinHeat:
      ir_state->mode = 8;
      break;
    
    default:
      break;
    }

    switch ((uint8_t)ir_state->fanspeed)
    {
    case kDaikinFanAuto:
      ir_state->fanspeed = 1;
      break;
    
    case kDaikinFanMin:
      ir_state->fanspeed = 4;
      break;
    
    case kDaikinFanMed:
      ir_state->fanspeed = 3;
      break;
    
    case kDaikinFanMax:
      ir_state->fanspeed = 2;
      break;
    
    default:
      break;
    }
}

static void IRDaikin152stateReset(Daikin152Protocol *_) {
  for (uint8_t i = 3; i < kDaikin152StateLength; i++) _->raw[i] = 0x00;
  _->raw[0] =  0x11;
  _->raw[1] =  0xDA;
  _->raw[2] =  0x27;
  _->raw[15] = 0xC5;
  // _.raw[19] is a checksum byte, it will be set by checksum().
}

static void IRDaikin152setTemp(Daikin152Protocol *_, const uint8_t temp) {
  uint8_t degrees = max(
      temp, (_->Mode == kDaikinHeat) ? kDaikinMinTemp : kDaikin2MinCoolTemp);
  degrees = min(degrees, kDaikinMaxTemp);
  if (temp == kDaikin152FanTemp) degrees = temp;  // Handle fan only temp.
  _->Temp = degrees;
}

static void IRDaikin152setMode(Daikin152Protocol *_, const uint8_t mode) {
  switch (mode) {
    case kDaikinFan:
      IRDaikin152setTemp(_, kDaikin152FanTemp);  // Handle special temp for fan mode.
      break;
    case kDaikinDry:
      IRDaikin152setTemp(_, kDaikin152DryTemp);  // Handle special temp for dry mode.
      break;
    case kDaikinAuto:
    case kDaikinCool:
    case kDaikinHeat:
      break;
    default:
      _->Mode = kDaikinAuto;
      return;
  }
  _->Mode = mode;
}

static void IRDaikin152setFan(Daikin152Protocol *_, const uint8_t fan) {
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

static void sendDaikin152(const unsigned char data[], const uint16_t nbytes,
                           const uint16_t repeat) {
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    // Leader
    sendGeneric(0, 0, kDaikin152BitMark, kDaikin152OneSpace,
                kDaikin152BitMark, kDaikin152ZeroSpace,
                kDaikin152BitMark, kDaikin152Gap,
                (uint64_t)0, kDaikin152LeaderBits,
                kDaikin152Freq, false, 0, kDutyDefault);
    // Header + Data + Footer
    sendGenericBytes(kDaikin152HdrMark, kDaikin152HdrSpace, kDaikin152BitMark,
                kDaikin152OneSpace, kDaikin152BitMark, kDaikin152ZeroSpace,
                kDaikin152BitMark, kDaikin152Gap, data,
                nbytes, kDaikin152Freq, false, 0, kDutyDefault);
  }
  vTaskExitCritical();
}

void Daikin152_send(state_ir_t state_ir)
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
    
    case 4:
      state_ir.mode = kDaikinFan;
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

    Daikin152Protocol _;
    IRDaikin152stateReset(&_);

    IRDaikin152setMode(&_, state_ir.mode);
    IRDaikin152setFan(&_, state_ir.fanspeed);
    // IRDaikin2setPower(&_, state_ir.power);
    _.Power = state_ir.power;
    IRDaikin152setTemp(&_, state_ir.degrees);

    _.SwingV = kDaikinSwingOn;

    _.Sum = sumBytesState(_.raw, kDaikin152StateLength - 1, 0);

    sendDaikin152(_.raw, kDaikin152StateLength, kDaikin152DefaultRepeat); 
}

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------Daikin160-------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/

static bool IRDaikin160validChecksum(uint8_t state[], const uint16_t length) {
  // Validate the checksum of section #1.
  if (length <= kDaikin160Section1Length - 1 ||
      state[kDaikin160Section1Length - 1] != sumBytesState(
          state, kDaikin160Section1Length - 1, 0))
    return false;
  // Validate the checksum of section #2 (a.k.a. the rest)
  if (length <= kDaikin160Section1Length + 1 ||
      state[length - 1] != sumBytesState(state + kDaikin160Section1Length,
                                    length - kDaikin160Section1Length - 1, 0))
    return false;
  return true;
}

bool decodeDaikin160(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * (nbits + kHeader + kFooter) - 1 + offset)
    return false;

  // Compliance
  if (strict && nbits != kDaikin160Bits) return false;

  uint8_t ksectionSize[] = {kDaikin160Section1Length,
                                                    kDaikin160Section2Length};

  // Sections
  uint16_t pos = 0;
  for (uint8_t section = 0; section < kDaikin160Sections; section++) {
    uint16_t used;
    // Section Header + Section Data (7 bytes) + Section Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, ksectionSize[section] * 8,
                        kDaikin160HdrMark-1000,kDaikin160HdrMark+1000, kDaikin160HdrSpace-300, kDaikin160HdrSpace+300,
                        kDaikin160BitMark-262, kDaikin160BitMark+458, kDaikin160OneSpace-500, kDaikin160OneSpace+400,
                        kDaikin160BitMark-262, kDaikin160BitMark+458, kDaikin160ZeroSpace-300, kDaikin160ZeroSpace+400,
                        kDaikin160BitMark-262, kDaikin160BitMark+458, kDaikin160Gap-7500, kDaikin160Gap+7500,
                        section >= kDaikin160Sections - 1,
                        45, kDaikinMarkExcess, false);
    if (used == 0) {
      // os_trace("%s matchGenericState failed\r\n", __func__);
      return false;
    }
    offset += used;
    pos += ksectionSize[section];
  }
  // Compliance
  if (strict) {
    // Validate the checksum.
    if (!IRDaikin160validChecksum(results->state, kDaikin160StateLength)) {
      os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }

  // Success
  results->decode_type = DAIKIN160;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  os_trace("%s success\r\n", __func__);
  return true;
}

static uint8_t IRDaikin160getFan(Daikin160Protocol _)
{
  uint8_t fan = _.Fan;
  if (fan != kDaikinFanQuiet && fan != kDaikinFanAuto) fan -= 2;
  return fan;
}

swingv_t IRDaikin160toCommonSwingV(const uint8_t setting) {
  switch (setting) {
    case kDaikin160SwingVHighest: return swingv_kHighest;
    case kDaikin160SwingVHigh:    return swingv_kHigh;
    case kDaikin160SwingVMiddle:  return swingv_kMiddle;
    case kDaikin160SwingVLow:     return swingv_kLow;
    case kDaikin160SwingVLowest:  return swingv_kLowest;
    default:
      return swingv_kAuto;
  }
}

void Daikin160_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin160Protocol _;
  memcpy(_.raw, newState, kDaikin160StateLength);

  ir_state->protocol = DAIKIN160;
  ir_state->power = _.Power;

  ir_state->mode = DaikintoCommonMode(_.Mode);
  ir_state->fanspeed = DaikintoCommonFanSpeed(IRDaikin160getFan(_));

  ir_state->swingv = IRDaikin160toCommonSwingV(_.SwingV);

  ir_state->degrees = _.Temp + 10;

}

void Daikin160_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin160Protocol _;
  memcpy(_.raw, newState, kDaikin160StateLength);

  ir_state->protocol = DAIKIN160;
  ir_state->power = _.Power;

  ir_state->mode = _.Mode;
  ir_state->fanspeed = IRDaikin160getFan(_);

  ir_state->swingv = IRDaikin160toCommonSwingV(_.SwingV);

  ir_state->degrees = _.Temp + 10;

  switch ((uint8_t)ir_state->mode)
    {
    case kDaikinAuto:
      ir_state->mode = 10;
      break;
    
    case kDaikinDry:
      ir_state->mode = 1;
      break;
    
    case kDaikinCool:
      ir_state->mode = 2;
      break;
    
    case kDaikinFan:
      ir_state->mode = 4;
      break;
    
    case kDaikinHeat:
      ir_state->mode = 8;
      break;
    
    default:
      ir_state->mode = 10;
      break;
    }

    switch ((uint8_t)ir_state->fanspeed)
    {
    case kDaikinFanAuto:
      ir_state->fanspeed = 1;
      break;
    
    case kDaikinFanMin:
      ir_state->fanspeed = 4;
      break;
    
    case kDaikinFanMed:
      ir_state->fanspeed = 3;
      break;
    
    case kDaikinFanMax:
      ir_state->fanspeed = 2;
      break;
    
    default:
      ir_state->fanspeed = 1;
      break;
    }

  ir_state->swingv = _.SwingV;
}

static void sendDaikin160(const unsigned char data[], const uint16_t nbytes,
                           const uint16_t repeat) {
  if (nbytes < kDaikin160Section1Length)
    return;  // Not enough bytes to send a partial message.
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    // Section #1
    sendGenericBytes(kDaikin160HdrMark, kDaikin160HdrSpace, kDaikin160BitMark,
                kDaikin160OneSpace, kDaikin160BitMark, kDaikin160ZeroSpace,
                kDaikin160BitMark, kDaikin160Gap, data,
                kDaikin160Section1Length,
                kDaikin160Freq, false, 0, kDutyDefault);
    // Section #2
    sendGenericBytes(kDaikin160HdrMark, kDaikin160HdrSpace, kDaikin160BitMark,
                kDaikin160OneSpace, kDaikin160BitMark, kDaikin160ZeroSpace,
                kDaikin160BitMark, kDaikin160Gap,
                data + kDaikin160Section1Length,
                nbytes - kDaikin160Section1Length,
                kDaikin160Freq, false, 0, kDutyDefault);
  }
  vTaskExitCritical();
}

static void IRDaikin160stateReset(Daikin160Protocol *_) {
  for (uint8_t i = 0; i < kDaikin160StateLength; i++) _->raw[i] = 0x00;
  _->raw[0] =  0x11;
  _->raw[1] =  0xDA;
  _->raw[2] =  0x27;
  _->raw[3] =  0xF0;
  _->raw[4] =  0x0D;
  // _.raw[6] is a checksum byte, it will be set by checksum().
  _->raw[7] =  0x11;
  _->raw[8] =  0xDA;
  _->raw[9] =  0x27;
  _->raw[11] = 0xD3;
  _->raw[12] = 0x30;
  _->raw[13] = 0x11;
  _->raw[16] = 0x1E;
  _->raw[17] = 0x0A;
  _->raw[18] = 0x08;
  // _.raw[19] is a checksum byte, it will be set by checksum().
}

static void IRDaikin160setMode(Daikin160Protocol *_, const uint8_t mode) {
  switch (mode) {
    case kDaikinAuto:
    case kDaikinCool:
    case kDaikinHeat:
    case kDaikinFan:
    case kDaikinDry:
      _->Mode = mode;
      break;
    default: _->Mode = kDaikinAuto;
  }
}

static void IRDaikin160setFan(Daikin160Protocol *_, const uint8_t fan) {
  uint8_t fanset;
  if (fan == kDaikinFanQuiet || fan == kDaikinFanAuto)
    fanset = fan;
  else if (fan < kDaikinFanMin || fan > kDaikinFanMax)
    fanset = kDaikinFanAuto;
  else
    fanset = 2 + fan;
  _->Fan = fanset;
}

static void IRDaikin160setTemp(Daikin160Protocol *_, const uint8_t temp) {
  uint8_t degrees = max(temp, kDaikinMinTemp);
  degrees = min(degrees, kDaikinMaxTemp) - 10;
  _->Temp = degrees;
}

void Daikin160_send(state_ir_t state_ir)
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
    
    case 4:
      state_ir.mode = kDaikinFan;
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

    Daikin160Protocol _;
    IRDaikin160stateReset(&_);

    _.SwingV = kDaikin160SwingVAuto;

    IRDaikin160setMode(&_, state_ir.mode);
    IRDaikin160setFan(&_, state_ir.fanspeed);
    _.Power = state_ir.power;
    IRDaikin160setTemp(&_, state_ir.degrees);

    _.Sum1 = sumBytesState(_.raw, kDaikin160Section1Length - 1, 0);
    _.Sum2 = sumBytesState(_.raw + kDaikin160Section1Length,
                    kDaikin160Section2Length - 1, 0);

    sendDaikin160(_.raw, kDaikin160StateLength, kDaikin160DefaultRepeat); 
}


/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------Daikin216-------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/

bool IRDaikin216validChecksum(uint8_t state[], const uint16_t length) {
  // Validate the checksum of section #1.
  if (length <= kDaikin216Section1Length - 1 ||
      state[kDaikin216Section1Length - 1] != sumBytesState(
          state, kDaikin216Section1Length - 1, 0))
    return false;
  // Validate the checksum of section #2 (a.k.a. the rest)
  if (length <= kDaikin216Section1Length + 1 ||
      state[length - 1] != sumBytesState(state + kDaikin216Section1Length,
                                    length - kDaikin216Section1Length - 1, 0))
    return false;
  return true;
}

bool decodeDaikin216(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * (nbits + kHeader + kFooter) - 1 + offset)
    return false;

  // Compliance
  if (strict && nbits != kDaikin216Bits) return false;

  uint8_t ksectionSize[] = {kDaikin216Section1Length,
                                                    kDaikin216Section2Length};
  // Sections
  uint16_t pos = 0;
  for (uint8_t section = 0; section < kDaikin216Sections; section++) {
    uint16_t used;
    // Section Header + Section Data + Section Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, ksectionSize[section] * 8,
                        kDaikin216HdrMark-400,kDaikin216HdrMark+400, kDaikin216HdrSpace-300,kDaikin216HdrSpace+400,
                        kDaikin216BitMark-340,kDaikin216BitMark+380, kDaikin216OneSpace-400,kDaikin216OneSpace+400,
                        kDaikin216BitMark-340,kDaikin216BitMark+380, kDaikin216ZeroSpace-370,kDaikin216ZeroSpace+400,
                        kDaikin216BitMark-340,kDaikin216BitMark+380, kDaikin216Gap-7500,kDaikin216Gap+7500,
                        section >= kDaikin216Sections - 1,
                        45, kDaikinMarkExcess, false);
    if (used == 0) {
      // os_trace("%s matchGenericState failed\r\n", __func__);
      return false;
    }
    offset += used;
    pos += ksectionSize[section];
  }
  // Compliance
  if (strict) {
    if (pos * 8 != kDaikin216Bits) return false;
    // Validate the checksum.
    if (!IRDaikin216validChecksum(results->state, kDaikin216StateLength)) {
      os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }

  // Success
  results->decode_type = DAIKIN216;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  os_trace("%s success\r\n", __func__);
  return true;
}

uint8_t IRDaikin216getFan(Daikin216Protocol _)
{
  uint8_t fan = _.Fan;
  if (fan != kDaikinFanQuiet && fan != kDaikinFanAuto) fan -= 2;
  return fan;
}

void Daikin216_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin216Protocol _;
  memcpy(_.raw, newState, kDaikin216StateLength);

  ir_state->protocol = DAIKIN216;
  ir_state->power = _.Power;

  ir_state->mode = DaikintoCommonMode(_.Mode);
  ir_state->fanspeed = DaikintoCommonFanSpeed(IRDaikin216getFan(_));

  ir_state->degrees = _.Temp;

  ir_state->swingv = _.SwingV ? swingv_kAuto : swingv_kOff;
  ir_state->swingh = _.SwingH ? swingh_kAuto : swingh_kOff;

  ir_state->turbo = _.Powerful;

}

void Daikin216_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin216Protocol _;
  memcpy(_.raw, newState, kDaikin216StateLength);

  ir_state->protocol = DAIKIN216;
  ir_state->power = _.Power;

  ir_state->mode = _.Mode;
  ir_state->fanspeed = IRDaikin216getFan(_);

  ir_state->degrees = _.Temp;

  ir_state->swingv = _.SwingV ? swingv_kAuto : swingv_kOff;
  ir_state->swingh = _.SwingH ? swingh_kAuto : swingh_kOff;

  ir_state->turbo = _.Powerful;

  switch ((uint8_t)ir_state->mode)
    {
    case kDaikinAuto:
      ir_state->mode = 10;
      break;
    
    case kDaikinDry:
      ir_state->mode = 1;
      break;
    
    case kDaikinCool:
      ir_state->mode = 2;
      break;
    
    case kDaikinFan:
      ir_state->mode = 4;
      break;
    
    case kDaikinHeat:
      ir_state->mode = 8;
      break;
    
    default:
      break;
    }

    switch ((uint8_t)ir_state->fanspeed)
    {
    case kDaikinFanAuto:
      ir_state->fanspeed = 1;
      break;
    
    case kDaikinFanMin:
      ir_state->fanspeed = 4;
      break;
    
    case kDaikinFanMed:
      ir_state->fanspeed = 3;
      break;
    
    case kDaikinFanMax:
      ir_state->fanspeed = 2;
      break;
    
    default:
      break;
    }

}

static void sendDaikin216(const unsigned char data[], const uint16_t nbytes,
                           const uint16_t repeat) {
  if (nbytes < kDaikin216Section1Length)
    return;  // Not enough bytes to send a partial message.
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    // Section #1
    sendGenericBytes(kDaikin216HdrMark, kDaikin216HdrSpace, kDaikin216BitMark,
                kDaikin216OneSpace, kDaikin216BitMark, kDaikin216ZeroSpace,
                kDaikin216BitMark, kDaikin216Gap, data,
                kDaikin216Section1Length,
                kDaikin216Freq, false, 0, kDutyDefault);
    // Section #2
    sendGenericBytes(kDaikin216HdrMark, kDaikin216HdrSpace, kDaikin216BitMark,
                kDaikin216OneSpace, kDaikin216BitMark, kDaikin216ZeroSpace,
                kDaikin216BitMark, kDaikin216Gap,
                data + kDaikin216Section1Length,
                nbytes - kDaikin216Section1Length,
                kDaikin216Freq, false, 0, kDutyDefault);
  }
  vTaskExitCritical();
}

static void IRDaikin216stateReset(Daikin216Protocol *_) {
  for (uint8_t i = 0; i < kDaikin216StateLength; i++) _->raw[i] = 0x00;
  _->raw[0] =  0x11;
  _->raw[1] =  0xDA;
  _->raw[2] =  0x27;
  _->raw[3] =  0xF0;
  // _.raw[7] is a checksum byte, it will be set by checksum().
  _->raw[8] =  0x11;
  _->raw[9] =  0xDA;
  _->raw[10] = 0x27;
  _->raw[23] = 0xC0;
  // _.raw[26] is a checksum byte, it will be set by checksum().
}

static void IRDaikin216setMode(Daikin216Protocol *_, const uint8_t mode) {
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

static void IRDaikin216setFan(Daikin216Protocol *_, const uint8_t fan) {
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

static void IRDaikin216setTemp(Daikin216Protocol *_, const uint8_t temp) {
  uint8_t degrees = max(temp, kDaikinMinTemp);
  degrees = min(degrees, kDaikinMaxTemp);
  _->Temp = degrees;
}

void Daikin216_send(state_ir_t state_ir)
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
    
    case 4:
      state_ir.mode = kDaikinFan;
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

    Daikin216Protocol _;
    IRDaikin216stateReset(&_);

    _.SwingV = kDaikin216SwingOn;
    _.SwingH = kDaikin216SwingOn;

    _.Power = state_ir.power;
    IRDaikin216setMode(&_, state_ir.mode);
    IRDaikin216setFan(&_, state_ir.fanspeed);
    IRDaikin216setTemp(&_, state_ir.degrees);

    _.Sum1 = sumBytesState(_.raw, kDaikin216Section1Length - 1, 0);
    _.Sum2 = sumBytesState(_.raw + kDaikin216Section1Length,
                    kDaikin216Section2Length - 1, 0);

    state_ir_t my_state_ir;
    Daikin216_setRaw(_.raw, kDaikin216StateLength, DAIKIN216, &my_state_ir);
    IR_logCommon(my_state_ir);

    sendDaikin216(_.raw, kDaikin216StateLength, kDaikin216DefaultRepeat); 
}


/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------Daikin128-------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/

static uint8_t IRDaikin128calcFirstChecksum(const uint8_t state[]) {
  return sumNibblesByte(state, kDaikin128SectionLength - 1,
                    state[kDaikin128SectionLength - 1] & 0x0F) & 0x0F;
}

static uint8_t IRDaikin128calcSecondChecksum(const uint8_t state[]) {
  return sumNibblesByte(state + kDaikin128SectionLength,
                    kDaikin128SectionLength - 1, 0);
}

static bool IRDaikin128validChecksum(uint8_t state[]) {
  // Validate the checksum of section #1.
  if (state[kDaikin128SectionLength - 1] >> 4 != IRDaikin128calcFirstChecksum(state))
    return false;
  // Validate the checksum of section #2
  if (state[kDaikin128StateLength - 1] != IRDaikin128calcSecondChecksum(state))
    return false;
  return true;
}

bool decodeDaikin128(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * (nbits + kHeader) + kFooter - 1 + offset)
    return false;
  if (nbits / 8 <= kDaikin128SectionLength) return false;

  // Compliance
  if (strict && nbits != kDaikin128Bits) return false;

  // Leader
  for (uint8_t i = 0; i < 2; i++) {
    if (!matchLimit(results->rawbuf[offset++], kDaikin128LeaderMark-400, kDaikin128LeaderMark+400)) return false;
    if (!matchLimit(results->rawbuf[offset++], kDaikin128LeaderSpace-400, kDaikin128LeaderSpace+400)) return false;
  }
  uint16_t ksectionSize[] = {
      kDaikin128SectionLength, (uint16_t)(nbits / 8 - kDaikin128SectionLength)};
  // Data Sections
  uint16_t pos = 0;
  for (uint8_t section = 0; section < kDaikin128Sections; section++) {
    uint16_t used;
    // Section Header (first section only) + Section Data (8 bytes) +
    //     Section Footer (Not for first section)
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, ksectionSize[section] * 8,
                        section == 0 ? kDaikin128HdrMark - 500 : 0, section == 0 ? kDaikin128HdrMark + 400 : 0,
                        section == 0 ? kDaikin128HdrSpace - 400 : 0, section == 0 ? kDaikin128HdrSpace + 400 : 0,
                        kDaikin128BitMark - 270, kDaikin128BitMark + 250, kDaikin128OneSpace-254, kDaikin128OneSpace + 246,
                        kDaikin128BitMark - 270, kDaikin128BitMark + 250, kDaikin128ZeroSpace - 302, kDaikin128ZeroSpace + 318,
                        section > 0 ? kDaikin128FooterMark - 500 : kDaikin128BitMark - 270, section > 0 ? kDaikin128FooterMark + 1150 : kDaikin128BitMark + 1150,
                        kDaikin128Gap - 5000, kDaikin128Gap + 5000,
                        section > 0,
                        kDaikinTolerance, kDaikinMarkExcess, false);
    if (used == 0) {
      // os_trace("%s matchGenericState failed\r\n", __func__);
      return false;
    }
    offset += used;
    pos += ksectionSize[section];
  }
  // Compliance
  if (strict) {
    if (!IRDaikin128validChecksum(results->state)) {
      os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }

  // Success
  results->decode_type = DAIKIN128;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  os_trace("%s success\r\n", __func__);
  return true;
}

static opmode_t IRDaikin128toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kDaikin128Cool: return opmode_kCool;
    case kDaikin128Heat: return opmode_kHeat;
    case kDaikin128Dry: return opmode_kDry;
    case kDaikin128Fan: return opmode_kFan;
    default: return opmode_kAuto;
  }
}

static fanspeed_t IRDaikin128toCommonFanSpeed(const uint8_t speed) {
  switch (speed) {
    case kDaikin128FanPowerful: return fanspeed_kMax;
    case kDaikin128FanHigh: return fanspeed_kHigh;
    case kDaikin128FanMed: return fanspeed_kMedium;
    case kDaikin128FanLow: return fanspeed_kLow;
    case kDaikinFanQuiet: return fanspeed_kMin;
    default: return fanspeed_kAuto;
  }
}

static uint8_t IRDaikin128getTemp(Daikin128Protocol _){ return bcdToUint8(_.Temp); }

static bool daikin128_prev_power = true;
void Daikin128_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin128Protocol _;
  memcpy(_.raw, newState, kDaikin128StateLength);

  ir_state->protocol = DAIKIN128;
  if (_.Power) {
    daikin128_prev_power = !daikin128_prev_power;
  }
  ir_state->power ^= daikin128_prev_power;

  ir_state->mode = IRDaikin128toCommonMode(_.Mode);
  ir_state->fanspeed = IRDaikin128toCommonFanSpeed(_.Fan);

  ir_state->degrees = IRDaikin128getTemp(_);

  ir_state->swingv = _.SwingV ? swingv_kAuto : swingv_kOff;

}

void Daikin128_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
  
  Daikin128Protocol _;
  memcpy(_.raw, newState, kDaikin128StateLength);

  ir_state->protocol = DAIKIN128;
  if (_.Power) {
    daikin128_prev_power = !daikin128_prev_power;
  }
  ir_state->power ^= daikin128_prev_power;

  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;

  ir_state->degrees = IRDaikin128getTemp(_);

  ir_state->swingv = _.SwingV ? swingv_kAuto : swingv_kOff;

}

static void IRDaikin128stateReset(Daikin128Protocol *_) {
  for (uint8_t i = 0; i < kDaikin128StateLength; i++) _->raw[i] = 0x00;
  _->raw[0] = 0x16;
  _->raw[7] = 0x04;  // Most significant nibble is a checksum.
  _->raw[8] = 0xA1;
  // _.raw[15] is a checksum byte, it will be set by checksum().
}

static void IRDaikin128setFan(Daikin128Protocol *_, const uint8_t speed) {
  uint8_t new_speed = speed;
  uint8_t mode = _->Mode;
  switch (speed) {
    case kDaikin128FanQuiet:
    case kDaikin128FanPowerful:
      if (mode == kDaikin128Auto) new_speed = kDaikin128FanAuto;
      // FALL-THRU
    case kDaikin128FanAuto:
    case kDaikin128FanHigh:
    case kDaikin128FanMed:
    case kDaikin128FanLow:
      _->Fan = new_speed;
      break;
    default:
      _->Fan = kDaikin128FanAuto;
      return;
  }
}

static void IRDaikin128setMode(Daikin128Protocol *_, const uint8_t mode) {
  switch (mode) {
    case kDaikin128Auto:
    case kDaikin128Cool:
    case kDaikin128Heat:
    case kDaikin128Fan:
    case kDaikin128Dry:
      _->Mode = mode;
      break;
    default:
      _->Mode = kDaikin128Auto;
      break;
  }
  // Force a reset of mode dependant things.
  IRDaikin128setFan(_, _->Fan);  // Covers Quiet & Powerful too.

  _->Econo = (0 && (_->Mode == kDaikin128Cool || _->Mode == kDaikin128Heat));
  // setEcono(getEcono());
}

static void IRDaikin128setTemp(Daikin128Protocol *_, const uint8_t temp) {
  _->Temp = uint8ToBcd(min(kDaikin128MaxTemp,
                              max(temp, kDaikin128MinTemp)));
}

static void sendDaikin128(const unsigned char data[], const uint16_t nbytes,
                           const uint16_t repeat) {
  if (nbytes < kDaikin128SectionLength)
    return;  // Not enough bytes to send a partial message.
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    enableIROut(kDaikin128Freq, 50);
    // Leader
    for (uint8_t i = 0; i < 2; i++) {
      mark(kDaikin128LeaderMark);
      space(kDaikin128LeaderSpace);
    }
    // Section #1 (Header + Data)
    sendGeneric(kDaikin128HdrMark, kDaikin128HdrSpace, kDaikin128BitMark,
                kDaikin128OneSpace, kDaikin128BitMark, kDaikin128ZeroSpace,
                kDaikin128BitMark, kDaikin128Gap, data,
                kDaikin128SectionLength,
                kDaikin128Freq, false, 0, kDutyDefault);
    // Section #2 (Data + Footer)
    sendGeneric(0, 0, kDaikin128BitMark,
                kDaikin128OneSpace, kDaikin128BitMark, kDaikin128ZeroSpace,
                kDaikin128FooterMark, kDaikin128Gap,
                data + kDaikin128SectionLength,
                nbytes - kDaikin128SectionLength,
                kDaikin128Freq, false, 0, kDutyDefault);
  }
  vTaskExitCritical();
}

void Daikin128_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);
    Daikin128Protocol _;
    IRDaikin128stateReset(&_);
    _.SwingV = 1;
    _.Power = 0;

    IRDaikin128setFan(&_, state_ir.fanspeed);
    IRDaikin128setMode(&_, state_ir.mode);

    IRDaikin128setTemp(&_, state_ir.degrees);

    _.Sum1 = IRDaikin128calcFirstChecksum(_.raw);
    _.Sum2 = IRDaikin128calcSecondChecksum(_.raw);

    sendDaikin128(_.raw, kDaikin128StateLength, kDaikin128DefaultRepeat); 
}


/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------Daikin64-------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/

static uint8_t IRDaikin64calcChecksum(const uint64_t state) {
  uint64_t data = GETBITS64(state, 0, kDaikin64ChecksumOffset);
  uint8_t result = 0;
  for (; data; data >>= 4)  // Add each nibble together.
    result += GETBITS64(data, 0, 4);
  return result & 0xF;
}

static bool IRDaikin64validChecksum(const uint64_t state) {
  // Validate the checksum of the given state.
  return (GETBITS64(state, kDaikin64ChecksumOffset,
                    kDaikin64ChecksumSize) == IRDaikin64calcChecksum(state));
}

bool decodeDaikin64(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits + kDaikin64Overhead - offset)
    return false;  // Too short a message to match.
  // Compliance
  if (strict && nbits != kDaikin64Bits)
    return false;

  // Leader
  for (uint8_t i = 0; i < 2; i++) {
    if (!matchLimit(results->rawbuf[offset++], kDaikin64LdrMark-2000, kDaikin64LdrMark+2000))
      return false;
    if (!matchLimit(results->rawbuf[offset++], kDaikin64LdrSpace-2000, kDaikin64LdrSpace+2000))
      return false;
  }
  // Header + Data + Footer #1
  uint16_t used = matchGenericDataLimit(results->rawbuf + offset, &results->value,
                               results->rawlen - offset, nbits,
                               kDaikin64HdrMark-800, kDaikin64HdrMark+800, kDaikin64HdrSpace-500, kDaikin64HdrSpace+500,
                               kDaikin64BitMark-250, kDaikin64BitMark+300, kDaikin64OneSpace-350, kDaikin64OneSpace+350,
                               kDaikin64BitMark-250, kDaikin64BitMark+300, kDaikin64ZeroSpace-250, kDaikin64ZeroSpace+300,
                               kDaikin64BitMark-250, kDaikin64BitMark+300, kDaikin64Gap-5000, kDaikin64Gap+5000,
                               false, _tolerance + kDaikin64ToleranceDelta,
                               kMarkExcess, false);
  if (used == 0) return false;
  offset += used;
  // Footer #2
  if (!matchLimit(results->rawbuf[offset++], kDaikin64HdrMark-1000, kDaikin64HdrMark+1000))
    return false;

  // Compliance
  if (strict && !IRDaikin64validChecksum(results->value)) {
    os_trace("%s invalid checksum\r\n", __func__);
    return false;
  }
  // Success
  results->decode_type = DAIKIN64;
  results->bits = nbits;
  results->command = 0;
  results->address = 0;
  os_trace("%s success\r\n", __func__);
  return true;
}

static opmode_t IRDaikin64toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kDaikin64Cool: return opmode_kCool;
    case kDaikin64Heat: return opmode_kHeat;
    case kDaikin64Dry:  return opmode_kDry;
    case kDaikin64Fan:  return opmode_kFan;
    default: return opmode_kAuto;
  }
}

static fanspeed_t IRDaikin64toCommonFanSpeed(const uint8_t speed) {
  switch (speed) {
    case kDaikin64FanTurbo: return fanspeed_kMax;
    case kDaikin64FanHigh:  return fanspeed_kHigh;
    case kDaikin64FanMed:   return fanspeed_kMedium;
    case kDaikin64FanLow:   return fanspeed_kLow;
    case kDaikinFanQuiet:   return fanspeed_kMin;
    default:                return fanspeed_kAuto;
  }
}

static uint8_t IRDaikin64getTemp(Daikin64Protocol _){ return bcdToUint8(_.Temp); }

void Daikin64_setRaw(const uint64_t new_state, const decode_type_t protocol, state_ir_t *ir_state) 
{
  os_trace("%s\r\n", __func__);
  Daikin64Protocol _;
  _.raw = new_state;

  ir_state->protocol = DAIKIN64;
  ir_state->power ^= _.Power;
  ir_state->mode = IRDaikin64toCommonMode(_.Mode);
  ir_state->fanspeed = IRDaikin64toCommonFanSpeed(_.Fan);

  ir_state->degrees = IRDaikin64getTemp(_);
  ir_state->swingv = _.SwingV ? swingv_kAuto : swingv_kOff;
}

static bool daikin64_pre_power = true;
void Daikin64_setInfo(const uint64_t new_state, const decode_type_t protocol, state_ir_t *ir_state) 
{
  os_trace("%s\r\n", __func__);
  Daikin64Protocol _;
  _.raw = new_state;

  ir_state->protocol = DAIKIN64;
  if (_.Power) {
    daikin64_pre_power = !daikin64_pre_power;
  }
  ir_state->power = daikin64_pre_power;
  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;

  ir_state->degrees = IRDaikin64getTemp(_);
  ir_state->swingv = _.SwingV;
}

static void IRDaikin64setMode(Daikin64Protocol *_, const uint8_t mode) {
  switch (mode) {
    case kDaikin64Fan:
    case kDaikin64Dry:
    case kDaikin64Cool:
    case kDaikin64Heat:
      _->Mode = mode;
      break;
    default:
      _->Mode = kDaikin64Cool;
  }
}

static void IRDaikin64setFan(Daikin64Protocol *_, const uint8_t speed) {
  switch (speed) {
    case kDaikin64FanQuiet:
    case kDaikin64FanTurbo:
    case kDaikin64FanAuto:
    case kDaikin64FanHigh:
    case kDaikin64FanMed:
    case kDaikin64FanLow:
      _->Fan = speed;
      break;
    default:
      _->Fan = kDaikin64FanAuto;
  }
}

static void IRDaikin64setTemp(Daikin64Protocol *_, const uint8_t temp) {
  uint8_t degrees = max(temp, kDaikin64MinTemp);
  degrees = min(degrees, kDaikin64MaxTemp);
  _->Temp = uint8ToBcd(degrees);
}

static void sendDaikin64(const uint64_t data, const uint16_t nbits,
                          const uint16_t repeat) {
  vTaskEnterCritical();                          
  enableIROut(kDaikin64Freq, 50);
  for (uint16_t r = 0; r <= repeat; r++) {
    for (uint8_t i = 0; i < 2; i++) {
      // Leader
      mark(kDaikin64LdrMark);
      space(kDaikin64LdrSpace);
    }
    // Header + Data + Footer #1
    sendGeneric(kDaikin64HdrMark, kDaikin64HdrSpace,
                kDaikin64BitMark, kDaikin64OneSpace,
                kDaikin64BitMark, kDaikin64ZeroSpace,
                kDaikin64BitMark, kDaikin64Gap,
                data, nbits, kDaikin64Freq, false, 0, 50);
    // Footer #2
    mark(kDaikin64HdrMark);
    space(kDefaultMessageGap);  // A guess of the gap between messages.
  }
}

void Daikin64_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);
    Daikin64Protocol _;
    _.raw = kDaikin64KnownGoodState;

    if (state_ir.swingv != swingv_kOff) {
      _.SwingV = 1;
    }

    if (state_ir.power != daikin64_pre_power) {
      daikin64_pre_power = !daikin64_pre_power;
      _.Power = 1;
    } else {
      _.Power = 0;
    }

    IRDaikin64setMode(&_, state_ir.mode);
    IRDaikin64setFan(&_, state_ir.fanspeed);

    IRDaikin64setTemp(&_, state_ir.degrees);

    _.Sum = IRDaikin64calcChecksum(_.raw);

    sendDaikin64(_.raw, kDaikin64Bits, kDaikin64DefaultRepeat); 
}

static bool IRDaikin176_validChecksum(uint8_t state[], const uint16_t length) {
  // Validate the checksum of section #1.
  if (length <= kDaikin176Section1Length - 1 ||
      state[kDaikin176Section1Length - 1] != sumBytesState(
          state, kDaikin176Section1Length - 1, 0))
    return false;
  // Validate the checksum of section #2 (a.k.a. the rest)
  if (length <= kDaikin176Section1Length + 1 ||
      state[length - 1] != sumBytesState(state + kDaikin176Section1Length,
                                    length - kDaikin176Section1Length - 1, 0))
    return false;
  return true;
}

bool decodeDaikin176(decode_results *results, uint16_t offset,
                             const uint16_t nbits,
                             const bool strict) {
  if (results->rawlen < 2 * (nbits + kHeader + kFooter) - 1 + offset)
    return false;

  // Compliance
  if (strict && nbits != kDaikin176Bits) return false;

  uint8_t ksectionSize[] = {kDaikin176Section1Length,
                                                    kDaikin176Section2Length};

  // Sections
  uint16_t pos = 0;
  for (uint8_t section = 0; section < kDaikin176Sections; section++) {
    uint16_t used;
    // Section Header + Section Data (7 bytes) + Section Footer
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, ksectionSize[section] * 8,
                        kDaikin176HdrMark-1000, kDaikin176HdrMark+1000, kDaikin176HdrSpace-500, kDaikin176HdrSpace+500,
                        kDaikin176BitMark-270, kDaikin176BitMark+330, kDaikin176OneSpace-500, kDaikin176OneSpace+500,
                        kDaikin176BitMark-270, kDaikin176BitMark+330, kDaikin176ZeroSpace-500, kDaikin176ZeroSpace+500,
                        kDaikin176BitMark-270, kDaikin176BitMark+330, kDaikin176Gap-6000, kDaikin176Gap+6000,
                        section >= kDaikin176Sections - 1,
                        kDaikinTolerance, kDaikinMarkExcess, false);
    if (used == 0) return false;
    offset += used;
    pos += ksectionSize[section];
  }
  // Compliance
  if (strict) {
    // Validate the checksum.
    if (!IRDaikin176_validChecksum(results->state, kDaikin176StateLength)){
      os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }

  // Success
  os_trace("%s success\r\n", __func__);
  results->decode_type = DAIKIN176;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRDaikin176_setRaw(const uint8_t new_code[], state_ir_t *ir_state) {
  Daikin176Protocol _;
  memset(&_, 0, sizeof(Daikin176Protocol));
  memcpy(_.raw, new_code, kDaikin176StateLength);

  ir_state->power = _.Power;
  ir_state->degrees = _.Temp + 9;
  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;
  ir_state->swingh = _.SwingH;
  // _saved_temp = getTemp();
}


static void sendDaikin176(const unsigned char data[], const uint16_t nbytes,
                           const uint16_t repeat) {
  if (nbytes < kDaikin176Section1Length)
    return;  // Not enough bytes to send a partial message.
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    // Section #1
    sendGenericBytes(kDaikin176HdrMark, kDaikin176HdrSpace, kDaikin176BitMark,
                kDaikin176OneSpace, kDaikin176BitMark, kDaikin176ZeroSpace,
                kDaikin176BitMark, kDaikin176Gap, data,
                kDaikin176Section1Length,
                kDaikin176Freq, false, 0, kDutyDefault);
    // Section #2
    sendGenericBytes(kDaikin176HdrMark, kDaikin176HdrSpace, kDaikin176BitMark,
                kDaikin176OneSpace, kDaikin176BitMark, kDaikin176ZeroSpace,
                kDaikin176BitMark, kDaikin176Gap,
                data + kDaikin176Section1Length,
                nbytes - kDaikin176Section1Length,
                kDaikin176Freq, false, 0, kDutyDefault);
  }
  vTaskExitCritical();
}

void Daikin176_send(state_ir_t state_ir)
{
  Daikin176Protocol _;
  memset(&_, 0, sizeof(Daikin176Protocol));

  for (uint8_t i = 0; i < kDaikin176StateLength; i++) _.raw[i] = 0x00;
  _.raw[0] =  0x11;
  _.raw[1] =  0xDA;
  _.raw[2] =  0x17;
  _.raw[3] =  0x18;
  _.raw[4] =  0x04;
  // _.raw[6] is a checksum byte, it will be set by checksum().
  _.raw[7] =  0x11;
  _.raw[8] =  0xDA;
  _.raw[9] =  0x17;
  _.raw[10] = 0x18;
  _.raw[12] = 0x73;
  _.raw[14] = 0x20;
  _.raw[18] = 0x16;  // Fan speed and swing
  _.raw[20] = 0x20;

  // power
  _.Power = state_ir.power;

  // mode
  uint8_t altmode = 0;
  // Set the mode bits.
  _.Mode = state_ir.mode;
  // Daikin172 has some alternate/additional mode bits that need to be changed
  // in line with the operating mode. The following few lines match up these
  // bits with the corresponding operating bits.
  switch ((uint8_t)state_ir.mode) {
    case kDaikin176Dry:  altmode = 2; break;
    case kDaikin176Fan:  altmode = 6; break;
    case kDaikin176Auto:
    case kDaikin176Cool:
    case kDaikin176Heat: altmode = 7; break;
    default: _.Mode = kDaikin176Cool; altmode = 7; break;
  }
  // Set the additional mode bits.
  _.AltMode = altmode;

  // set fanspeed
  switch ((uint8_t) state_ir.fanspeed) {
    case kDaikinFanMin:
    case kDaikin176FanMax:
      _.Fan = state_ir.fanspeed;
      break;
    default:
      _.Fan = kDaikin176FanMax;
      break;
  }

  uint8_t degrees = min(kDaikinMaxTemp, max((uint8_t)state_ir.degrees, kDaikinMinTemp));
  switch (_.Mode) {
    case kDaikin176Dry:
    case kDaikin176Fan:
      degrees = kDaikin176DryFanTemp; break;
  }
  _.Temp = degrees - 9;

  if (state_ir.swingh != swingh_kOff) {
    switch ((uint8_t)state_ir.swingh) {
      case kDaikin176SwingHOff:
      case kDaikin176SwingHAuto:
        _.SwingH = state_ir.swingh;
        break;
      default: _.SwingH = kDaikin176SwingHAuto;
    }
  }

  _.Sum1 = sumBytesState(_.raw, kDaikin176Section1Length - 1, 0);
  _.Sum2 = sumBytesState(_.raw + kDaikin176Section1Length,
                    kDaikin176Section2Length - 1, 0);

  sendDaikin176(_.raw, kDaikin176StateLength, kDaikin176DefaultRepeat);
}
