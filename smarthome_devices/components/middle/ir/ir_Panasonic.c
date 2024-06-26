#include "ir_Panasonic.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

const uint16_t kPanasonicHdrMark = 3456;             ///< uSeconds.
const uint16_t kPanasonicHdrSpace = 1728;            ///< uSeconds.
const uint16_t kPanasonicBitMark = 432;              ///< uSeconds.
const uint16_t kPanasonicOneSpace = 1296;            ///< uSeconds.
const uint16_t kPanasonicZeroSpace = 432;            ///< uSeconds.
const uint32_t kPanasonicMinCommandLength = 163296;  ///< uSeconds.
const uint16_t kPanasonicEndGap = 5000;              ///< uSeconds. See #245
const uint32_t kPanasonicMinGap = 74736;             ///< uSeconds.

const uint16_t kPanasonicAcSectionGap = 10000;       ///< uSeconds.
const uint16_t kPanasonicAcSection1Length = 8;
const uint32_t kPanasonicAcMessageGap = kDefaultMessageGap;  // Just a guess.

const uint16_t kPanasonicAc32HdrMark = 3543;         ///< uSeconds.
const uint16_t kPanasonicAc32BitMark = 920;          ///< uSeconds.
const uint16_t kPanasonicAc32HdrSpace = 3450;        ///< uSeconds.
const uint16_t kPanasonicAc32OneSpace = 2575;        ///< uSeconds.
const uint16_t kPanasonicAc32ZeroSpace = 828;        ///< uSeconds.
const uint16_t kPanasonicAc32SectionGap = 13946;     ///< uSeconds.
const uint8_t  kPanasonicAc32Sections = 2;
const uint8_t  kPanasonicAc32BlocksPerSection = 2;

static bool pre_ion_enable = false;
static panasonic_ac_remote_model_t pre_model = kPanasonicUnknown;


bool IRPanasonicAc_validChecksum(const uint8_t *state, const uint16_t length) {
  if (length < 2) return false;  // 1 byte of data can't have a checksum.
  return (state[length - 1] ==
          sumBytesState(state, length - 1, kPanasonicAcChecksumInit));
}

bool IRPanasonicAc_validChecksum_2(const uint8_t *state, const uint16_t length) {
  if (length < 2) return false;  // 1 byte of data can't have a checksum.
  return (state[length - 1] ==
          sumBytesState(state, length - 1, kPanasonicAc2ChecksumInit));
}

bool decodePanasonicAC(decode_results *results, uint16_t offset,
                               const uint16_t nbits, const bool strict) {
  uint8_t min_nr_of_messages = 1;
  if (strict) {
    if (nbits != kPanasonicAcBits && nbits != kPanasonicAcShortBits)
      return false;  // Not strictly a PANASONIC_AC message.
  }

  if (results->rawlen <=
      min_nr_of_messages * (2 * nbits + kHeader + kFooter) - 1 + offset)
    return false;  // Can't possibly be a valid PANASONIC_AC message.

  // Match Header + Data #1 + Footer
  uint16_t used;
  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, kPanasonicAcSection1Length * 8,
                      kPanasonicHdrMark - 456, kPanasonicHdrMark + 456, kPanasonicHdrSpace - 400, kPanasonicHdrSpace + 400,
                      kPanasonicBitMark - 332, kPanasonicBitMark + 368, kPanasonicOneSpace - 495, kPanasonicOneSpace + 504,
                      kPanasonicBitMark - 332, kPanasonicBitMark + 368, kPanasonicZeroSpace - 332, kPanasonicZeroSpace + 300,
                      kPanasonicBitMark - 332, kPanasonicBitMark + 368, kPanasonicAcSectionGap - 2500, kPanasonicAcSectionGap + 2500, true,
                      kPanasonicAcTolerance, kPanasonicAcExcess, false);
  if (!used) return false;
  offset += used;

  // Match Header + Data #2 + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset,
                    results->state + kPanasonicAcSection1Length,
                    results->rawlen - offset,
                    nbits - kPanasonicAcSection1Length * 8,
                    kPanasonicHdrMark - 456, kPanasonicHdrMark + 456, kPanasonicHdrSpace - 400, kPanasonicHdrSpace + 400,
                    kPanasonicBitMark - 332, kPanasonicBitMark + 368, kPanasonicOneSpace - 495, kPanasonicOneSpace + 504,
                    kPanasonicBitMark - 332, kPanasonicBitMark + 368, kPanasonicZeroSpace - 332, kPanasonicZeroSpace + 300,
                    kPanasonicBitMark - 332, kPanasonicBitMark + 368, kPanasonicAcMessageGap - 25000, kPanasonicAcMessageGap + 25000, true,
                    kPanasonicAcTolerance, kPanasonicAcExcess, false))
    return false;
  // Compliance
  if (strict) {
    // Check the signatures of the section blocks. They start with 0x02& 0x20.
    if (results->state[0] != 0x02 || results->state[1] != 0x20 ||
        results->state[8] != 0x02 || results->state[9] != 0x20)
      return false;
    
    os_trace("PANASONIC: 0x ");
    for(uint8_t i = 0; i < nbits/8; i++) {
        os_trace("%02X-", results->state[i]);
    }
    os_trace("\r\n");

    if (IRPanasonicAc_validChecksum(results->state, nbits / 8)){
      results->decode_type = PANASONIC_AC;
    } else if (IRPanasonicAc_validChecksum_2(results->state, nbits / 8)) {
      results->decode_type = PANASONIC_AC_2;
    } else {
      os_trace("%s invalid checksum\r\n", __func__);
      return false;
    }
  }
  os_trace("%s success type %u\r\n", __func__, results->decode_type);

  // Success
  
  results->bits = nbits;
  return true;
}

