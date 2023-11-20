#ifndef __IRUTILS_H_
#define __IRUTILS_H_

#include "IRrecv.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

bool hasACState(const decode_type_t protocol);
uint8_t sumNibbles(const uint64_t data, const uint8_t count,
                    const uint8_t init, const bool nibbleonly);
uint64_t reverseBits(uint64_t input, uint16_t nbits);
void resultToHexidecimal(const decode_results * const result, char *output);

#endif