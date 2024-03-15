#include "ir_Kelvinator.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"


// Constants
const uint16_t kKelvinatorTick = 85;
const uint16_t kKelvinatorHdrMarkTicks = 106;
const uint16_t kKelvinatorHdrMark = kKelvinatorHdrMarkTicks * kKelvinatorTick;
const uint16_t kKelvinatorHdrSpaceTicks = 53;
const uint16_t kKelvinatorHdrSpace = kKelvinatorHdrSpaceTicks * kKelvinatorTick;
const uint16_t kKelvinatorBitMarkTicks = 8;
const uint16_t kKelvinatorBitMark = kKelvinatorBitMarkTicks * kKelvinatorTick;
const uint16_t kKelvinatorOneSpaceTicks = 18;
const uint16_t kKelvinatorOneSpace = kKelvinatorOneSpaceTicks * kKelvinatorTick;
const uint16_t kKelvinatorZeroSpaceTicks = 6;
const uint16_t kKelvinatorZeroSpace =
    kKelvinatorZeroSpaceTicks * kKelvinatorTick;
const uint16_t kKelvinatorGapSpaceTicks = 235;
const uint16_t kKelvinatorGapSpace = kKelvinatorGapSpaceTicks * kKelvinatorTick;

const uint8_t kKelvinatorCmdFooter = 2;
const uint8_t kKelvinatorCmdFooterBits = 3;

const uint8_t kKelvinatorChecksumStart = 10;


uint8_t IRKelvinatorAC_calcBlockChecksum(const uint8_t *block,
                                          const uint16_t length) {
  uint8_t sum = kKelvinatorChecksumStart;
  // Sum the lower half of the first 4 bytes of this block.
  for (uint8_t i = 0; i < 4 && i < length - 1; i++, block++)
    sum += (*block & 0b1111);
  // then sum the upper half of the next 3 bytes.
  for (uint8_t i = 4; i < length - 1; i++, block++) sum += (*block >> 4);
  // Trim it down to fit into the 4 bits allowed. i.e. Mod 16.
  return sum & 0b1111;
}

static bool IRKelvinatorAC_validChecksum(const uint8_t state[],
                                   const uint16_t length) {
  for (uint16_t offset = 0; offset + 7 < length; offset += 8) {
    // Top 4 bits of the last byte in the block is the block's checksum.
    if (GETBITS8(state[offset + 7], kHighNibble, kNibbleSize) !=
        IRKelvinatorAC_calcBlockChecksum(state + offset, kKelvinatorStateLength / 2))
      return false;
  }
  return true;
}

bool decodeKelvinator(decode_results *results, uint16_t offset,
                              const uint16_t nbits, const bool strict) {
  if (results->rawlen <=
      2 * (nbits + kKelvinatorCmdFooterBits) + (kHeader + kFooter + 1) * 2 - 1 +
      offset)
    return false;  // Can't possibly be a valid Kelvinator message.
  if (strict && nbits != kKelvinatorBits)
    return false;  // Not strictly a Kelvinator message.

  // There are two messages back-to-back in a full Kelvinator IR message
  // sequence.
  int8_t pos = 0;
  for (uint8_t s = 0; s < 2; s++) {
    match_result_t data_result;

    uint16_t used;
    // Header + Data Block #1 (32 bits)
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, 32,
                        kKelvinatorHdrMark-500,kKelvinatorHdrMark+500, kKelvinatorHdrSpace-400, kKelvinatorHdrSpace+400,
                        kKelvinatorBitMark-133, kKelvinatorBitMark+232, kKelvinatorOneSpace-450, kKelvinatorOneSpace+450,
                        kKelvinatorBitMark-133, kKelvinatorBitMark+232, kKelvinatorZeroSpace-410, kKelvinatorZeroSpace+410,
                        0, 0, 0, 0, false,
                        _tolerance, kMarkExcess, false);
    if (used == 0) return false;
    offset += used;
    pos += 4;

    // Command data footer (3 bits, B010)
    data_result = matchDataLimit(
        &(results->rawbuf[offset]), kKelvinatorCmdFooterBits,
        kKelvinatorBitMark-450, kKelvinatorBitMark+450, kKelvinatorOneSpace-450, kKelvinatorOneSpace+450,
        kKelvinatorBitMark-450, kKelvinatorBitMark+450, kKelvinatorZeroSpace-410, kKelvinatorZeroSpace+410,
        _tolerance, kMarkExcess, false, true);
    if (data_result.success == false) return false;
    if (data_result.data != kKelvinatorCmdFooter) return false;
    offset += data_result.used;

    // Gap + Data (Options) (32 bits)
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, 32,
                        kKelvinatorBitMark-133, kKelvinatorBitMark+232, kKelvinatorGapSpace-5000, kKelvinatorGapSpace+5000,
                        kKelvinatorBitMark-133, kKelvinatorBitMark+232, kKelvinatorOneSpace-420, kKelvinatorOneSpace+420,
                        kKelvinatorBitMark-133, kKelvinatorBitMark+232, kKelvinatorZeroSpace-410, kKelvinatorZeroSpace+410,
                        kKelvinatorBitMark-133, kKelvinatorBitMark+232, kKelvinatorGapSpace * 2-10000,kKelvinatorGapSpace * 2+10000,
                        s > 0,
                        _tolerance, kMarkExcess, false);
    if (used == 0) return false;
    offset += used;
    pos += 4;
  }

  // Compliance
  if (strict) {
    // Verify the message's checksum is correct.
    if (!IRKelvinatorAC_validChecksum(results->state, kKelvinatorStateLength)){
        os_trace("%s verify checksum is incorrect\r\n", __func__);
        return false;
    }
  }

    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = KELVINATOR;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRKelvinatorAC_setInfo(const uint8_t new_code[], state_ir_t *ir_state) {
    KelvinatorProtocol _;
    memset(&_, 0, sizeof(KelvinatorProtocol));
    memcpy(_.raw, new_code, kKelvinatorStateLength);

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kKelvinatorMinTemp;
}

