#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "security.h"

static uint8_t get_index_conv_by_tid(uint8_t tid, uint8_t *index_1, uint8_t *index_2)
{
    *index_1 = tid % 8;
    *index_2 = (tid * 3) % 9;

    if (*index_1 == *index_2)
    {
        *index_2 = (*index_2 + 1) % 9;
    }

    return (*index_1 + *index_2) % 8;
}

static uint8_t hnn_beacon_swap_bit(uint8_t data, uint8_t first, uint8_t secnd)
{
    uint8_t bit1 = (data >> first) & 1;
    uint8_t bit2 = (data >> secnd) & 1;

    if (bit1 != bit2)
    {
        data ^= (1 << first);
        data ^= (1 << secnd);
    }

    return data;
}

uint16_t calculate_checksum(uint8_t data[], uint8_t len)
{
    uint16_t checksum = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        checksum += data[i];
        if (checksum > 0xFFFF)
        {
            checksum = (checksum & 0xFFFF) + 1;
        }
    }

    checksum = ~checksum;
    return checksum;
}

uint16_t encrypt_data(uint8_t *data, uint8_t len, uint8_t tid)
{
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t first = 0;
        uint8_t secnd = 0;
        get_index_conv_by_tid(tid, &first, &secnd);
        data[i] = hnn_beacon_swap_bit(data[i], first, secnd);
    }

    return calculate_checksum(data, len);
}

uint16_t decrypt_data(uint8_t *data, uint8_t len, uint8_t tid)
{
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t first = 0;
        uint8_t secnd = 0;
        get_index_conv_by_tid(tid, &first, &secnd);
        data[i] = hnn_beacon_swap_bit(data[i], first, secnd);
    }

    return calculate_checksum(data, len);
}
