#ifndef IRTIMER_H_
#define IRTIMER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct 
{
    uint32_t start;
} IRtimer_t;

void reset(IRtimer_t *obj);
uint32_t elapsed(IRtimer_t *obj);


#endif