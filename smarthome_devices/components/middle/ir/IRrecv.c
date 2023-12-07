#include "IRrecv.h"

#include "hardware.h"
#include "bl_timer.h"
#include "hosal_gpio.h"
#include "IRutils.h"
#include "IRsend.h"
#include "bl602_glb.h"
#include <bl_os_hal.h>
#include <bl_wdt.h>

irparams_t params;
irparams_t *params_save;
hosal_gpio_irq_trigger_t trigger_type = HOSAL_IRQ_TRIG_NEG_PULSE;

uint8_t _tolerance;

hosal_gpio_dev_t touch_gpio;

struct timer_adpt *timer3;

static void timer3_cb(void *data)
{
    if (params.rawlen)
        params.rcvstate = kStopState;
    // os_trace("timer3_cb:start timer1 again and change timer cycle\r\n");
    // uint32_t delay_ms=1000;           //cycle =1000ms
    // bl_os_timer_start_periodic(timer1,(delay_ms / 1000),((delay_ms % 1000) * 1e6));
}

/// Interrupt handler for changes on the GPIO pin handling incoming IR messages.
static void gpio_intr(void)
{

    uint32_t now = bl_timer_now_us();

    // bl_os_timer_stop(timer3, BL_OS_NO_WAITING);
    static uint32_t start = 0;

#if defined(ESP8266)
    uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    os_timer_disarm(&timer);
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
#endif // ESP8266

    // Grab a local copy of rawlen to reduce instructions used in IRAM.
    // This is an ugly premature optimisation code-wise, but we do everything we
    // can to save IRAM.
    // It seems referencing the value via the structure uses more instructions.
    // Less instructions means faster and less IRAM used.
    // N.B. It saves about 13 bytes of IRAM.
    uint16_t rawlen = params.rawlen;

    if (rawlen >= params.bufsize)
    {
        params.overflow = true;
        params.rcvstate = kStopState;
    }

    if (params.rcvstate == kStopState)
        // os_trace("stop_state\r\n");
        return;

    if (params.rcvstate == kIdleState)
    {
        params.rcvstate = kMarkState;
        params.rawbuf[rawlen] = 1;
    }
    else
    {
        if (now < start)
            params.rawbuf[rawlen] = (UINT32_MAX - start + now) / kRawTick;
        else
            params.rawbuf[rawlen] = (now - start) / kRawTick;
    }
    // os_trace("[%d]%d\r\n", params.rawlen, params.rawbuf[rawlen]);
    params.rawlen++;

    start = now;

    uint32_t delay_ms = (uint32_t)params.timeout;
    bl_os_timer_start_once(timer3, (delay_ms / 1000), ((delay_ms % 1000) * 1e6));

    if (trigger_type == HOSAL_IRQ_TRIG_NEG_PULSE)
    {
        trigger_type = HOSAL_IRQ_TRIG_POS_PULSE;
    }
    else
    {
        trigger_type = HOSAL_IRQ_TRIG_NEG_PULSE;
    }
    bl_set_gpio_intmod(params.recvpin, GLB_GPIO_INT_CONTROL_ASYNC, trigger_type);

    // #if defined(ESP8266)
    //   os_timer_arm(&timer, params.timeout, ONCE);
    // #endif  // ESP8266
}

void resume(void)
{
    params.rcvstate = kIdleState;
    params.rawlen = 0;
    params.overflow = false;
    trigger_type = HOSAL_IRQ_TRIG_NEG_PULSE;
    bl_set_gpio_intmod(params.recvpin, GLB_GPIO_INT_CONTROL_ASYNC, trigger_type);
#if defined(ESP32)
    timerAlarmDisable(timer);
    gpio_intr_enable((gpio_num_t)params.recvpin);
#endif // ESP32
}

uint8_t _validTolerance(const uint8_t percentage)
{
    return (percentage > 100) ? _tolerance : percentage;
}

uint32_t ticksLow(const uint32_t usecs, const uint8_t tolerance,
                  const uint16_t delta)
{
    // max() used to ensure the result can't drop below 0 before the cast.
    return ((uint32_t)max(
        (int32_t)(usecs * (1.0 - _validTolerance(tolerance) / 100.0) - delta),
        (int32_t)0));
}

