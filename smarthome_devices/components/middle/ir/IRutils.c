#include "IRutils.h"
#include "hardware.h"

bool hasACState(const decode_type_t protocol) 
{
  switch (protocol) {
    // This is kept sorted by name
    // case AMCOR:
    // case ARGO:
    // case BOSCH144:
    // case CARRIER_AC84:
    // case CARRIER_AC128:
    // case CORONA_AC:
    // case DAIKIN:
    // case DAIKIN128:
    // case DAIKIN152:
    // case DAIKIN160:
    // case DAIKIN176:
    // case DAIKIN2:
    // case DAIKIN200:
    // case DAIKIN216:
    // case DAIKIN312:
    // case ELECTRA_AC:
    // case FUJITSU_AC:
    // case GREE:
    // case HAIER_AC:
    // case HAIER_AC_YRW02:
    // case HAIER_AC160:
    // case HAIER_AC176:
    // case HITACHI_AC:
    // case HITACHI_AC1:
    // case HITACHI_AC2:
    // case HITACHI_AC3:
    // // case HITACHI_AC264:
    // // case HITACHI_AC296:
    // case HITACHI_AC344:
    // case HITACHI_AC424:
    // // case KELON168:
    // case KELVINATOR:
    // case MIRAGE:
    // case MITSUBISHI136:
    // case MITSUBISHI112:
    // case MITSUBISHI_AC:
    // case MITSUBISHI_HEAVY_88:
    // case MITSUBISHI_HEAVY_152:
    // case MWM:
    // case NEOCLIMA:
    // case PANASONIC_AC:
    // case RHOSS:
    // case SAMSUNG_AC:
    // case SANYO_AC:
    // case SANYO_AC88:
    // case SANYO_AC152:
    // case SHARP_AC:
    // case TCL96AC:
    // case TCL112AC:
    // case TEKNOPOINT:
    // case TOSHIBA_AC:
    // case TROTEC:
    // case TROTEC_3550:
    // case VOLTAS:
    // case WHIRLPOOL_AC:
    // case YORK:
    //   return true;
    default:
      return false;
  }
}

uint16_t countBits(uint8_t *start, const uint16_t length,
                   const bool ones, const uint16_t init) {
  uint16_t count = init;
  for (uint16_t offset = 0; offset < length; offset++)
    for (uint8_t currentbyte = *(start + offset);
         currentbyte;
         currentbyte >>= 1)
      if (currentbyte & 1) count++;
  if (ones || length == 0)
    return count;
  else
    return (length * 8) - count;
}

uint16_t countBits64(const uint64_t data, const uint8_t length, const bool ones,
                   const uint16_t init) 
{
  uint16_t count = init;
  uint8_t bitsSoFar = length;
  for (uint64_t remainder = data; remainder && bitsSoFar;
       remainder >>= 1, bitsSoFar--)
      if (remainder & 1) count++;
  if (ones || length == 0)
    return count;
  else
    return length - count;
}

uint64_t reverseBits(uint64_t input, uint16_t nbits) {
  if (nbits <= 1) return input;  // Reversing <= 1 bits makes no change at all.
  // Cap the nr. of bits to rotate to the max nr. of bits in the input.
  nbits = min(nbits, (uint16_t)(sizeof(input) * 8));
  uint64_t output = 0;
  for (uint16_t i = 0; i < nbits; i++) {
    output <<= 1;
    output |= (input & 1);
    input >>= 1;
  }
  // Merge any remaining unreversed bits back to the top of the reversed bits.
  return (input << nbits) | output;
}

uint8_t sumNibbles(const uint64_t data, const uint8_t count,
                    const uint8_t init, const bool nibbleonly) {
  uint8_t sum = init;
  uint64_t copy = data;
  const uint8_t nrofnibbles = (count < 16) ? count : (64 / 4);
  for (uint8_t i = 0; i < nrofnibbles; i++, copy >>= 4) sum += copy & 0xF;
  return nibbleonly ? sum & 0xF : sum;
}

