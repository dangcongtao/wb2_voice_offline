#include "ir_Samsung.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

#define kSamsungTick 560
#define kSamsungHdrMarkTicks 8
#define kSamsungHdrMark kSamsungHdrMarkTicks * kSamsungTick
#define kSamsungHdrSpaceTicks 8
#define kSamsungHdrSpace kSamsungHdrSpaceTicks * kSamsungTick
#define kSamsungBitMarkTicks 1
#define kSamsungBitMark kSamsungBitMarkTicks * kSamsungTick
#define kSamsungOneSpaceTicks 3
#define kSamsungOneSpace kSamsungOneSpaceTicks * kSamsungTick
#define kSamsungZeroSpaceTicks 1
#define kSamsungZeroSpace kSamsungZeroSpaceTicks * kSamsungTick
#define kSamsungRptSpaceTicks 4
#define kSamsungRptSpace kSamsungRptSpaceTicks * kSamsungTick
#define kSamsungMinMessageLengthTicks 193
#define kSamsungMinMessageLength (kSamsungMinMessageLengthTicks * kSamsungTick)
#define kSamsungMinGapTicks (kSamsungMinMessageLengthTicks - (kSamsungHdrMarkTicks + kSamsungHdrSpaceTicks + kSamsungBits * (kSamsungBitMarkTicks + kSamsungOneSpaceTicks) +kSamsungBitMarkTicks))
#define kSamsungMinGap (kSamsungMinGapTicks * kSamsungTick)

#define kSamsungAcHdrMark 690
#define kSamsungAcHdrSpace 17844
#define kSamsungAcSections 2
#define kSamsungAcSectionMark 3086
#define kSamsungAcSectionSpace 8864
#define kSamsungAcSectionGap 2886
#define kSamsungAcBitMark 560
#define kSamsungAcOneSpace 1560
#define kSamsungAcZeroSpace 436

// Data from https://github.com/crankyoldgit/IRremoteESP8266/issues/1220
// Values calculated based on the average of ten messages.
#define kSamsung36HdrMark 4515  /// < uSeconds
#define kSamsung36HdrSpace 4438  /// < uSeconds
#define kSamsung36BitMark 512  /// < uSeconds
#define kSamsung36OneSpace 1468  /// < uSeconds
#define kSamsung36ZeroSpace 490  /// < uSeconds

// _.Swing
#define kSamsungAcSwingV        0b010
#define kSamsungAcSwingH        0b011
#define kSamsungAcSwingBoth     0b100
#define kSamsungAcSwingOff      0b111
// _.FanSpecial
#define kSamsungAcFanSpecialOff 0b000
#define kSamsungAcPowerfulOn    0b011
#define kSamsungAcBreezeOn      0b101
#define kSamsungAcEconoOn       0b111

// uint8_t _tolerance;


bool decodeSAMSUNG(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict) {

  if (strict && nbits != kSamsungBits)
    return false;  // We expect Samsung to be 32 bits of message.

  uint64_t data = 0;

  // Match Header + Data + Footer
  if (!matchGeneric(results->rawbuf + offset, &data,
                    results->rawlen - offset, nbits,
                    kSamsungHdrMark, kSamsungHdrSpace,
                    kSamsungBitMark, kSamsungOneSpace,
                    kSamsungBitMark, kSamsungZeroSpace,
                    kSamsungBitMark, kSamsungMinGap, true, kUseDefTol, 0, true)) return false;
  // Compliance
  // According to the spec, the customer (address) code is the first 8
  // transmitted bits. It's then repeated. Check for that.
  uint8_t address = data >> 24;
  if (strict && address != ((data >> 16) & 0xFF)) return false;
  // Spec says the command code is the 3rd block of transmitted 8-bits,
  // followed by the inverted command code.
  uint8_t command = (data & 0xFF00) >> 8;
  if (strict && command != ((data & 0xFF) ^ 0xFF)) return false;

  // Success
  results->bits = nbits;
  results->value = data;
  results->decode_type = SAMSUNG;
  // command & address need to be reversed as they are transmitted LSB first,
  results->command = reverseBits(command, sizeof(command) * 8);
  results->address = reverseBits(address, sizeof(address) * 8);
  os_trace("%s success\r\n", __func__);
  return true;
}