uint32_t ticksHigh(const uint32_t usecs, const uint8_t tolerance,
                   const uint16_t delta)
{
    return ((uint32_t)(usecs * (1.0 + _validTolerance(tolerance) / 100.0)) + 1 +
            delta);
}

bool match(uint32_t measured, uint32_t desired, uint8_t tolerance,
           uint16_t delta)
{
    measured *= kRawTick; // Convert to uSecs.
#if DEBUG_LOG_RX
    os_trace("Matching: ");
    os_trace("%lu", ticksLow(desired, tolerance, delta));
    os_trace(" <= ");
    os_trace("%lu", measured);
    os_trace(" <= ");
    os_trace("%lu\r\n", ticksHigh(desired, tolerance, delta));
#endif
#ifdef UNIT_TEST
    // Sanity checks that we don't have values that cause integer over/underflow.
    // Only performed during testing so there is no performance hit in normal
    // operation.
    assert(ticksLow(desired, tolerance, delta) <= desired);
    // Check if we overflowed.  (UINT32_MAX >> 3 is approx 9 minutes!)
    assert(ticksHigh(desired, tolerance, delta) < UINT32_MAX >> 3);
    // Check if our high mark is below where we started. This could happen.
    // If there is a legit case, then this should be removed.
    assert(ticksHigh(desired, tolerance, delta) >= desired);
#endif // UNIT_TEST
    return (measured >= ticksLow(desired, tolerance, delta) &&
            measured <= ticksHigh(desired, tolerance, delta));
}

bool matchMark(uint32_t measured, uint32_t desired, uint8_t tolerance,
               int16_t excess)
{
#if DEBUG_LOG_RX
    os_trace("Matching MARK ");
    os_trace("%lu", (measured * kRawTick));
    os_trace(" vs ");
    os_trace("%lu", desired);
    os_trace(" + ");
    os_trace("%lu", excess);
    os_trace(". \r\n");
#endif
    return match(measured, desired + excess, tolerance, 0);
}

bool matchSpace(uint32_t measured, uint32_t desired, uint8_t tolerance,
                int16_t excess)
{
#if DEBUG_LOG_RX
    os_trace("Matching SPACE ");
    os_trace("%lu", measured * kRawTick);
    os_trace(" vs ");
    os_trace("%lu", desired);
    os_trace(" - ");
    os_trace("%lu", excess);
    os_trace(". \r\n");
#endif
    return match(measured, desired - excess, tolerance, 0);
}

match_result_t matchData(
    volatile uint16_t *data_ptr, const uint16_t nbits, const uint16_t onemark,
    const uint32_t onespace, const uint16_t zeromark, const uint32_t zerospace,
    const uint8_t tolerance, const int16_t excess, const bool MSBfirst,
    const bool expectlastspace)
{
    match_result_t result;
    result.success = false; // Fail by default.
    result.data = 0;
    if (expectlastspace)
    { // We are expecting data with a final space.
        for (result.used = 0; result.used < nbits * 2;
             result.used += 2, data_ptr += 2)
        {
            // Is the bit a '1'?
            if (matchMark(*data_ptr, onemark, tolerance, excess) &&
                matchSpace(*(data_ptr + 1), onespace, tolerance, excess))
            {
                result.data = (result.data << 1) | 1;
                #if DEBUG_LOG_RX
                os_trace("\r\n%u======>1\r\n", result.used);
                #endif
            }
            else if (matchMark(*data_ptr, zeromark, tolerance, excess) &&
                     matchSpace(*(data_ptr + 1), zerospace, tolerance, excess))
            {
                result.data <<= 1; // The bit is a '0'.
                #if DEBUG_LOG_RX
                os_trace("\r\n%u======>0\r\n", result.used);
                #endif
            }
            else
            {
                if (!MSBfirst)
                    result.data = reverseBits(result.data, result.used / 2);
                return result; // It's neither, so fail.
            }
        }
        result.success = true;
    }
    else
    { // We are expecting data without a final space.
        // Match all but the last bit, as it may not match easily.
        result = matchData(data_ptr, nbits ? nbits - 1 : 0, onemark, onespace,
                           zeromark, zerospace, tolerance, excess, true, true);
        if (result.success)
        {
            // Is the bit a '1'?
            if (matchMark(*(data_ptr + result.used), onemark, tolerance, excess))
                result.data = (result.data << 1) | 1;
            else if (matchMark(*(data_ptr + result.used), zeromark, tolerance,
                               excess))
                result.data <<= 1; // The bit is a '0'.
            else
                result.success = false;
            if (result.success)
                result.used++;
        }
    }
    if (!MSBfirst)
        result.data = reverseBits(result.data, nbits);
    return result;
}

