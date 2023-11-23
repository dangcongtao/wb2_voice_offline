#include "ir_NEC.h"
#include "system_typedef.h"

bool decodeNEC(decode_results *results, uint16_t offset,
                       const uint16_t nbits, const bool strict) {
  if (results->rawlen < kNecRptLength + offset - 1){
    os_trace("Can't possibly be a valid NEC message.\r\n");
    return false;  // Can't possibly be a valid NEC message.
  }
  if (strict && nbits != kNECBits){
    os_trace("Not strictly an NEC message.\r\n");
    return false;  // Not strictly an NEC message.
  }

  uint64_t data = 0;

  // Header - All NEC messages have this Header Mark.
  if (!matchMark(results->rawbuf[offset++], kNecHdrMark, kUseDefTol, kMarkExcess)){
    os_trace("NEC not match mark 1\r\n");
    return false;
  }
  // Check if it is a repeat code.
  if (matchSpace(results->rawbuf[offset], kNecRptSpace, kUseDefTol, kMarkExcess) &&
      matchMark(results->rawbuf[offset + 1], kNecBitMark, kUseDefTol, kMarkExcess) &&
      (offset + 2 <= results->rawlen ||
       matchAtLeast(results->rawbuf[offset + 2], kNecMinGap, kUseDefTol, 0))) {
    results->value = kRepeat;
    results->decode_type = NEC;
    results->bits = 0;
    results->address = 0;
    results->command = 0;
    results->repeat = true;
    os_trace("NEC it is a repeat code\r\n");
    return true;
  }

  // Match Header (cont.) + Data + Footer
  if (!matchGeneric(results->rawbuf + offset, &data,
                    results->rawlen - offset, nbits,
                    0, kNecHdrSpace,
                    kNecBitMark, kNecOneSpace,
                    kNecBitMark, kNecZeroSpace,
                    kNecBitMark, kNecMinGap, true, kUseDefTol, kMarkExcess, true)){
                        os_trace("NEC not Match Header\r\n");
                        return false;
                    }
  // Compliance
  // Calculate command and optionally enforce integrity checking.
  uint8_t command = (data & 0xFF00) >> 8;
  // Command is sent twice, once as plain and then inverted.
  if ((command ^ 0xFF) != (data & 0xFF)) {
    if (strict){ 
        os_trace("NEC Command integrity failed\r\n");
        return false;  // Command integrity failed.
    }
    command = 0;  // The command value isn't valid, so default to zero.
  }

  // Success
  results->bits = nbits;
  results->value = data;
  results->decode_type = NEC;
  // NEC command and address are technically in LSB first order so the
  // final versions have to be reversed.
  results->command = reverseBits(command, 8);
  // Normal NEC protocol has an 8 bit address sent, followed by it inverted.
  uint8_t address = (data & 0xFF000000) >> 24;
  uint8_t address_inverted = (data & 0x00FF0000) >> 16;
  if (address == (address_inverted ^ 0xFF))
    // Inverted, so it is normal NEC protocol.
    results->address = reverseBits(address, 8);
  else  // Not inverted, so must be Extended NEC protocol, thus 16 bit address.
    results->address = reverseBits((data >> 16) & UINT16_MAX, 16);

    os_trace("NEC success\r\n");
  return true;
}