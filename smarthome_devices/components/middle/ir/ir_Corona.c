#include "ir_Corona.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

// Constants
const uint16_t kCoronaAcHdrMark = 3500;
const uint16_t kCoronaAcHdrSpace = 1680;
const uint16_t kCoronaAcBitMark = 450;
const uint16_t kCoronaAcOneSpace = 1270;
const uint16_t kCoronaAcZeroSpace = 420;
const uint16_t kCoronaAcSpaceGap = 10800;
const uint16_t kCoronaAcFreq = 38000;  // Hz.
const uint16_t kCoronaAcOverheadShort = 3;
const uint16_t kCoronaAcOverhead = 11;  // full message
const uint8_t kCoronaTolerance = 5;  // +5%

static uint8_t getSectionByte(const uint8_t section) {
  // base byte
  uint8_t b = kCoronaAcSectionLabelBase;
  // 2 enabled bits shifted 0-2 bits depending on section
  if (section >= 3)
    return 0b10010000 | b;
  setBitsU8(&b, kHighNibble, kNibbleSize, 0b11 << section);
  return b;
}

static bool IRCoronaAc_validSection(const uint8_t state[], const uint16_t pos,
                              const uint8_t section) {
  // sanity check, pos must match section, section 4 is at pos 0
  if ((section % kCoronaAcSections) * kCoronaAcSectionBytes != pos)
    return false;
  // all individual sections has the same prefix
  CoronaSection *p = (CoronaSection*)(state + pos);
  if (p->Header0 != kCoronaAcSectionHeader0) {
    // DPRINT("State ");
    // DPRINT(&(p->Header0) - state);
    // DPRINT(" expected 0x28 was ");
    // DPRINTLN(uint64ToString(p->Header0, 16));
    return false;
  }
  if (p->Header1 != kCoronaAcSectionHeader1) {
    // DPRINT("State ");
    // DPRINT(&(p->Header1) - state);
    // DPRINT(" expected 0x61 was ");
    // DPRINTLN(uint64ToString(p->Header1, 16));
    return false;
  }

  // checking section byte
  if (p->Label != getSectionByte(section)) {
    // DPRINT("check 2 not matching, got ");
    // DPRINT(uint64ToString(p->Label, 16));
    // DPRINT(" expected ");
    // DPRINTLN(uint64ToString(getSectionByte(section), 16));
    return false;
  }

  // checking inverts
  uint8_t d0invinv = ~p->Data0Inv;
  if (p->Data0 != d0invinv) {
    // DPRINT("inverted 3 - 4 not matching, got ");
    // DPRINT(uint64ToString(p->Data0, 16));
    // DPRINT(" vs ");
    // DPRINTLN(uint64ToString(p->Data0Inv, 16));
    return false;
  }
  uint8_t d1invinv = ~p->Data1Inv;
  if (p->Data1 != d1invinv) {
    // DPRINT("inverted 5 - 6 not matching, got ");
    // DPRINT(uint64ToString(p->Data1, 16));
    // DPRINT(" vs ");
    // DPRINTLN(uint64ToString(p->Data1Inv, 16));
    return false;
  }
  return true;
}

bool decodeCoronaAc(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  bool isLong = results->rawlen >= kCoronaAcBits * 2;
  if (results->rawlen < 2 * nbits +
      (isLong ? kCoronaAcOverhead : kCoronaAcOverheadShort)
      - offset)
    return false;  // Too short a message to match.
  if (strict && nbits != kCoronaAcBits && nbits != kCoronaAcBitsShort)
    return false;

  uint16_t pos = 0;
  uint16_t used = 0;

  // Data Section #1 - 3 loop
  // e.g.
  //   bits = 56; bytes = 7;
  // #1  *(results->state + pos) = {0x28, 0x61, 0x3D, 0x19, 0xE6, 0x37, 0xC8};
  // #2  *(results->state + pos) = {0x28, 0x61, 0x6D, 0xFF, 0x00, 0xFF, 0x00};
  // #3  *(results->state + pos) = {0x28, 0x61, 0xCD, 0xFF, 0x00, 0xFF, 0x00};
  for (uint8_t section = 0; section < kCoronaAcSections; section++) {
    // os_trace("%s section: %u\r\n", __func__, section);
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, kCoronaAcBitsShort,
                        kCoronaAcHdrMark-875, kCoronaAcHdrMark+875, kCoronaAcHdrSpace-420, kCoronaAcHdrSpace+420,
                        kCoronaAcBitMark-350, kCoronaAcBitMark+350, kCoronaAcOneSpace-400, kCoronaAcOneSpace+400,
                        kCoronaAcBitMark-350, kCoronaAcBitMark+350, kCoronaAcZeroSpace-370, kCoronaAcZeroSpace+370,
                        kCoronaAcBitMark-350, kCoronaAcBitMark+350, kCoronaAcSpaceGap-2500, kCoronaAcSpaceGap+2500, true,
                        _tolerance + kCoronaTolerance, kMarkExcess, false);
    if (used == 0) return false;  // We failed to find any data.
    // short versions section 0 is special
    if (strict && !IRCoronaAc_validSection(results->state, pos,
                                            isLong ? section : 3))
    {
        os_trace("%s invalid Section\r\n", __func__);
        return false;
    }
    offset += used;  // Adjust for how much of the message we read.
    pos += kCoronaAcSectionBytes;  // Adjust by how many bytes of data was read
    // don't read more data then what we have
    if (results->rawlen <= offset)
      break;
  }

  // Re-check we got the correct size/length due to the way we read the data.
  if (strict && pos * 8 != kCoronaAcBits && pos * 8 != kCoronaAcBitsShort) {
    os_trace("%s strict bit match fail\r\n", __func__);
    return false;
  }
    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = CORONA_AC;
  results->bits = pos * 8;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRCoronaAc_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state) {
  CoronaProtocol _;
  memset(&_, 0, sizeof(CoronaProtocol));
  memcpy(_.raw, new_code, min(length, kCoronaAcStateLength));

  ir_state->power = _.Power;
  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;
  ir_state->degrees = _.Temp + kCoronaAcMinTemp - 1;
  ir_state->swingv = _.SwingVToggle;
  os_trace("%s PowerButton: %u\r\n", __func__, _.PowerButton);
}