uint16_t matchBytes(volatile uint16_t *data_ptr, uint8_t *result_ptr,
                    const uint16_t remaining, const uint16_t nbytes,
                    const uint16_t onemark, const uint32_t onespace,
                    const uint16_t zeromark, const uint32_t zerospace,
                    const uint8_t tolerance, const int16_t excess,
                    const bool MSBfirst, const bool expectlastspace)
{
    // Check if there is enough capture buffer to possibly have the desired bytes.
    if (remaining + expectlastspace < (nbytes * 8 * 2) + 1)
        return 0; // Nope, so abort.
    uint16_t offset = 0;
    for (uint16_t byte_pos = 0; byte_pos < nbytes; byte_pos++)
    {
        bool lastspace = (byte_pos + 1 == nbytes) ? expectlastspace : true;
        match_result_t result = matchData(data_ptr + offset, 8, onemark, onespace,
                                          zeromark, zerospace, tolerance, excess,
                                          MSBfirst, lastspace);
        if (result.success == false)
            return 0; // Fail
        
        result_ptr[byte_pos] = (uint8_t)result.data;
        offset += result.used;
    }
    return offset;
}

bool matchAtLeast(uint32_t measured, uint32_t desired,
                  uint8_t tolerance, uint16_t delta)
{
    measured *= kRawTick; // Convert to uSecs.
#if DEBUG_LOG_RX
    os_trace("Matching ATLEAST ");
    os_trace("%lu", measured);
    os_trace(" vs ");
    os_trace("%lu", desired);
    os_trace(". Matching: ");
    os_trace("%lu", measured);
    os_trace(" >= ");
    os_trace("%lu", ticksLow(min(desired, (uint32_t)MS_TO_USEC(params.timeout)),
                             tolerance, delta));
    os_trace(" [min(");
    os_trace("%lu", ticksLow(desired, tolerance, delta));
    os_trace(", ");
    os_trace("%lu", ticksLow(MS_TO_USEC(params.timeout), tolerance, delta));
    os_trace(")]\r\n");
#endif
#ifdef UNIT_TEST
    // Sanity checks that we don't have values that cause integer over/underflow.
    // Only performed during testing so there is no performance hit in normal
    // operation.
    assert(ticksLow(desired, tolerance, delta) <= desired);
    // Check if we overflowed.  (UINT32_MAX >> 3 is approx 9 minutes!)
    assert(ticksHigh(desired, tolerance, delta) < UINT32_MAX >> 3);
    // Check if our high mark is below where we started. This could happen.
    // If there is a legit case, then this should be removed.
    assert(ticksHigh(desired, tolerance, delta) >= desired);
#endif // UNIT_TEST
    // We really should never get a value of 0, except as the last value
    // in the buffer. If that is the case, then assume infinity and return true.
    if (measured == 0)
        return true;
    return measured >= ticksLow(min(desired,
                                    (uint32_t)MS_TO_USEC(params.timeout)),
                                tolerance, delta);
}

