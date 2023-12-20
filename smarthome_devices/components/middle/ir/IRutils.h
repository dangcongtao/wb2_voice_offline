#ifndef __IRUTILS_H_
#define __IRUTILS_H_

#include "IRrecv.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define kNibbleSize  4
#define kLowNibble  0
#define kHighNibble  4
#define kModeBitsSize  3

#define GETBITS8(data, offset, size) \
    (((data) & (((uint8_t)UINT8_MAX >> (8 - (size))) << (offset))) >> (offset))
#define GETBITS16(data, offset, size) \
    (((data) & (((uint16_t)UINT16_MAX >> (16 - (size))) << (offset))) >> \
     (offset))
#define GETBITS32(data, offset, size) \
    (((data) & (((uint32_t)UINT32_MAX >> (32 - (size))) << (offset))) >> \
     (offset))
#define GETBITS64(data, offset, size) \
    (((data) & (((uint64_t)UINT64_MAX >> (64 - (size))) << (offset))) >> \
     (offset))

bool hasACState(const decode_type_t protocol);
uint8_t sumNibbles(const uint64_t data, const uint8_t count,
                    const uint8_t init, const bool nibbleonly);
uint8_t sumNibblesByte(const uint8_t * const start, const uint16_t length,
                     const uint8_t init);
uint64_t reverseBits(uint64_t input, uint16_t nbits);
void resultToHexidecimal(const decode_results * const result, char *output);

uint16_t countBits(uint8_t * start, uint16_t length,
                   bool ones, uint16_t init);
uint16_t countBits64(const uint64_t data, const uint8_t length, const bool ones,
                   const uint16_t init);

uint8_t xorBytes(const uint8_t * const start, const uint16_t length,
                 const uint8_t init);
bool checkInvertedBytePairs(uint8_t *ptr,
                              const uint16_t length);
uint8_t * invertBytePairs(uint8_t *ptr, const uint16_t length);

uint8_t sumBytesState(const uint8_t * const start, const uint16_t length,
                 const uint8_t init);
uint16_t sumBytesData(const uint64_t data, const uint8_t count,
                  const uint8_t init, const bool byteonly);
float celsiusToFahrenheit(const float deg);
float fahrenheitToCelsius(const float deg);

uint8_t bcdToUint8(const uint8_t bcd);
uint8_t uint8ToBcd(const uint8_t integer);

#endif