uint8_t getSectionChecksum(const uint8_t *section) {
  return ((GETBITS8(*(section + 2), kLowNibble, kNibbleSize) << kNibbleSize) +
          GETBITS8(*(section + 1), kHighNibble, kNibbleSize));
}

uint8_t calcSectionChecksum(uint8_t *section) {
  uint8_t sum = 0;

  // sum += countBits(section, 8, true, 0);  // Include the entire first byte

  // os_trace("%s sum1: %u\r\n", __func__, sum);
  // // The lower half of the second byte.
  // uint8_t data = GETBITS8(*(section + 1), kLowNibble, kNibbleSize);
  // sum += countBits(&data, 8, true, 0);

  // os_trace("%s sum2: %u\r\n", __func__, sum);
  // // The upper half of the third byte.
  // data = GETBITS8(*(section + 2), kHighNibble, kNibbleSize);
  // sum += countBits(&data, 8, true, 0);

  // os_trace("%s sum3: %u\r\n", __func__, sum);
  // // The next 4 bytes.
  // sum += countBits(section + 3, 4, true, 0);

  // os_trace("%s sum4: %u\r\n", __func__, sum);

  sum += countBits64(*section, 8, true, 0);  // Include the entire first byte
  // The lower half of the second byte.
  sum += countBits64(GETBITS8(*(section + 1), kLowNibble, kNibbleSize), 8, true, 0);
  // The upper half of the third byte.
  sum += countBits64(GETBITS8(*(section + 2), kHighNibble, kNibbleSize), 8, true, 0);
  // The next 4 bytes.
  sum += countBits(section + 3, 4, true, 0);
  // Bitwise invert the result.
  return sum ^ UINT8_MAX;
}

static bool validChecksum(const uint8_t state[], const uint16_t length) {
  bool result = true;
  const uint16_t maxlength =
      (length > kSamsungAcExtendedStateLength) ? kSamsungAcExtendedStateLength
                                               : length;
  for (uint16_t offset = 0;
       offset + kSamsungAcSectionLength <= maxlength;
       offset += kSamsungAcSectionLength) {

        uint8_t cal2 = calcSectionChecksum(state + offset);
        uint8_t cal1 = getSectionChecksum(state + offset);
    result &= (cal1 == cal2);
  }
  return result;
}

bool decodeSamsungAC(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits + kHeader * 3 + kFooter * 2 - 1 + offset){
    // os_trace("Can't possibly be a valid Samsung A/C message.\r\n");
    return false;  // Can't possibly be a valid Samsung A/C message.
  }
  if (nbits != kSamsungAcBits && nbits != kSamsungAcExtendedBits){
    // os_trace("step 1 failed\r\n");
    return false;
  }

  // Message Header
  if (!matchMark(results->rawbuf[offset++], kSamsungAcBitMark, kUseDefTol, kMarkExcess)){
    // os_trace("match mark failed\r\n");
    return false;
  }
  if (!matchSpace(results->rawbuf[offset++], kSamsungAcHdrSpace, kUseDefTol, kMarkExcess)){
    // os_trace("match space failed\r\n");
    return false;
  }
  // Section(s)
  for (uint16_t pos = 0; pos <= (nbits / 8) - kSamsungAcSectionLength;
       pos += kSamsungAcSectionLength) {
    uint16_t used;
    // Section Header + Section Data (7 bytes) + Section Footer
    used = matchGeneric(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, kSamsungAcSectionLength * 8,
                        kSamsungAcSectionMark, kSamsungAcSectionSpace,
                        kSamsungAcBitMark, kSamsungAcOneSpace,
                        kSamsungAcBitMark, kSamsungAcZeroSpace,
                        kSamsungAcBitMark, kSamsungAcSectionGap,
                        pos + kSamsungAcSectionLength >= nbits / 8,
                        40, 0, false);
    if (used == 0){
      os_trace("%s step 2 not used failed\r\n", __func__);
      return false;
    }
    offset += used;
  }
  // Compliance
  if (strict) {
    // Is the checksum valid?
    if (!validChecksum(results->state, nbits / 8)) {
      os_trace("DEBUG: Checksum failed!\r\n");
      return false;
    }
  }
  // Success
  results->decode_type = SAMSUNG_AC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  os_trace("decode samsung ac success\r\n");
  return true;
}