static bool IRPanasonicAc_getPower(const uint8_t* remote_state) {
  return GETBIT8(remote_state[13], kPanasonicAcPowerOffset);
}

static uint8_t IRPanasonicAc_getMode(const uint8_t* remote_state) {
  return GETBITS8(remote_state[13], kHighNibble, kModeBitsSize);
}

static uint8_t IRPanasonicAc_getTemp(const uint8_t* remote_state) {
  return GETBITS8(remote_state[14], kPanasonicAcTempOffset,
                  kPanasonicAcTempSize);
}

static uint8_t IRPanasonicAc_getFan(const uint8_t* remote_state) {
  return GETBITS8(remote_state[16], kHighNibble, kNibbleSize) -
      kPanasonicAcFanDelta;
}

static panasonic_ac_remote_model_t IRPanasonicAc_getModel(const uint8_t* remote_state) {
  if (remote_state[23] == 0x89) return kPanasonicRkr;
  if (remote_state[17] == 0x00) {
    if ((remote_state[21] & 0x10) && (remote_state[23] & 0x01))
      return kPanasonicCkp;
    if (remote_state[23] & 0x80)
      return kPanasonicJke;
  }
  if (remote_state[17] == 0x06 && (remote_state[13] & 0x0F) == 0x02)
    return kPanasonicLke;
  if (remote_state[23] == 0x01)
    return kPanasonicDke;
  if (remote_state[17] == 0x06)
    return kPanasonicNke;
  return kPanasonicUnknown;  // Default
}

static uint8_t IRPanasonicAc_getSwingVertical(const uint8_t* remote_state) {
  return GETBITS8(remote_state[16], kLowNibble, kNibbleSize);
}

static uint8_t IRPanasonicAc_getSwingHorizontal(const uint8_t* remote_state) {
  return GETBITS8(remote_state[17], kLowNibble, kNibbleSize);
}

static uint8_t IRPanasonicAc_getIon(const uint8_t* remote_state) {
  switch (IRPanasonicAc_getModel(remote_state)) {
    case kPanasonicDke:
      return GETBIT8(remote_state[kPanasonicAcIonFilterByte],
                     kPanasonicAcIonFilterOffset);
    default:
      return false;
  }
}

static uint8_t IRPanasonicAc_calcChecksum(const uint8_t *state,
                                    const uint16_t length) {
  return sumBytesState(state, length - 1, kPanasonicAcChecksumInit);
}

static uint8_t IRPanasonicAc_calcChecksum2(const uint8_t *state,
                                    const uint16_t length) {
  return sumBytesState(state, length - 1, kPanasonicAc2ChecksumInit);
}

/// Calculate and set the checksum values for the internal state.
/// @param[in] length The size/length of the state.
static void IRPanasonicAc_fixChecksum(uint8_t* remote_state, const uint16_t length, int brand) {
  if (brand == PANASONIC_AC) {
    os_trace("old pana\r\n");
    remote_state[length - 1] = IRPanasonicAc_calcChecksum(remote_state, length);
  } else if (brand == PANASONIC_AC_2) {
    os_trace("new pana\r\n");
    remote_state[length - 1] = IRPanasonicAc_calcChecksum2(remote_state, length);
  }
}