uint16_t _matchGeneric(volatile uint16_t *data_ptr,
                       uint64_t *result_bits_ptr,
                       uint8_t *result_bytes_ptr,
                       const bool use_bits,
                       const uint16_t remaining,
                       const uint16_t nbits,
                       const uint16_t hdrmark,
                       const uint32_t hdrspace,
                       const uint16_t onemark,
                       const uint32_t onespace,
                       const uint16_t zeromark,
                       const uint32_t zerospace,
                       const uint16_t footermark,
                       const uint32_t footerspace,
                       const bool atleast,
                       const uint8_t tolerance,
                       const int16_t excess,
                       const bool MSBfirst)
{
    // If we are expecting byte sizes, check it's a factor of 8 or fail.
    if (!use_bits && nbits % 8 != 0)
        return 0;
    // Calculate if we expect a trailing space in the data section.
    const bool kexpectspace = footermark || (onespace != zerospace);
    // Calculate how much remaining buffer is required.
    uint16_t min_remaining = nbits * 2 - (kexpectspace ? 0 : 1);

    if (hdrmark)
        min_remaining++;
    if (hdrspace)
        min_remaining++;
    if (footermark)
        min_remaining++;
    // Don't need to extend for footerspace because it could be the end of message

    // Check if there is enough capture buffer to possibly have the message.
    if (remaining < min_remaining)
        return 0; // Nope, so abort.
    uint16_t offset = 0;

    // Header
    if (hdrmark && !matchMark(*(data_ptr + offset++), hdrmark, tolerance, excess))
        return 0;
    if (hdrspace && !matchSpace(*(data_ptr + offset++), hdrspace, tolerance,
                                excess))
        return 0;

    // Data
    if (use_bits)
    { // Bits.
        match_result_t result = matchData(data_ptr + offset, nbits,
                                          onemark, onespace,
                                          zeromark, zerospace, tolerance,
                                          excess, MSBfirst, kexpectspace);
        if (!result.success)
            return 0;
        *result_bits_ptr = result.data;
        offset += result.used;
    }
    else
    { // bytes
        // os_trace("%s match bytes use_bits: %d\r\n", __func__, use_bits);
        uint16_t data_used = matchBytes(data_ptr + offset, result_bytes_ptr,
                                        remaining - offset, nbits / 8,
                                        onemark, onespace,
                                        zeromark, zerospace, tolerance,
                                        excess, MSBfirst, kexpectspace);
        if (!data_used)
            return 0;
        offset += data_used;
    }
    // Footer
    if (footermark && !matchMark(*(data_ptr + offset++), footermark, tolerance,
                                 excess))
        return 0;
    // If we have something still to match & haven't reached the end of the buffer
    if (footerspace && offset < remaining)
    {
        if (atleast)
        {
            if (!matchAtLeast(*(data_ptr + offset), footerspace, tolerance, excess))
                return 0;
        }
        else
        {
            if (!matchSpace(*(data_ptr + offset), footerspace, tolerance, excess))
                return 0;
        }
        offset++;
    }
    return offset;
}

uint16_t matchGeneric(volatile uint16_t *data_ptr,
                      uint64_t *result_ptr,
                      const uint16_t remaining,
                      const uint16_t nbits,
                      const uint16_t hdrmark,
                      const uint32_t hdrspace,
                      const uint16_t onemark,
                      const uint32_t onespace,
                      const uint16_t zeromark,
                      const uint32_t zerospace,
                      const uint16_t footermark,
                      const uint32_t footerspace,
                      const bool atleast,
                      const uint8_t tolerance,
                      const int16_t excess,
                      const bool MSBfirst)
{
    return _matchGeneric(data_ptr, result_ptr, NULL, true, remaining, nbits,
                         hdrmark, hdrspace, onemark, onespace,
                         zeromark, zerospace, footermark, footerspace, atleast,
                         tolerance, excess, MSBfirst);
}

uint16_t matchGenericUint8(volatile uint16_t *data_ptr,
                        uint8_t *result_ptr,
                        const uint16_t remaining, const uint16_t nbits,
                        const uint16_t hdrmark, const uint32_t hdrspace,
                        const uint16_t onemark, const uint32_t onespace,
                        const uint16_t zeromark, const uint32_t zerospace,
                        const uint16_t footermark, const uint32_t footerspace,
                        const bool atleast,
                        const uint8_t tolerance,
                        const int16_t excess,
                        const bool MSBfirst )
{
    return _matchGeneric(data_ptr, NULL, result_ptr, false, remaining, nbits,
                         hdrmark, hdrspace, onemark, onespace,
                         zeromark, zerospace, footermark, footerspace, atleast,
                         tolerance, excess, MSBfirst);
}