static void checksum(SamsungProtocol *_) {
  uint8_t sectionsum = calcSectionChecksum(_->raw);
  _->Sum1Upper = GETBITS8(sectionsum, kHighNibble, kNibbleSize);
  _->Sum1Lower = GETBITS8(sectionsum, kLowNibble, kNibbleSize);
  sectionsum = calcSectionChecksum(_->raw + kSamsungAcSectionLength);
  _->Sum2Upper = GETBITS8(sectionsum, kHighNibble, kNibbleSize);
  _->Sum2Lower = GETBITS8(sectionsum, kLowNibble, kNibbleSize);
  sectionsum = calcSectionChecksum(_->raw + kSamsungAcSectionLength * 2);
  _->Sum3Upper = GETBITS8(sectionsum, kHighNibble, kNibbleSize);
  _->Sum3Lower = GETBITS8(sectionsum, kLowNibble, kNibbleSize);
}


void sendSamsungAC(const uint8_t data[], const uint16_t nbytes,
                           const uint16_t repeat) {
  if (nbytes < kSamsungAcStateLength && nbytes % kSamsungAcSectionLength)
    return;  // Not an appropriate number of bytes to send a proper message.
  vTaskEnterCritical();
  enableIROut(38, 50);
  for (uint16_t r = 0; r <= repeat; r++) {
    // Header
    mark(kSamsungAcHdrMark);
    space(kSamsungAcHdrSpace);
    // Send in 7 byte sections.
    for (uint16_t offset = 0; offset < nbytes;
         offset += kSamsungAcSectionLength) {
      sendGenericBytes(kSamsungAcSectionMark, kSamsungAcSectionSpace,
                  kSamsungAcBitMark, kSamsungAcOneSpace, kSamsungAcBitMark,
                  kSamsungAcZeroSpace, kSamsungAcBitMark, kSamsungAcSectionGap,
                  data + offset, kSamsungAcSectionLength,  // 7 bytes == 56 bits
                  38000, false, 0, 50);                    // Send in LSBF order
    }
    // Complete made up guess at inter-message gap.
    space(kDefaultMessageGap - kSamsungAcSectionGap);
  }
  vTaskExitCritical();
}