static void IRPanasonicAc_setTemp(uint8_t* remote_state, const uint8_t celsius, const bool remember) {
  uint8_t temperature;
  temperature = max(celsius, kPanasonicAcMinTemp);
  temperature = min(temperature, kPanasonicAcMaxTemp);
//   if (remember) _temp = 25;
  setBitsU8(&remote_state[14], kPanasonicAcTempOffset, kPanasonicAcTempSize,
          temperature);
}

void PanasonicAC_setInfo(const uint8_t* newState, state_ir_t *ir_state)
{
    uint8_t remote_state[kPanasonicAcStateLength];
    memcpy(remote_state, newState, kPanasonicAcStateLength);

    ir_state->power = IRPanasonicAc_getPower(remote_state);
    ir_state->mode = IRPanasonicAc_getMode(remote_state);
    ir_state->degrees = IRPanasonicAc_getTemp(remote_state);
    ir_state->fanspeed = IRPanasonicAc_getFan(remote_state);
    ir_state->model = IRPanasonicAc_getModel(remote_state);
    pre_model = ir_state->model;
    ir_state->swingv = IRPanasonicAc_getSwingVertical(remote_state);
    ir_state->swingh = IRPanasonicAc_getSwingHorizontal(remote_state);
    pre_ion_enable = IRPanasonicAc_getIon(remote_state);
}

void sendPanasonicAC(const uint8_t data[], const uint16_t nbytes,
                             const uint16_t repeat) {
  if (nbytes < kPanasonicAcSection1Length) return;
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    // First section. (8 bytes)
    sendGenericBytes(kPanasonicHdrMark, kPanasonicHdrSpace, kPanasonicBitMark,
                kPanasonicOneSpace, kPanasonicBitMark, kPanasonicZeroSpace,
                kPanasonicBitMark, kPanasonicAcSectionGap, data,
                kPanasonicAcSection1Length, kPanasonicFreq, false, 0, 50);
    // First section. (The rest of the data bytes)
    sendGenericBytes(kPanasonicHdrMark, kPanasonicHdrSpace, kPanasonicBitMark,
                kPanasonicOneSpace, kPanasonicBitMark, kPanasonicZeroSpace,
                kPanasonicBitMark, kPanasonicAcMessageGap,
                data + kPanasonicAcSection1Length,
                nbytes - kPanasonicAcSection1Length, kPanasonicFreq, false, 0,
                50);
  }
  vTaskExitCritical();
}