void copyIrParams(volatile irparams_t *src, irparams_t *dst)
{
    // Typecast src and dst addresses to (char *)
    char *csrc = (char *)src; // NOLINT(readability/casting)
    char *cdst = (char *)dst; // NOLINT(readability/casting)

    // Save the pointer to the destination's rawbuf so we don't lose it as
    // the for-loop/copy after this will overwrite it with src's rawbuf pointer.
    // This isn't immediately obvious due to typecasting/different variable names.
    uint16_t *dst_rawbuf_ptr;
    dst_rawbuf_ptr = dst->rawbuf;

    // Copy contents of src[] to dst[]
    for (uint16_t i = 0; i < sizeof(irparams_t); i++)
        cdst[i] = csrc[i];

    // Restore the buffer pointer
    dst->rawbuf = dst_rawbuf_ptr;

    // Copy the rawbuf
    for (uint16_t i = 0; i < dst->bufsize; i++)
        dst->rawbuf[i] = src->rawbuf[i];
}

bool decode(decode_results *results, irparams_t *save,
            uint8_t max_skip, uint16_t noise_floor)
{
    // Proceed only if an IR message been received.
#ifndef UNIT_TEST
    if (params.rcvstate != kStopState)
        return false;
#endif
    os_trace("%s\r\n", __func__);

    if (!params.overflow)
        params.rawbuf[params.rawlen] = 0;

    bool resumed = false; // Flag indicating if we have resumed.

    // If we were requested to use a save buffer previously, do so.
    if (save == NULL)
        save = params_save;

    if (save == NULL)
    {
        // We haven't been asked to copy it so use the existing memory.
#ifndef UNIT_TEST
        results->rawbuf = params.rawbuf;
        results->rawlen = params.rawlen;
        results->overflow = params.overflow;
#endif
    }
    else
    {
        copyIrParams(&params, save); // Duplicate the interrupt's memory.
        resume();                    // It's now safe to rearm. The IR message won't be overridden.
        resumed = true;
        // Point the results at the saved copy.
        results->rawbuf = save->rawbuf;
        results->rawlen = save->rawlen;
        results->overflow = save->overflow;
    }

    // Reset any previously partially processed results.
    results->decode_type = UNKNOWN;
    results->bits = 0;
    results->value = 0;
    results->address = 0;
    results->command = 0;
    results->repeat = false;

    if (results->rawlen <= 10) {
        trigger_type = HOSAL_IRQ_TRIG_NEG_PULSE;
        bl_set_gpio_intmod(params.recvpin, GLB_GPIO_INT_CONTROL_ASYNC, trigger_type);
        if (!resumed) // Check if we have already resumed.
            resume();
        return false;
    }

    os_trace("BUFF_LEN[%d]: ", results->rawlen);
if (1){
    char str_res[300];
    int seek = 0;
    memset(str_res, 0, sizeof(str_res));
    for (uint32_t h = 0; h < results->rawlen; h++)
    {
        seek += sprintf(&str_res[seek], "%u-", results->rawbuf[h]);
        if (seek >= 128)
        {
            seek = 0;
            os_trace(str_res);
            memset(str_res, 0, sizeof(str_res));
        }
    }
    sprintf(&str_res[seek], "\r\n");
    os_trace(str_res);
}

    

    // os_trace("bit: ");
    if (results->rawlen == 200 && 0)
    {
        uint64_t data_te = 0;
        uint64_t lenn = 0;
        int step = 0;
        uint8_t temp = 0, fan = 0, mode = 0, power = 0;
        uint8_t neg_mode_dry = 0;
        uint8_t btn = 0;
        char fan_str[10], mode_str[10];
        char btn_str[10];

        for (uint32_t h = 4; h < results->rawlen; h += 2)
        {
            if (results->rawbuf[h] > 2000)
            {
                os_trace("-%llu-0x%llx", lenn, data_te);
                os_trace("|");
                if (step == 0)
                {
                    temp = (data_te >> 28) & 0xf;
                    mode = (data_te >> 24) & 0xf;
                    fan = (data_te >> 12) & 0b110000;
                    neg_mode_dry = (data_te >> 22) & 0b1;
                }
                else if (step == 1)
                {
                    if (mode == 3 || mode == 4)
                    { // mode DRY or FAN
                        if ((data_te >> 60) == 0xf)
                        {
                            temp = 0xf;
                        }
                        else if (mode == 3)
                        {
                            uint8_t temp_1 = (data_te >> 60);
                            switch (temp_1)
                            {
                            case 0xa:
                            case 0xe:
                                temp = 0x1;
                                break;
                            case 0x9:
                            case 0xd:
                                temp = 0x2;
                                break;

                            default:
                                temp = 0xf;
                                break;
                            }
                        }
                    }
                }
                data_te = 0;
                lenn = 0;
                step++;
            }
            else if (results->rawbuf[h] > 600 && results->rawbuf[h] < 1000)
            {
                os_trace("1");
                data_te |= ((uint64_t)1 << lenn);

                lenn++;
            }
            else if (results->rawbuf[h] > 100 && results->rawbuf[h] < 500)
            {
                os_trace("0");
                data_te |= ((uint64_t)0 << lenn);
                lenn++;
            }
            else
            {
                os_trace("INVALID(%u)", results->rawbuf[h]);
                lenn++;
            }
        }

        os_trace("\r\n-%llu-0x%llx\r\n", lenn, data_te);

        fan |= (data_te >> 30) & 0b1;
        power = (data_te >> 36) & 0b1;
        btn = (data_te >> 8) & 0x1F;

        memset(fan_str, 0, sizeof(fan_str));
        memset(mode_str, 0, sizeof(mode_str));
        memset(btn_str, 0, sizeof(btn_str));

        switch (btn)
        {
        case 0x00:
            sprintf(btn_str, "LIGHT_OFF");
            break;
        case 0x01:
            sprintf(btn_str, "POWER");
            break;
        case 0x02:
            sprintf(btn_str, "TEMP");
            break;
        case 0x04:
            sprintf(btn_str, "SUPER");
            break;
        case 0x05:
            sprintf(btn_str, "TIMER");
            break;
        case 0x06:
            sprintf(btn_str, "MODE");
            break;
        case 0x7:
            sprintf(btn_str, "DIR_HOR");
            break;
        case 0x8:
            sprintf(btn_str, "DIR_VER");
            break;
        case 0x0c:
            sprintf(btn_str, "ECO");
            break;
        case 0x11:
            sprintf(btn_str, "FAN SPEED");
            break;
        case 0x03:
            sprintf(btn_str, "SLEEP");
            break;
        case 0x0b:
            sprintf(btn_str, "QUITE");
            break;
        case 0x17:
            sprintf(btn_str, "SMART");
            break;
        
        default:
            sprintf(btn_str, "INVALID");
            break;
        }



        if (fan == 0x00)
        {
            sprintf(fan_str, "AUTO");
        }
        else if (fan == 0x10)
        {
            sprintf(fan_str, "VERY HIGH");
        }
        else if (fan == 0x11)
        {
            sprintf(fan_str, "HIGH");
        }
        else if (fan == 0x20)
        {
            sprintf(fan_str, "MEDIUM");
        }
        else if (fan == 0x31)
        {
            sprintf(fan_str, "LOW");
        }
        else if (fan == 0x30)
        {
            sprintf(fan_str, "VERY LOW");
        }
        else if (fan == 0x40)
        {
            sprintf(fan_str, "POWER OFF");
        }
        else
        {
            sprintf(fan_str, "INVALID");
        }

        switch (mode)
        {
        case 0:
            sprintf(mode_str, "HEAT");
            break;

        case 2:
            sprintf(mode_str, "COOL");
            break;

        case 3:
            sprintf(mode_str, "DRY");
            // if (temp == 2 && temp != 0xf) {
            temp += (data_te >> 18) & 0x7;
            // }
            break;

        case 4:
            sprintf(mode_str, "FAN");
            break;

        default:
            sprintf(mode_str, "INVALID");
            break;
        }

        os_trace("-%llu-0x%llx", lenn, data_te);
        os_trace("\r\n");

        os_trace("btn: %s, power: %u, temp: %u *C, mode: %s/%u, fan: %s/%02X, neg_mode_dry: %u(-)%u\r\n", btn_str, power, temp + 16, mode_str, mode, fan_str, fan, neg_mode_dry, temp);
    }

#if ENABLE_NOISE_FILTER_OPTION
    crudeNoiseFilter(results, noise_floor);
#endif // ENABLE_NOISE_FILTER_OPTION
    // Keep looking for protocols until we've run out of entries to skip or we
    // find a valid protocol message.
    for (uint16_t offset = kStartOffset;
         offset <= (max_skip * 2) + kStartOffset;
         offset += 2)
    {
#if DECODE_LG
        os_trace("Attempting LG (28-bit) decode\r\n");
        if (decodeLG(results, offset, kLgBits, false))
        {
            os_trace("decode LG 28-bit success\r\n");
            return true;
        }
        os_trace("Attempting LG (32-bit) decode\r\n");
        if (decodeLG(results, offset, kLg32Bits, true))
        {
            os_trace("decode LG 32-bit success\r\n");
            return true;
        }
#endif

#if DECODE_NEC
        os_trace("Attempting NEC decode\r\n");
        if (decodeNEC(results, offset, kNECBits, true))
        {
            os_trace("decode NEC success\r\n");
            return true;
        }
#endif

#if DECODE_SAMSUNG_AC
    // os_trace("Attempting Samsung AC (extended) decode\r\n");
    // Check the extended size first, as it should fail fast due to longer
    // length.
    // if (decodeSamsungAC(results, offset, kSamsungAcExtendedBits, true)) return true;
    // Now check for the more common length.
    os_trace("Attempting Samsung AC decode\r\n");
    if (decodeSamsungAC(results, offset, kSamsungAcBits, false)) return true;
#endif


#if DECODE_FUNIKI
    os_trace("Attempting Funiki AC\r\n");
    if (decodeFunikiAC(results, offset, kFunikiAcBits, true)) return true;
    if (decodeFunikiAC(results, offset, kFunikiAcExtendedBits, true)) return true;
#endif

#if DECODE_AQUA
    os_trace("Attempting Aqua AC\r\n");
    if (decodeAquaAC(results, offset, kAquaAcBits, true)) return true;
#endif

#if DECODE_CASPER
    os_trace("Attempting Casper AC\r\n");
    if (decodeCasperAC(results, offset, kCasperAcBits, true)) return true;
#endif

    }

#if DECODE_HASH
    // decodeHash returns a hash on any input.
    // Thus, it needs to be last in the list.
    // If you add any decodes, add them before this.
    if (decodeHash(results))
    {
        return true;
    }
#endif // DECODE_HASH
    // Throw away and start over
    if (!resumed) // Check if we have already resumed.
        resume();
    return false;
}