void Samsung_sendExtended(state_ir_t state_ir) 
{

  SamsungProtocol _;

  static const uint8_t kReset[kSamsungAcExtendedStateLength] = {
      0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
      0x01, 0x02, 0xAE, 0x71, 0x00, 0x15, 0xF0};
  memcpy(_.raw, kReset, kSamsungAcExtendedStateLength);

  // temp
  uint8_t my_temp = (uint8_t)state_ir.degrees;
  uint8_t newtemp = max(kSamsungAcMinTemp, my_temp);
  newtemp = min(kSamsungAcMaxTemp, newtemp);
  _.Temp = newtemp - kSamsungAcMinTemp;

  // mode
  uint8_t newmode = state_ir.mode;
  if (newmode > kSamsungAcHeat) newmode = kSamsungAcAuto;
  _.Mode = newmode;

  // Auto mode has a special fan setting valid only in auto mode.
  if (newmode == kSamsungAcAuto) {
    _.Fan = kSamsungAcFanAuto2;
  } else {
    // Non-Auto can't have this fan setting
    if (_.Fan == kSamsungAcFanAuto2)
      _.Fan = kSamsungAcFanAuto;  // Default to something safe.
  }

  switch ((uint8_t)state_ir.fanspeed) {
    case kSamsungAcFanAuto:
    case kSamsungAcFanLow:
    case kSamsungAcFanMed:
    case kSamsungAcFanHigh:
    case kSamsungAcFanTurbo:
      if (_.Mode == kSamsungAcAuto) {
          break;  // Not valid in Auto mode.
      }
      _.Fan = (uint8_t)state_ir.fanspeed;
      break;
    case kSamsungAcFanAuto2:  // Special fan setting for when in Auto mode.
      if (_.Mode != kSamsungAcAuto){
        break;
      }
      _.Fan = (uint8_t)state_ir.fanspeed;
      break;
    default: {
      break;
    }
  }

  _.Power1 = _.Power2 = (state_ir.power ? 0b11 : 0b00);

  static const uint8_t extended_middle_section[kSamsungAcSectionLength] = {
      0x01, 0xD2, 0x0F, 0x00, 0x00, 0x00, 0x00};

  memcpy(_.raw + 2 * kSamsungAcSectionLength,
              _.raw + kSamsungAcSectionLength,
              kSamsungAcSectionLength);
  memcpy(_.raw + kSamsungAcSectionLength, extended_middle_section,
              kSamsungAcSectionLength);


  _.OnTimerEnable = 0;
  _.OnTimeDay = 0;

  checksum(&_);

  // Send it.
  sendSamsungAC(_.raw, kSamsungAcExtendedStateLength, 0);
  // Now revert it by copying the third section over the second section.
  // memcpy(_.raw + kSamsungAcSectionLength,
  //             _.raw + 2 * kSamsungAcSectionLength,
  //             kSamsungAcSectionLength);

  // _forceextended = false;  // It has now been sent, so clear the flag if set.
}


// send 14 bytes
void Samsung_send(state_ir_t state_ir)
{
  os_trace("%s\r\n", __func__);
  SamsungProtocol _;

  static const uint8_t kReset[kSamsungAcExtendedStateLength] = {
      0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
      0x01, 0x02, 0xAE, 0x71, 0x00, 0x15, 0xF0};
  memcpy(_.raw, kReset, kSamsungAcExtendedStateLength);

  // temp
  uint8_t my_temp = (uint8_t)state_ir.degrees;
  uint8_t newtemp = max(kSamsungAcMinTemp, my_temp);
  newtemp = min(kSamsungAcMaxTemp, newtemp);
  _.Temp = newtemp - kSamsungAcMinTemp;

  // mode
  uint8_t newmode = state_ir.mode;
  if (newmode > kSamsungAcHeat) newmode = kSamsungAcAuto;
  _.Mode = newmode;

  // Auto mode has a special fan setting valid only in auto mode.
  if (newmode == kSamsungAcAuto) {
    _.Fan = kSamsungAcFanAuto2;
  } else {
    // Non-Auto can't have this fan setting
    if (_.Fan == kSamsungAcFanAuto2)
      _.Fan = kSamsungAcFanAuto;  // Default to something safe.
  }

  switch ((uint8_t)state_ir.fanspeed) {
    case kSamsungAcFanAuto:
    case kSamsungAcFanLow:
    case kSamsungAcFanMed:
    case kSamsungAcFanHigh:
    case kSamsungAcFanTurbo:
      if (_.Mode == kSamsungAcAuto) {
          // os_trace("Not valid in Auto mode\r\n");
          break;  // Not valid in Auto mode.
      }
      _.Fan = (uint8_t)state_ir.fanspeed;
      break;
    case kSamsungAcFanAuto2:  // Special fan setting for when in Auto mode.
      if (_.Mode != kSamsungAcAuto){
        // os_trace("Special fan setting for when in Auto mode.\r\n");
        break;
      }
      _.Fan = (uint8_t)state_ir.fanspeed;
      break;
    default: {
      // os_trace("default fanspeed not valid %u\r\n", (uint8_t)state_ir.fanspeed);
      break;
    }
  }
  
  _.Power1 = _.Power2 = (state_ir.power ? 0b11 : 0b00);

  checksum(&_);

  os_trace("SAMSUNG_SEND: 0x");
  for (uint8_t i = 0; i < 14; i++) {
      if (i == 7) {
          os_trace("\t|\t");
      } else if (i == 14) {
        break;
      }
      os_trace("%02x", _.raw[i]);
  }
  os_trace("\r\n");

  // state_ir_t my_state_ir;

  // if (!validChecksum(_.raw, kSamsungAcStateLength)) {
  //   os_trace("DEBUG: Checksum failed!\r\n");
  //   // return false;
  // }

  // Samsung_setRaw(_.raw, kSamsungAcStateLength, SAMSUNG_AC, &my_state_ir);

  // IR_logCommon(my_state_ir);

  sendSamsungAC(_.raw, kSamsungAcStateLength, 0);
}