void PanasonicAC_send(state_ir_t state_ir)
{
    os_trace("%s\r\n", __func__);
    uint8_t remote_state[kPanasonicAcStateLength];
    memcpy(remote_state, kPanasonicKnownGoodState, kPanasonicAcStateLength);

    if (pre_model != kPanasonicUnknown) {
      switch (pre_model) {
        case kPanasonicDke:
        case kPanasonicJke:
        case kPanasonicLke:
        case kPanasonicNke:
        case kPanasonicCkp:
        case kPanasonicRkr: break;
        // Only proceed if we know what to do.
        default: break;;
      }
      // clear & set the various bits and bytes.
      remote_state[13] &= 0xF0;
      remote_state[17] = 0x00;
      remote_state[21] &= 0b11101111;
      remote_state[23] = 0x81;
      remote_state[25] = 0x00;

      switch (pre_model) {
        case kPanasonicLke:
          remote_state[13] |= 0x02;
          remote_state[17] = 0x06;
          break;
        case kPanasonicDke:
          remote_state[23] = 0x01;
          remote_state[25] = 0x06;
          // Has to be done last as setSwingHorizontal has model check built-in
          // setSwingHorizontal(_swingh);
          break;
        case kPanasonicNke:
          remote_state[17] = 0x06;
          break;
        case kPanasonicJke:
          break;
        case kPanasonicCkp:
          remote_state[21] |= 0x10;
          remote_state[23] = 0x01;
          break;
        case kPanasonicRkr:
          remote_state[13] |= 0x08;
          remote_state[23] = 0x89;
        default:
          break;
      }
      // Reset the Ion filter.
      // setIon(getIon());
    }

    // set power
    setBitU8(&remote_state[13], kPanasonicAcPowerOffset, state_ir.power);
    // set mode
    uint8_t desired = (uint8_t)state_ir.mode;
    uint8_t mode = kPanasonicAcAuto;  // Default to Auto mode.
    switch (desired) {
        case kPanasonicAcFan:
        // Allegedly Fan mode has a temperature of 27.
        IRPanasonicAc_setTemp(remote_state, kPanasonicAcFanModeTemp, false);
        mode = desired;
        break;
        case kPanasonicAcAuto:
        case kPanasonicAcCool:
        case kPanasonicAcHeat:
        case kPanasonicAcDry:
        mode = desired;
        // Set the temp to the saved temp, just incase our previous mode was Fan.
        IRPanasonicAc_setTemp(remote_state, 25, true);
        // setTemp(_temp);
        break;
    }
    remote_state[13] &= 0x0F;  // Clear the previous mode bits.
    setBitsU8(&remote_state[13], kHighNibble, kModeBitsSize, mode);

    // set fan
    switch ((uint8_t)state_ir.fanspeed) {
        case kPanasonicAcFanMin:
        case kPanasonicAcFanLow:
        case kPanasonicAcFanMed:
        case kPanasonicAcFanHigh:
        case kPanasonicAcFanMax:
        case kPanasonicAcFanAuto:
        setBitsU8(&remote_state[16], kHighNibble, kNibbleSize,
                (uint8_t)state_ir.fanspeed + kPanasonicAcFanDelta);
        break;
        default: setBitsU8(&remote_state[16], kHighNibble, kNibbleSize,
                kPanasonicAcFanAuto + kPanasonicAcFanDelta);
    }
    // set temp
    IRPanasonicAc_setTemp(remote_state, (uint8_t)state_ir.degrees, true);

    if (state_ir.swingv != swingv_kOff) {
      // set swing vertical
      uint8_t elevation = state_ir.swingv;
      if (elevation != kPanasonicAcSwingVAuto) {
        if (elevation > kPanasonicAcSwingVLowest || elevation < kPanasonicAcSwingVHighest) {
          elevation = kPanasonicAcSwingVAuto;
        }
      }
      setBitsU8(&remote_state[16], kLowNibble, kNibbleSize, elevation);
    }

    // set swing horizontal
    if (state_ir.swingh != swingh_kOff) {
      switch ((uint8_t)state_ir.swingh) {
        case kPanasonicAcSwingHAuto:
        case kPanasonicAcSwingHMiddle:
        case kPanasonicAcSwingHFullLeft:
        case kPanasonicAcSwingHLeft:
        case kPanasonicAcSwingHRight:
        case kPanasonicAcSwingHFullRight: 
          break;
        // Ignore anything that isn't valid.
        default: 
          state_ir.swingh = kPanasonicAcSwingHAuto;
          
        break;
      }
      uint8_t direction = (uint8_t)state_ir.swingh;
      switch (IRPanasonicAc_getModel(remote_state)) {
        case kPanasonicDke:
        case kPanasonicRkr:
          setBitsU8(&remote_state[17], kLowNibble, kNibbleSize, direction);
          break;
        case kPanasonicNke:
        case kPanasonicLke:
          direction = kPanasonicAcSwingHMiddle;
          setBitsU8(&remote_state[17], kLowNibble, kNibbleSize, direction);
          break;
        default:  // Ignore everything else.
          break;
      }
    }

    if (pre_ion_enable) {
      setBitU8(&remote_state[kPanasonicAcIonFilterByte],
           kPanasonicAcIonFilterOffset, pre_ion_enable);
    }

    // check sum
    IRPanasonicAc_fixChecksum(remote_state, kPanasonicAcStateLength, state_ir.protocol);
    sendPanasonicAC(remote_state, kPanasonicAcStateLength, kPanasonicAcDefaultRepeat);
}

/************PANASONIC_AC_32*************************************************************************************/