static void enableIRIn(const bool pullup)
{
    // ESP32's seem to require explicitly setting the GPIO to INPUT etc.
    // This wasn't required on the ESP8266s, but it shouldn't hurt to make sure.

    timer3 = bl_os_timer_create(timer3_cb, NULL);

    bl_gpio_enable_input(params.recvpin, 1, 0);

    touch_gpio.port = params.recvpin;

//   if (pullup) {
// #ifndef UNIT_TEST
//     pinMode(params.recvpin, INPUT_PULLUP);
//   } else {
//     pinMode(params.recvpin, INPUT);
// #endif  // UNIT_TEST
//   }
#if defined(ESP32)
    // Initialise the ESP32 timer.
    // 80MHz / 80 = 1 uSec granularity.
    timer = timerBegin(_timer_num, 80, true);
#ifdef DEBUG
    if (timer == NULL)
    {
        os_trace("FATAL: Unable enable system timer: ");
        os_traceLN((uint16_t)_timer_num);
    }
#endif                     // DEBUG
    assert(timer != NULL); // Check we actually got the timer.
    // Set the timer so it only fires once, and set it's trigger in uSeconds.
    timerAlarmWrite(timer, MS_TO_USEC(params.timeout), ONCE);
    // Note: Interrupt needs to be attached before it can be enabled or disabled.
    // Note: EDGE (true) is not supported, use LEVEL (false). Ref: #1713
    // See: https://github.com/espressif/arduino-esp32/blob/caef4006af491130136b219c1205bdcf8f08bf2b/cores/esp32/esp32-hal-timer.c#L224-L227
    timerAttachInterrupt(timer, &read_timeout, false);
#endif // ESP32

    // Initialise state machine variables
    resume();

#ifndef UNIT_TEST
#if defined(ESP8266)
    // Initialise ESP8266 timer.
    os_timer_disarm(&timer);
    os_timer_setfn(&timer, reinterpret_cast<os_timer_func_t *>(read_timeout),
                   NULL);
#endif // ESP8266
       // Attach Interrupt
       //   attachInterrupt(params.recvpin, gpio_intr, CHANGE);
    //    trigger_type = HOSAL_IRQ_TRIG_NEG_PULSE;
    hosal_gpio_irq_set(&touch_gpio, trigger_type, gpio_intr, NULL);
#endif // UNIT_TEST
}