static void sendCoronaAc(const uint8_t data[],
                          const uint16_t nbytes, const uint16_t repeat) {
  if (nbytes < kCoronaAcSectionBytes) return;
  if (kCoronaAcSectionBytes < nbytes &&
      nbytes < kCoronaAcStateLength) return;

  for (uint16_t r = 0; r <= repeat; r++) {
    uint16_t pos = 0;
    // Data Section #1 - 3 loop
    // e.g.
    //   bits = 56; bytes = 7;
    // #1  *(data + pos) = {0x28, 0x61, 0x3D, 0x19, 0xE6, 0x37, 0xC8};
    // #2  *(data + pos) = {0x28, 0x61, 0x6D, 0xFF, 0x00, 0xFF, 0x00};
    // #3  *(data + pos) = {0x28, 0x61, 0xCD, 0xFF, 0x00, 0xFF, 0x00};
    for (uint8_t section = 0; section < kCoronaAcSections; section++) {
      sendGenericBytes(kCoronaAcHdrMark, kCoronaAcHdrSpace,
                  kCoronaAcBitMark, kCoronaAcOneSpace,
                  kCoronaAcBitMark, kCoronaAcZeroSpace,
                  kCoronaAcBitMark, kCoronaAcSpaceGap,
                  data + pos, kCoronaAcSectionBytes,
                  kCoronaAcFreq, false, kNoRepeat, kDutyDefault);
      pos += kCoronaAcSectionBytes;  // Adjust by how many bytes was sent
      // don't send more data then what we have
      if (nbytes <= pos)
        break;
    }
  }

}

void Corona_send(state_ir_t state_ir)
{
  CoronaProtocol _;
  memset(&_, 0, sizeof(CoronaProtocol));

  _.sections[kCoronaAcSettingsSection].Data0 = kCoronaAcSectionData0Base;
  _.sections[kCoronaAcSettingsSection].Data1 = 0x00;  // ensure no unset mem
    // convert 16 bit value to separate 8 bit parts
  _.sections[kCoronaAcOnTimerSection].Data1 = kCoronaAcTimerOff >> 8;
  _.sections[kCoronaAcOnTimerSection].Data0 = kCoronaAcTimerOff;
  _.sections[kCoronaAcOffTimerSection].Data1 = kCoronaAcTimerOff >> 8;
  _.sections[kCoronaAcOffTimerSection].Data0 = kCoronaAcTimerOff;

  _.PowerButton = false;
  // power
  _.Power = state_ir.power;

  // temp
  uint8_t degrees = max((uint8_t)state_ir.degrees, kCoronaAcMinTemp);
  degrees = min(degrees, kCoronaAcMaxTemp);
  _.Temp = degrees - kCoronaAcMinTemp + 1;

  // fan
  if ((uint8_t)state_ir.fanspeed > kCoronaAcFanHigh)
    _.Fan = kCoronaAcFanAuto;
  else
    _.Fan = state_ir.fanspeed;

  // mode
  switch ((uint8_t)state_ir.mode) {
    case kCoronaAcModeCool:
    case kCoronaAcModeDry:
    case kCoronaAcModeFan:
    case kCoronaAcModeHeat:
      _.Mode = state_ir.mode;
      break;
    default:
      _.Mode = kCoronaAcModeCool;
  }

  // checksum
  for (uint8_t i = 0; i < kCoronaAcSections; i++) {
    _.sections[i].Header0 = kCoronaAcSectionHeader0;
    _.sections[i].Header1 = kCoronaAcSectionHeader1;
    _.sections[i].Label = getSectionByte(i);
    _.sections[i].Data0Inv = ~_.sections[i].Data0;
    _.sections[i].Data1Inv = ~_.sections[i].Data1;
  }
  vTaskEnterCritical();
  sendCoronaAc(_.raw, kCoronaAcStateLength, kNoRepeat);
  _.PowerButton = true;
  for (uint8_t i = 0; i < kCoronaAcSections; i++) {
    _.sections[i].Header0 = kCoronaAcSectionHeader0;
    _.sections[i].Header1 = kCoronaAcSectionHeader1;
    _.sections[i].Label = getSectionByte(i);
    _.sections[i].Data0Inv = ~_.sections[i].Data0;
    _.sections[i].Data1Inv = ~_.sections[i].Data1;
  }
  sendCoronaAc(_.raw, kCoronaAcStateLength, kNoRepeat);
  vTaskExitCritical();
}