bool decodePanasonicAC32(decode_results *results, uint16_t offset,
                                 const uint16_t nbits, const bool strict) {
  if (strict && (nbits != kPanasonicAc32Bits &&
                 nbits != kPanasonicAc32Bits / 2))
    return false;  // Not strictly a valid bit size.

  // Determine if this is a long or a short message we are looking for.
  const bool is_long = (nbits > kPanasonicAc32Bits / 2);
  const uint16_t min_length = is_long ?
      kPanasonicAc32Sections * kPanasonicAc32BlocksPerSection *
      ((2 * nbits) + kHeader + kFooter) - 1 + offset :
      (kPanasonicAc32BlocksPerSection + 1) * ((4 * nbits) + kHeader) +
      kFooter - 1 + offset;

  if (results->rawlen < min_length)
    return false;  // Can't possibly be a valid message.

  // Calculate the parameters for the decode based on it's length.
  uint16_t sections;
  uint16_t blocks_per_section;
  if (is_long) {
    sections = kPanasonicAc32Sections;
    blocks_per_section = kPanasonicAc32BlocksPerSection;
  } else {
    sections = kPanasonicAc32Sections - 1;
    blocks_per_section = kPanasonicAc32BlocksPerSection + 1;
  }
  const uint16_t bits_per_block = nbits / sections;

  uint64_t data = 0;
  uint64_t section_data = 0;
  uint32_t prev_section_data;

  // Match all the expected data blocks.
  for (uint16_t block = 0;
       block < sections * blocks_per_section;
       block++) {
    prev_section_data = section_data;
    uint16_t used = matchGenericDataLimit(results->rawbuf + offset, &section_data,
                                 results->rawlen - offset, bits_per_block * 2,
                                 kPanasonicAc32HdrMark-543,kPanasonicAc32HdrMark+543, kPanasonicAc32HdrSpace-450, kPanasonicAc32HdrSpace+450,
                                 kPanasonicAc32BitMark-500, kPanasonicAc32BitMark+500, kPanasonicAc32OneSpace-575,kPanasonicAc32OneSpace+625,
                                 kPanasonicAc32BitMark-500, kPanasonicAc32BitMark+500, kPanasonicAc32ZeroSpace-500, kPanasonicAc32ZeroSpace+500,
                                 0, 0, 0, 0,  // No Footer
                                 false, kUseDefTol, kMarkExcess, false);
    if (!used) return false;
    offset += used;
    // Is it the first block of the section?
    if (block % blocks_per_section == 0) {
      // The protocol repeats each byte twice, so to shrink the code we
      // remove the duplicate bytes in the collected data. We only need to do
      // this for the first block in a section.
      uint64_t shrunk_data = 0;
      uint64_t data_copy = section_data;
      for (uint8_t i = 0; i < sizeof(data_copy); i += 2) {
        const uint8_t first_byte = GETBITS64(data_copy,
                                             (sizeof(data_copy) - 1) * 8, 8);
        shrunk_data = (shrunk_data << 8) | first_byte;
        // Compliance
        if (strict) {
          // Every second byte must be a duplicate of the previous.
          const uint8_t next_byte = GETBITS64(data_copy,
                                              (sizeof(data_copy) - 2) * 8, 8);
          if (first_byte != next_byte) return false;
        }
        data_copy <<= 16;
      }
      // Keep the data from the first of the block in the section.
      data = (data << bits_per_block) | shrunk_data;
    } else {  // Not the first block in a section.
      // Compliance
      if (strict)
        // Compare the data from the blocks in pairs.
        if (section_data != prev_section_data) return false;
      // Look for the section footer at the end of the blocks.
      if ((block + 1) % blocks_per_section == 0) {
        uint64_t junk;
        used = matchGenericDataLimit(results->rawbuf + offset, &junk,
                            results->rawlen - offset, 0,
                            // Header
                            kPanasonicAc32HdrMark-543, kPanasonicAc32HdrMark+543, kPanasonicAc32HdrSpace-600, kPanasonicAc32HdrSpace+600,
                            // No Data
                            0, 0, 0, 0,
                            0, 0, 0, 0,
                            // Footer
                            kPanasonicAc32BitMark - 500, kPanasonicAc32BitMark+500, kPanasonicAc32SectionGap-3500, kPanasonicAc32SectionGap+3500,
                            true, kUseDefTol, kMarkExcess, true);
        if (!used) return false;
        offset += used;
      }
    }
  }

  os_trace("%s success\r\n", __func__);

  // Success
  results->value = data;
  results->decode_type = PANASONIC_AC32;
  results->bits = nbits;
  results->address = 0;
  results->command = 0;
  return true;
}

static uint8_t ac32_prev_power = true;

void IRPanasonicAc32_setInfo(const uint32_t state,  state_ir_t *ir_state) {  
    PanasonicAc32Protocol _;
    _.raw = state; 
    if (!_.PowerToggle) {
        ac32_prev_power = !ac32_prev_power;
    }
    ir_state->power = ac32_prev_power;
    ir_state->mode = _.Mode;
    ir_state->degrees = _.Temp + (kPanasonicAcMinTemp - 1);
    ir_state->fanspeed = _.Fan;
    ir_state->swingv = _.SwingV;
    ir_state->swingh = _.SwingH;
}