uint8_t sumNibblesByte(const uint8_t * const start, const uint16_t length,
                     const uint8_t init) {
  uint8_t sum = init;
  const uint8_t *ptr;
  for (ptr = start; ptr - start < length; ptr++)
    sum += (*ptr >> 4) + (*ptr & 0xF);
  return sum;
}

uint8_t xorBytes(const uint8_t * const start, const uint16_t length,
                 const uint8_t init) {
  uint8_t checksum = init;
  const uint8_t *ptr;
  for (ptr = start; ptr - start < length; ptr++) checksum ^= *ptr;
  return checksum;
}

bool checkInvertedBytePairs(uint8_t *ptr,
                              const uint16_t length) 
{
  for (uint16_t i = 1; i < length; i += 2) {
    // Code done this way to avoid a compiler warning bug.
    uint8_t inv = ~*(ptr + i - 1);
    if (*(ptr + i) != inv) return false;
  }
  return true;
}

uint8_t * invertBytePairs(uint8_t *ptr, const uint16_t length)
{
  for (uint16_t i = 1; i < length; i += 2) {
    // Code done this way to avoid a compiler warning bug.
    uint8_t inv = ~*(ptr + i - 1);
    *(ptr + i) = inv;
  }
  return ptr;
}

uint8_t sumBytesState(const uint8_t * const start, const uint16_t length,
                 const uint8_t init) 
{
  uint8_t checksum = init;
  const uint8_t *ptr;
  for (ptr = start; ptr - start < length; ptr++) checksum += *ptr;
  return checksum;
}

uint16_t sumBytesData(const uint64_t data, const uint8_t count,
                  const uint8_t init, const bool byteonly) 
{
  uint16_t sum = init;
  uint64_t copy = data;
  const uint8_t nrofbytes = (count < 8) ? count : (64 / 8);
  for (uint8_t i = 0; i < nrofbytes; i++, copy >>= 8) sum += (copy & 0xFF);
  return byteonly ? sum & 0xFF : sum;
}

float celsiusToFahrenheit(const float deg) { return (deg * 9.0) / 5.0 + 32.0; }

float fahrenheitToCelsius(const float deg) { return (deg - 32.0) * 5.0 / 9.0; }

uint8_t bcdToUint8(const uint8_t bcd) {
  if (bcd > 0x99) return 255;  // Too big.
  return (bcd >> 4) * 10 + (bcd & 0xF);
}

uint8_t uint8ToBcd(const uint8_t integer)
{
  if (integer > 99) return 255;  // Too big.
  return ((integer / 10) << 4) + (integer % 10);
}

uint16_t getCorrectedRawLength(const decode_results * const results) {
  uint16_t extended_length = results->rawlen - 1;
  for (uint16_t i = 0; i < results->rawlen - 1; i++) {
    uint32_t usecs = ((uint32_t)results->rawbuf[i]) * kRawTick;
    // Add two extra entries for multiple larger than UINT16_MAX it is.
    extended_length += (usecs / (UINT16_MAX + 1)) * 2;
  }
  return extended_length;
}

void resultToRawArray(const decode_results * const decode, uint16_t* result) 
{
  
    // Convert the decode data.
    uint16_t pos = 0;
    for (uint16_t i = 1; i < decode->rawlen; i++) {
      uint32_t usecs = ((uint32_t)decode->rawbuf[i]) * kRawTick;
      while (usecs > UINT16_MAX) {  // Keep truncating till it fits.
        result[pos++] = UINT16_MAX;
        result[pos++] = 0;  // A 0 in a sendRaw() array basically means skip.
        usecs -= UINT16_MAX;
      }
      result[pos++] = (uint16_t)usecs;
    }
}

// void resultToHexidecimal(const decode_results * const result, char *output) 
// {
//   strcpy(output, "0x");
//   if (hasACState(result->decode_type)) {
// #if DECODE_AC
//     for (uint16_t i = 0; result->bits > i * 8; i++) {
//       if (result->state[i] < 0x10) strcat(output, "0");
//       char temp[17];
//       sprintf(temp, "%llx", result->state[i]);
//       strcat(output, temp);
//     }
// #endif  
//   } else {
//     char temp[17];
//     sprintf(temp, "%llx", result->value);
//     strcat(output, temp);
//   }
// }