static opmode_t get_mode(const uint8_t mode)
{
  switch (mode) {
    case kSamsungAcCool: return opmode_kCool;
    case kSamsungAcHeat: return opmode_kHeat;
    case kSamsungAcDry:  return opmode_kDry;
    case kSamsungAcFan:  return opmode_kFan;
    default:             return opmode_kAuto;
  }
}

static opmode_t get_fan(const uint8_t spd)
{
  switch (spd) {
    case kSamsungAcFanTurbo: return fanspeed_kMax;
    case kSamsungAcFanHigh:  return fanspeed_kHigh;
    case kSamsungAcFanMed:   return fanspeed_kMedium;
    case kSamsungAcFanLow:   return fanspeed_kMin;
    default:                 return fanspeed_kAuto;
  }
}

static bool getPower(SamsungProtocol _)
{
  return _.Power1 == 0b11 && _.Power2 == 0b11;
}

static uint8_t getTemp(SamsungProtocol _)
{
  return _.Temp + kSamsungAcMinTemp;
}

static void Samsung_toCommon(SamsungProtocol _, state_ir_t *ir_state)
{
    ir_state->protocol = SAMSUNG_AC;
    ir_state->model = -1;
    ir_state->mode = get_mode(_.Mode);
    ir_state->fanspeed = get_fan(_.Fan);
    ir_state->power = getPower(_);
    ir_state->degrees = getTemp(_);
}

void Samsung_setRaw(const uint8_t* new_code, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
    // memcpy(_.data.raw, new_code, kFunikiAcStateLength);
    SamsungProtocol _;
    memset(&_, 0, sizeof(SamsungProtocol));
    memcpy(_.raw, new_code, min(length, kSamsungAcExtendedStateLength));
    
    if (length > kSamsungAcStateLength) {
      // _OnTimerEnable = _.OnTimerEnable;
      // _OffTimerEnable = _.OffTimerEnable;
      // _Sleep = _.Sleep5 && _.Sleep12;
      // _OnTimer = _getOnTimer();
      // _OffTimer = _getOffTimer();
      for (uint8_t i = kSamsungAcStateLength; i < length; i++)
        _.raw[i - kSamsungAcSectionLength] = _.raw[i];
    }

    Samsung_toCommon(_, ir_state);
    
    // memcpy(&_.data.raw, new_code, 12);
    // os_trace("fan: %u\r\n", _.data.fan);
    // _.data.raw = new_code;
}

void Samsung_setInfo(const uint8_t* new_code, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) 
{
    // memcpy(_.data.raw, new_code, kFunikiAcStateLength);
    SamsungProtocol _;
    memset(&_, 0, sizeof(SamsungProtocol));
    memcpy(_.raw, new_code, min(length, kSamsungAcExtendedStateLength));
    
    if (length > kSamsungAcStateLength) {
      // _OnTimerEnable = _.OnTimerEnable;
      // _OffTimerEnable = _.OffTimerEnable;
      // _Sleep = _.Sleep5 && _.Sleep12;
      // _OnTimer = _getOnTimer();
      // _OffTimer = _getOffTimer();
      for (uint8_t i = kSamsungAcStateLength; i < length; i++)
        _.raw[i - kSamsungAcSectionLength] = _.raw[i];
    }

    ir_state->protocol = SAMSUNG_AC;
    ir_state->model = -1;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->power = getPower(_);
    ir_state->degrees = getTemp(_);

    // memcpy(&_.data.raw, new_code, 12);
    // os_trace("fan: %u\r\n", _.data.fan);
    // _.data.raw = new_code;
}