static void sendPanasonicAC32(const uint64_t data, const uint16_t nbits,
                               const uint16_t repeat) {
  uint16_t section_bits;
  uint16_t sections;
  uint16_t blocks;
  // Calculate the section, block, and bit sizes based on the requested bit size
  if (nbits > kPanasonicAc32Bits / 2) {  // A long message
    section_bits = nbits / kPanasonicAc32Sections;
    sections = kPanasonicAc32Sections;
    blocks = kPanasonicAc32BlocksPerSection;
  } else {  // A short message
    section_bits = nbits;
    sections = kPanasonicAc32Sections - 1;
    blocks = kPanasonicAc32BlocksPerSection + 1;
  }
  vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    for (uint8_t section = 0; section < sections;  section++) {
      uint64_t section_data;
      section_data = GETBITS64(data, section_bits * (sections - section - 1),
                               section_bits);

      // Duplicate bytes in the data.
      uint64_t expanded_data = 0;
      for (uint8_t i = 0; i < sizeof(expanded_data); i++) {
       const uint8_t first_byte = section_data >> 56;
       for (uint8_t i = 0; i < 2; i++)
         expanded_data = (expanded_data << 8) | first_byte;
       section_data <<= 8;
      }
      // Two data blocks per section (i.e. 1 + a repeat)
      sendGeneric(kPanasonicAc32HdrMark, kPanasonicAc32HdrSpace,  // Header
                  kPanasonicAc32BitMark, kPanasonicAc32OneSpace,  // Data
                  kPanasonicAc32BitMark, kPanasonicAc32ZeroSpace,
                  0, 0,  // No Footer
                  expanded_data, section_bits * 2, kPanasonicFreq, false,
                  blocks - 1,  // Repeat
                  50);
      // Section Footer
      sendGeneric(kPanasonicAc32HdrMark, kPanasonicAc32HdrSpace,  // Header
                  0, 0, 0, 0,  // No Data
                  kPanasonicAc32BitMark, kPanasonicAc32SectionGap,  // Footer
                  data, 0,  // No data (bits)
                  kPanasonicFreq, true, 0, 50);
    }
  }
  vTaskExitCritical();
}

void PanasonicAc32_send(state_ir_t state_ir)
{
    PanasonicAc32Protocol _;
    _.raw = kPanasonicAc32KnownGood;
    // Power
    if (state_ir.power != ac32_prev_power) {
        _.PowerToggle = false;
    }
    // Mode
    switch ((uint8_t)state_ir.mode)
    {
        case kPanasonicAc32Auto:
        case kPanasonicAc32Cool:
        case kPanasonicAc32Dry:
        case kPanasonicAc32Heat:
        case kPanasonicAc32Fan:
        _.Mode = state_ir.mode;
        break;
        default: _.Mode = kPanasonicAc32Auto;
    }
    // Temp
    uint8_t temp = max((uint8_t)kPanasonicAcMinTemp, ((uint8_t)state_ir.degrees));
    temp = min((uint8_t)kPanasonicAcMaxTemp, temp);
    _.Temp = temp - (kPanasonicAcMinTemp - 1);
    // fanspeed
    switch ((uint8_t)state_ir.fanspeed) {
      case kPanasonicAc32FanMin:
      case kPanasonicAc32FanLow:
      case kPanasonicAc32FanMed:
      case kPanasonicAc32FanHigh:
      case kPanasonicAc32FanMax:
      case kPanasonicAc32FanAuto:
        _.Fan = state_ir.fanspeed;
        break;
      default: _.Fan = kPanasonicAc32FanAuto;
    }

  // swing vertical
    uint8_t elevation = (uint8_t)state_ir.swingv;
    if (elevation != kPanasonicAc32SwingVAuto) {
      if (elevation > kPanasonicAcSwingVLowest || elevation < kPanasonicAcSwingVHighest) {
        elevation = kPanasonicAc32SwingVAuto;
      }
    }
    _.SwingV = elevation;

  // swing horizontal
  switch ((uint8_t)state_ir.swingh)
  {
  case 0:
    _.SwingH = false;
    break;
  
  case 1:
    _.SwingH = true;
    break;
  
  default:
    break;
  }

  sendPanasonicAC32(_.raw, kPanasonicAc32Bits, kPanasonicAc32DefaultRepeat);
}


