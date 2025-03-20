#include "hardware.h"
#include "bl_timer.h"

static uint8_t hc595_data;
static uint8_t need_send_hc595;

static const uint8_t arr_seg_7[] = {
    0b10000000, // 0 (0x40) - Các thanh 0,1,2,3,4,5 sáng (bit 0), thanh 6 tắt (bit 1)
    0b11110010, // 1 (0x79) - Thanh 1,2 sáng
    0b01001000, // 2 (0x24) - Thanh 0,1,6,4,3 sáng
    0b01100000, // 3 (0x30) - Thanh 0,1,6,2,3 sáng
    0b00110010, // 4 (0x19) - Thanh 1,2,5,6 sáng
    0b00100100, // 5 (0x12) - Thanh 0,6,2,3,5 sáng
    0b00000100, // 6 (0x02) - Thanh 0,6,2,3,4,5 sáng
    0b11110000, // 7 (0x78) - Thanh 0,1,2 sáng
    0b00000000, // 8 (0x00) - Tất cả sáng
    0b00100000, // 9 (0x10) - Thanh 0,1,2,3,5,6 sáng
    0b11111110, // 10 (0x00) - Tắt hết
    0b00010000, // A
    0b10001100, // C

};

void hw_74hc595_init(void)
{
    if (device_type == FANAC)
    {
        return;
    }
    bl_gpio_enable_output(LED_COM_PIN, 1, 0);
    bl_gpio_enable_output(SHCP_PIN, 1, 0);
    bl_gpio_enable_output(STCP_PIN, 1, 0);
    bl_gpio_enable_output(DS_PIN, 1, 0);
    bl_gpio_enable_output(OE_PIN, 1, 0);
    // bl_gpio_enable_output(OE_PIN_2, 1, 0);

    bl_gpio_output_set(SHCP_PIN, 0);
    bl_gpio_output_set(STCP_PIN, 0);
    bl_gpio_output_set(DS_PIN, 0);
    bl_gpio_output_set(OE_PIN, 1);
    // bl_gpio_output_set(OE_PIN_2, 0);
    hc595_data = 0b11111110;
    need_send_hc595 = 0;
}

void hw_74hc595_set_bit(uint8_t bit, uint8_t val)
{
    if (device_type == FANAC)
    {
        return;
    }
    hc595_data &= ~(1 << bit);
    hc595_data |= ((val ? 1 : 0) << bit);
    need_send_hc595 = 1;
}

void hw_74hc595_set_number(uint8_t num)
{
    if (device_type == FANAC)
    {
        return;
    }
    os_trace("%s num: %u\r\n", __func__, num);
    hc595_data &= 0x01;
    hc595_data |= arr_seg_7[num];
    need_send_hc595 = 1;
}

void hw_74hc595_write_byte(uint16_t data, uint8_t num_hc595)
{
    if (device_type == FANAC)
    {
        return;
    }
    bl_gpio_output_set(SHCP_PIN, 0);
    bl_gpio_output_set(STCP_PIN, 0);
    uint16_t i;
    uint16_t mask = 0x80;
    if (num_hc595 == 2)
    {
        mask = 0x8000;
    }
    for (i = 0; i < 8 * num_hc595; i++)
    {
        bl_gpio_output_set(DS_PIN, (data & mask) ? 1 : 0);
        data <<= 1;
        bl_gpio_output_set(SHCP_PIN, 1);
        bl_gpio_output_set(SHCP_PIN, 0);
    }
    bl_gpio_output_set(STCP_PIN, 1);
    bl_gpio_output_set(STCP_PIN, 0);
    bl_gpio_output_set(OE_PIN, 0);
    // if (num_hc595 == 2)
    // {
    //     bl_gpio_output_set(OE_PIN_2, 0);
    // }
}

void hw_74hc595_loop(void)
{
    if (device_type == FANAC)
    {
        return;
    }
    if (need_send_hc595 == 1)
    {
        {
            need_send_hc595 = 0;
            hw_74hc595_write_byte(hc595_data, 1);
        }
    }
}