void IRrecv_init(const uint32_t recvpin, const uint16_t bufsize,
                 const uint8_t timeout, const bool save_buffer)
{
    /// @endcond
    params.recvpin = recvpin;
    params.bufsize = bufsize;
    // Ensure we are going to be able to store all possible values in the
    // capture buffer.
    params.timeout = min(timeout, (uint8_t)kMaxTimeoutMs);
    params.rawbuf = pvPortMalloc(sizeof(uint16_t) * bufsize);
    if (params.rawbuf == NULL)
    {
        os_trace(
            "Could not allocate memory for the primary IR buffer.\n"
            "Try a smaller size for CAPTURE_BUFFER_SIZE.\nRebooting!");
    }
    // If we have been asked to use a save buffer (for decoding), then create one.
    if (save_buffer)
    {
        params_save = pvPortMalloc(sizeof(irparams_t));
        params_save->rawbuf = pvPortMalloc(sizeof(uint16_t) * bufsize);
        // Check we allocated the memory successfully.
        if (params_save->rawbuf == NULL)
        {
            os_trace(
                "Could not allocate memory for the second IR buffer.\n"
                "Try a smaller size for CAPTURE_BUFFER_SIZE.\nRebooting!");
        }
    }
    else
    {
        params_save = NULL;
    }
#if DECODE_HASH
    _unknown_threshold = kUnknownThreshold;
#endif // DECODE_HASH
    _tolerance = kTolerance;
}