void setBitU8(uint8_t * const data, const uint8_t position, const bool on) {
    uint8_t mask = 1 << position;
    if (on)
      *data |= mask;
    else
      *data &= ~mask;
  }

  /// Alter the value of an Integer with the `position`th bit changed.
  /// @param[in,out] data A pointer to the 32-bit integer to be changed.
  /// @param[in] position Nr. of the bit to be changed. `0` is the LSB.
  /// @param[in] on Value to set the position'th bit to.
  void setBitU32(uint32_t * const data, const uint8_t position, const bool on) {
    uint32_t mask = (uint32_t)1 << position;
    if (on)
      *data |= mask;
    else
      *data &= ~mask;
  }

  /// Alter the value of an Integer with the `position`th bit changed.
  /// @param[in,out] data A pointer to the 64-bit integer to be changed.
  /// @param[in] position Nr. of the bit to be changed. `0` is the LSB.
  /// @param[in] on Value to set the position'th bit to.
  void setBitU64(uint64_t * const data, const uint8_t position, const bool on) {
    uint64_t mask = (uint64_t)1 << position;
    if (on)
      *data |= mask;
    else
      *data &= ~mask;
  }

    void setBitsU8(uint8_t * const dst, const uint8_t offset, const uint8_t nbits,
               const uint8_t data) {
    if (offset >= 8 || !nbits) return;  // Short circuit as it won't change.
    // Calculate the mask for the supplied value.
    uint8_t mask = UINT8_MAX >> (8 - ((nbits > 8) ? 8 : nbits));
    // Calculate the mask & clear the space for the data.
    // Clear the destination bits.
    *dst &= ~(uint8_t)(mask << offset);
    // Merge in the data.
    *dst |= ((data & mask) << offset);
  }

  /// Alter an uint32_t value by overwriting an arbitrary given number of bits.
  /// @param[in,out] dst A pointer to the value to be changed.
  /// @param[in] offset Nr. of bits from the Least Significant Bit to be ignored
  /// @param[in] nbits Nr of bits of data to be placed into the destination.
  /// @param[in] data The value to be placed.
  void setBitsU32(uint32_t * const dst, const uint8_t offset, const uint8_t nbits,
               const uint32_t data) {
    if (offset >= 32 || !nbits) return;  // Short circuit as it won't change.
    // Calculate the mask for the supplied value.
    uint32_t mask = UINT32_MAX >> (32 - ((nbits > 32) ? 32 : nbits));
    // Calculate the mask & clear the space for the data.
    // Clear the destination bits.
    *dst &= ~(mask << offset);
    // Merge in the data.
    *dst |= ((data & mask) << offset);
  }

  /// Alter an uint64_t value by overwriting an arbitrary given number of bits.
  /// @param[in,out] dst A pointer to the value to be changed.
  /// @param[in] offset Nr. of bits from the Least Significant Bit to be ignored
  /// @param[in] nbits Nr of bits of data to be placed into the destination.
  /// @param[in] data The value to be placed.
  void setBitsU64(uint64_t * const dst, const uint8_t offset, const uint8_t nbits,
               const uint64_t data) {
    if (offset >= 64 || !nbits) return;  // Short circuit as it won't change.
    // Calculate the mask for the supplied value.
    uint64_t mask = UINT64_MAX >> (64 - ((nbits > 64) ? 64 : nbits));
    // Calculate the mask & clear the space for the data.
    // Clear the destination bits.
    *dst &= ~(mask << offset);
    // Merge in the data.
    *dst |= ((data & mask) << offset);
  }


uint64_t invertBits(const uint64_t data, const uint16_t nbits) {
  // No change if we are asked to invert no bits.
  if (nbits == 0) return data;
  uint64_t result = ~data;
  // If we are asked to invert all the bits or more than we have, it's simple.
  if (nbits >= sizeof(data) * 8) return result;
  // Mask off any unwanted bits and return the result.
  return (result & ((1ULL << nbits) - 1));
}