static void sendKelvinator(const unsigned char data[], const uint16_t nbytes,
                            const uint16_t repeat) {
  if (nbytes < kKelvinatorStateLength)
    return;  // Not enough bytes to send a proper message.
    vTaskEnterCritical();
  for (uint16_t r = 0; r <= repeat; r++) {
    // Command Block #1 (4 bytes)
    sendGenericBytes(kKelvinatorHdrMark, kKelvinatorHdrSpace, kKelvinatorBitMark,
                kKelvinatorOneSpace, kKelvinatorBitMark, kKelvinatorZeroSpace,
                0, 0,  // No Footer yet.
                data, 4, 38, false, 0, 50);
    // Send Footer for the command block (3 bits (b010))
    sendGeneric(0, 0,  // No Header
                kKelvinatorBitMark, kKelvinatorOneSpace, kKelvinatorBitMark,
                kKelvinatorZeroSpace, kKelvinatorBitMark, kKelvinatorGapSpace,
                kKelvinatorCmdFooter, kKelvinatorCmdFooterBits, 38, false, 0,
                50);
    // Data Block #1 (4 bytes)
    sendGenericBytes(0, 0,  // No header
                kKelvinatorBitMark, kKelvinatorOneSpace, kKelvinatorBitMark,
                kKelvinatorZeroSpace, kKelvinatorBitMark,
                kKelvinatorGapSpace * 2, data + 4, 4, 38, false, 0, 50);
    // Command Block #2 (4 bytes)
    sendGenericBytes(kKelvinatorHdrMark, kKelvinatorHdrSpace, kKelvinatorBitMark,
                kKelvinatorOneSpace, kKelvinatorBitMark, kKelvinatorZeroSpace,
                0, 0,  // No Footer yet.
                data + 8, 4, 38, false, 0, 50);
    // Send Footer for the command block (3 bits (B010))
    sendGeneric(0, 0,  // No Header
                kKelvinatorBitMark, kKelvinatorOneSpace, kKelvinatorBitMark,
                kKelvinatorZeroSpace, kKelvinatorBitMark, kKelvinatorGapSpace,
                kKelvinatorCmdFooter, kKelvinatorCmdFooterBits, 38, false, 0,
                50);
    // Data Block #2 (4 bytes)
    sendGenericBytes(0, 0,  // No header
                kKelvinatorBitMark, kKelvinatorOneSpace, kKelvinatorBitMark,
                kKelvinatorZeroSpace, kKelvinatorBitMark,
                kKelvinatorGapSpace * 2, data + 12, 4, 38, false, 0, 50);
  }
  vTaskExitCritical();
}

void KelvinatorAC_send(state_ir_t state_ir)
{
    KelvinatorProtocol _;
    memset(&_, 0, sizeof(KelvinatorProtocol));
    for (uint8_t i = 0; i < kKelvinatorStateLength; i++) _.raw[i] = 0x0;

    _.raw[3] = 0x50;
    _.raw[11] = 0x70;

    _.Power = state_ir.power;

    uint8_t temp = max(kKelvinatorMinTemp, (uint8_t)state_ir.degrees);
    temp = min(kKelvinatorMaxTemp, temp);
    _.Temp = temp - kKelvinatorMinTemp;

    switch ((uint8_t)state_ir.mode) {
    case kKelvinatorAuto:
    case kKelvinatorDry:
      // When the remote is set to Auto or Dry, it defaults to 25C and doesn't
      // show it.
    //   setTemp(kKelvinatorAutoTemp);
        _.Temp = 9;
      // FALL-THRU
    case kKelvinatorHeat:
    case kKelvinatorCool:
    case kKelvinatorFan:
      _.Mode = state_ir.mode;
      break;
    default:
    //   setTemp(kKelvinatorAutoTemp);
      _.Temp = 9;
      _.Mode = kKelvinatorAuto;
      break;
  }

  uint8_t fan = min(kKelvinatorFanMax, (uint8_t)state_ir.fanspeed);  // Bounds check

  // Only change things if we need to.
  if (fan != _.Fan) {
    // Set the basic fan values.
    _.BasicFan = min(kKelvinatorBasicFanMax, fan);
    // Set the advanced(?) fan value.
    _.Fan = fan;
    // Turbo mode is turned off if we change the fan settings.
    // setTurbo(false);
    _.Turbo = false;
  }

  if (_.Mode != kKelvinatorCool && _.Mode != kKelvinatorDry)
    _.XFan = false;
  // Duplicate to the 2nd command chunk.
  _.raw[8] = _.raw[0];
  _.raw[9] = _.raw[1];
  _.raw[10] = _.raw[2];
//   checksum();  // Calculate the checksums

  _.Sum1 = IRKelvinatorAC_calcBlockChecksum(_.raw, kKelvinatorStateLength / 2);
  _.Sum2 = IRKelvinatorAC_calcBlockChecksum(_.raw + 8, kKelvinatorStateLength / 2);
  sendKelvinator(_.raw, kKelvinatorStateLength, kKelvinatorDefaultRepeat);
}