decode_results capture;
uint32_t lastIrReceivedTime = 0;
uint32_t irRecvCounter = 0;
char *lastIrReceived[256];

#include "IRutils.h"

void m_ir_task(void)
{
    os_trace("%s init\r\n", __func__);
    while (1)
    {
        if (decode(&capture, NULL, 0, 0))
        {
            os_trace("decode_type: %d, bits: %u, value: 0x%llx, command: %lx, address: 0x%llx\r\n", capture.decode_type, capture.bits, (uint64_t)capture.value, capture.command, (uint64_t)capture.address);
            state_ir_t ir_state = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
            if (capture.decode_type == LG2) {
                LG_setRaw(capture.value, capture.decode_type, &ir_state);
            } else if (capture.decode_type == AQUA){
                // os_trace("data[16]: 0x");
                // for (uint8_t i = 0; i < 14; i++) {
                //     os_trace("%02x", capture.state[i]);
                // }
                // os_trace("\r\n");
                AQUA_setRaw(capture.state, capture.decode_type, &ir_state);
            } else if (capture.decode_type == FUNIKI){
                // os_trace("FUNIKI: 0x");
                // for (uint8_t i = 0; i < 18; i++) {
                //     if (i == 6) {
                //         os_trace("\t|\t");
                //     } else if (i == 12) {
                //         if (capture.bits == kFunikiAcBits)
                //             break;
                //         else {
                //             os_trace("\t|\t");
                //         }
                //     }
                //     os_trace("%02x", capture.state[i]);
                // }
                // os_trace("\r\n");
                Funiki_setRaw((uint8_t*)&capture.value, capture.decode_type, &ir_state);
            } else if (capture.decode_type == CASPER){
                os_trace("CASPER: 0x");
                for (uint8_t i = 0; i < 21; i++) {
                    if (i == 6 || i == 14) {
                        os_trace("\t|\t");
                    }
                    os_trace("%02x", capture.state[i]);
                }
                os_trace("\r\n");
                Casper_setRaw((uint8_t*)&capture.value, capture.decode_type, &ir_state);
            }
            IR_logCommon(ir_state);
            irRecvCounter++;
            resume();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void m_ir_init(void)
{
    bl_wdt_init(60000);
    bl_gpio_enable_output(IRpin, 0, 0);
    IRsend_init(IRpin, false, true);
    IRrecv_init(IR_RX, 1024, 50, false);

    enableIRIn(1);

    xTaskCreate(m_ir_task,   /* Function that implements the task. */
                "m_ir_task", /* Text name for the task. */
                1024,        /* Stack size in words, not bytes. */
                NULL,        /* Parameter passed into the task. */
                1,           /* Priority at which the task is created. */
                NULL);
}