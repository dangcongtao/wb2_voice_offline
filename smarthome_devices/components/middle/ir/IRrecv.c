#include "IRrecv.h"

#include "hardware.h"
#include "bl_timer.h"
#include "hosal_gpio.h"
#include "IRutils.h"
#include "IRsend.h"
#include "bl602_glb.h"
#include <bl_os_hal.h>
#include "axk_attr.h"
#include "IRutils.h"

DRAM_ATTR irparams_t params;
irparams_t *params_save;
DRAM_ATTR hosal_gpio_irq_trigger_t trigger_type = HOSAL_IRQ_TRIG_NEG_PULSE;

uint8_t _tolerance;

hosal_gpio_dev_t touch_gpio_pos;
hosal_gpio_dev_t touch_gpio_neg;

struct timer_adpt *timer3;

static void timer3_cb(void *data)
{
    if (params.rawlen)
        params.rcvstate = kStopState;
    // os_trace("timer3_cb:start timer1 again and change timer cycle\r\n");
    // uint32_t delay_ms=1000;           //cycle =1000ms
    // bl_os_timer_start_periodic(timer1,(delay_ms / 1000),((delay_ms % 1000) * 1e6));
}

void Irrecv_pause(void)
{
    params.rcvstate = kStopState;
}

DRAM_ATTR uint32_t  start = 0;
/// Interrupt handler for changes on the GPIO pin handling incoming IR messages.
IRAM_ATTR void gpio_intr_neg(void)
{
    uint32_t now = bl_timer_now_us();

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

    bl_os_timer_start_once(timer3, ((uint32_t)params.timeout / 1000), (((uint32_t)params.timeout % 1000) * 1e6));

    

    // #if defined(ESP8266)
    //   os_timer_arm(&timer, params.timeout, ONCE);
    // #endif  // ESP8266
}
IRAM_ATTR void gpio_intr_pos(void)
{
    uint32_t now = bl_timer_now_us();
    
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
        return;
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

    bl_os_timer_start_once(timer3, ((uint32_t)params.timeout / 1000), (((uint32_t)params.timeout % 1000) * 1e6));

    

    // #if defined(ESP8266)
    //   os_timer_arm(&timer, params.timeout, ONCE);
    // #endif  // ESP8266
}

void resume(void)
{
    params.rcvstate = kIdleState;
    params.rawlen = 0;
    params.overflow = false;
    // trigger_type = HOSAL_IRQ_TRIG_NEG_PULSE;
    // bl_set_gpio_intmod(params.recvpin, GLB_GPIO_INT_CONTROL_ASYNC, trigger_type);
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
        #if DEBUG_LOG_RX
        os_trace("result_ptr[%u]: %u\r\n", byte_pos, result_ptr[byte_pos]);
        #endif
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

uint16_t matchGenericState(volatile uint16_t *data_ptr,
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

// HUNONIC_LIMIT

bool matchLimit(uint32_t measured, uint32_t lower_limit, uint32_t upper_limit)
{
#if DEBUG_LOG_RX
    os_trace("%s measured: %lu, lower_limit: %lu, upper_limit: %lu\r\n", __func__, measured, lower_limit, upper_limit);
#endif
    return measured * 2 >= lower_limit && measured * 2 <= upper_limit;
}

match_result_t matchDataLimit(
    volatile uint16_t *data_ptr, const uint16_t nbits, 
                    const uint16_t onemarkLower,
                    const uint16_t onemarkUpper,
                    const uint32_t onespaceLower,
                    const uint32_t onespaceUpper,
                    const uint16_t zeromarkLower,
                    const uint16_t zeromarkUpper,
                    const uint32_t zerospaceLower,
                    const uint32_t zerospaceUpper,
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
            if (matchLimit(*data_ptr, onemarkLower, onemarkUpper) &&
                matchLimit(*(data_ptr + 1), onespaceLower, onespaceUpper))
            {
                result.data = (result.data << 1) | 1;
                #if DEBUG_LOG_RX
                os_trace("\r\n%u======>1\r\n", result.used);
                #endif
            }
            else if (matchLimit(*data_ptr, zeromarkLower, zeromarkUpper) &&
                     matchLimit(*(data_ptr + 1), zerospaceLower, zerospaceUpper))
            {
                result.data <<= 1; // The bit is a '0'.
                #if DEBUG_LOG_RX
                os_trace("\r\n%u======>0\r\n", result.used);
                #endif
            }
            else
            {
                if (!MSBfirst) {
                    os_trace("%s !MSBfirst one mark data: %lu, %lu %lu, space %lu, %lu %lu\r\n", __func__, *data_ptr * 2, onemarkLower, onemarkUpper,*(data_ptr + 1) * 2, onespaceLower, onespaceUpper);
                    os_trace("%s !MSBfirst zero mark data: %lu, %lu %lu, space %lu, %lu %lu\r\n", __func__, *data_ptr * 2, zeromarkLower, zeromarkUpper,(*(data_ptr + 1)) * 2, zerospaceLower, zerospaceUpper);
                    result.data = reverseBits(result.data, result.used / 2);
                }
                return result; // It's neither, so fail.
            }
        }
        result.success = true;
    }
    else
    { // We are expecting data without a final space.
        // Match all but the last bit, as it may not match easily.
        result = matchDataLimit(data_ptr, nbits ? nbits - 1 : 0, onemarkLower, onemarkUpper, onespaceLower, onespaceUpper,
                                          zeromarkLower, zeromarkUpper, zerospaceLower, zerospaceUpper, tolerance, excess, true, true);
        if (result.success)
        {
            // Is the bit a '1'?
            if (matchLimit(*(data_ptr + result.used), onemarkLower, onemarkUpper))
                result.data = (result.data << 1) | 1;
            else if (matchLimit(*(data_ptr + result.used), zeromarkLower, zeromarkUpper))
                result.data <<= 1; // The bit is a '0'.
            else {
                os_trace("%s data: %u, %u %u %u %u\r\n", __func__, *(data_ptr + result.used), onemarkLower, onemarkUpper, zeromarkLower, zeromarkUpper);
                result.success = false;
            }
            if (result.success)
                result.used++;
        }
    }
    if (!MSBfirst)
        result.data = reverseBits(result.data, nbits);
    return result;
}

uint16_t matchBytesLimit(volatile uint16_t *data_ptr, uint8_t *result_ptr,
                    const uint16_t remaining, const uint16_t nbytes,
                    const uint16_t onemarkLower,
                    const uint16_t onemarkUpper,
                    const uint32_t onespaceLower,
                    const uint32_t onespaceUpper,
                    const uint16_t zeromarkLower,
                    const uint16_t zeromarkUpper,
                    const uint32_t zerospaceLower,
                    const uint32_t zerospaceUpper,
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
        match_result_t result = matchDataLimit(data_ptr + offset, 8, onemarkLower, onemarkUpper, onespaceLower, onespaceUpper,
                                          zeromarkLower, zeromarkUpper, zerospaceLower, zerospaceUpper, tolerance, excess,
                                          MSBfirst, lastspace);
        if (result.success == false)
            return 0; // Fail
        
        result_ptr[byte_pos] = (uint8_t)result.data;
        #if DEBUG_LOG_RX
        os_trace("result_ptr[%u]: %u\r\n", byte_pos, result_ptr[byte_pos]);
        #endif
        offset += result.used;
    }
    return offset;
}

uint16_t _matchGenericLimit(volatile uint16_t *data_ptr,
                       uint64_t *result_bits_ptr,
                       uint8_t *result_bytes_ptr,
                       const bool use_bits,
                       const uint16_t remaining,
                       const uint16_t nbits,
                       const uint16_t hdrmarkLower,
                       const uint16_t hdrmarkUpper,
                       const uint32_t hdrspaceLower,
                       const uint32_t hdrspaceUpper,
                       const uint16_t onemarkLower,
                       const uint16_t onemarkUpper,
                       const uint32_t onespaceLower,
                       const uint32_t onespaceUpper,
                       const uint16_t zeromarkLower,
                       const uint16_t zeromarkUpper,
                       const uint32_t zerospaceLower,
                       const uint32_t zerospaceUpper,
                       const uint16_t footermarkLower,
                       const uint16_t footermarkUpper,
                       const uint32_t footerspaceLower,
                       const uint32_t footerspaceUpper,
                       const bool atleast,
                       const uint8_t tolerance,
                       const int16_t excess,
                       const bool MSBfirst)
{
    // If we are expecting byte sizes, check it's a factor of 8 or fail.
    if (!use_bits && nbits % 8 != 0)
        return 0;
    // Calculate if we expect a trailing space in the data section.
    const bool kexpectspace = footermarkLower || (onespaceLower != zerospaceLower);
    // Calculate how much remaining buffer is required.
    uint16_t min_remaining = nbits * 2 - (kexpectspace ? 0 : 1);

    if (hdrmarkLower)
        min_remaining++;
    if (hdrspaceLower)
        min_remaining++;
    if (footermarkLower)
        min_remaining++;
    // Don't need to extend for footerspace because it could be the end of message

    // Check if there is enough capture buffer to possibly have the message.
    if (remaining < min_remaining)
        return 0; // Nope, so abort.
    uint16_t offset = 0;

    // Header
    if (hdrmarkLower && !matchLimit(*(data_ptr + offset++), hdrmarkLower, hdrmarkUpper))
        return 0;
    if (hdrspaceLower && !matchLimit(*(data_ptr + offset++), hdrspaceLower, hdrspaceUpper))
        return 0;

    // Data
    if (use_bits)
    { // Bits.
        match_result_t result = matchDataLimit(data_ptr + offset, nbits,
                                          onemarkLower, onemarkUpper, onespaceLower, onespaceUpper,
                                          zeromarkLower, zeromarkUpper, zerospaceLower, zerospaceUpper,
                                          tolerance,
                                          excess, MSBfirst, kexpectspace);
        if (!result.success)
            return 0;
        *result_bits_ptr = result.data;
        offset += result.used;
    }
    else
    { // bytes
        // os_trace("%s match bytes use_bits: %d\r\n", __func__, use_bits);
        uint16_t data_used = matchBytesLimit(data_ptr + offset, result_bytes_ptr,
                                        remaining - offset, nbits / 8,
                                        onemarkLower, onemarkUpper, onespaceLower, onespaceUpper,
                                        zeromarkLower, zeromarkUpper, zerospaceLower, zerospaceUpper, 
                                        tolerance,
                                        excess, MSBfirst, kexpectspace);
        if (!data_used)
            return 0;
        offset += data_used;
    }
    // Footer
    if (footermarkLower && !matchLimit(*(data_ptr + offset++), footermarkLower, footermarkUpper)) {
        os_trace("%s footer, mark: %lu, %lu %lu\r\n", __func__, *(data_ptr + offset - 1) * 2, footermarkLower, footermarkUpper);
        return 0;
    }
    // If we have something still to match & haven't reached the end of the buffer
    if (footerspaceLower && offset < remaining)
    {
        if (atleast)
        {
            if (!matchLimit(*(data_ptr + offset), footerspaceLower, footerspaceUpper)) {
                os_trace("%s atleast 1, offset:%u, space: %lu, %lu %lu, space %lu, %lu %lu\r\n", __func__, offset, *(data_ptr + offset) * 2, footerspaceLower, footerspaceUpper);
                return 0;
            }
        }
        else
        {
            if (!matchLimit(*(data_ptr + offset), footerspaceLower, footerspaceUpper)) {
                os_trace("%s atleast 0, offset:%u, space: %lu, %lu %lu, space %lu, %lu %lu\r\n", __func__, offset, *(data_ptr + offset) * 2, footerspaceLower, footerspaceUpper);
                return 0;
            }
        }
        offset++;
    }
    return offset;
}

uint16_t matchGenericDataLimit(volatile uint16_t *data_ptr,
                      uint64_t *result_ptr,
                      const uint16_t remaining,
                      const uint16_t nbits,
                      const uint16_t hdrmarkLower,
                       const uint16_t hdrmarkUpper,
                       const uint32_t hdrspaceLower,
                       const uint32_t hdrspaceUpper,
                       const uint16_t onemarkLower,
                       const uint16_t onemarkUpper,
                       const uint32_t onespaceLower,
                       const uint32_t onespaceUpper,
                       const uint16_t zeromarkLower,
                       const uint16_t zeromarkUpper,
                       const uint32_t zerospaceLower,
                       const uint32_t zerospaceUpper,
                       const uint16_t footermarkLower,
                       const uint16_t footermarkUpper,
                       const uint32_t footerspaceLower,
                       const uint32_t footerspaceUpper,
                      const bool atleast,
                      const uint8_t tolerance,
                      const int16_t excess,
                      const bool MSBfirst)
{
    return _matchGenericLimit(data_ptr, result_ptr, NULL, true, remaining, nbits,
                         hdrmarkLower,hdrmarkUpper, hdrspaceLower, hdrspaceUpper,
                         onemarkLower, onemarkUpper, onespaceLower, onespaceUpper,
                                          zeromarkLower, zeromarkUpper, zerospaceLower, zerospaceUpper, 
                                          footermarkLower,footermarkUpper, footerspaceLower, footerspaceUpper, 
                                          atleast, tolerance, excess, MSBfirst); 
}

uint16_t matchGenericStateLimit(volatile uint16_t *data_ptr,
                        uint8_t *result_ptr,
                        const uint16_t remaining, const uint16_t nbits,
                        const uint16_t hdrmarkLower,
                       const uint16_t hdrmarkUpper,
                       const uint32_t hdrspaceLower,
                       const uint32_t hdrspaceUpper,
                       const uint16_t onemarkLower,
                       const uint16_t onemarkUpper,
                       const uint32_t onespaceLower,
                       const uint32_t onespaceUpper,
                       const uint16_t zeromarkLower,
                       const uint16_t zeromarkUpper,
                       const uint32_t zerospaceLower,
                       const uint32_t zerospaceUpper,
                       const uint16_t footermarkLower,
                       const uint16_t footermarkUpper,
                       const uint32_t footerspaceLower,
                       const uint32_t footerspaceUpper,
                        const bool atleast,
                        const uint8_t tolerance,
                        const int16_t excess,
                        const bool MSBfirst )
{
    return _matchGenericLimit(data_ptr, NULL, result_ptr, false, remaining, nbits,
                         hdrmarkLower,hdrmarkUpper, hdrspaceLower, hdrspaceUpper,
                         onemarkLower, onemarkUpper, onespaceLower, onespaceUpper,
                                          zeromarkLower, zeromarkUpper, zerospaceLower, zerospaceUpper, 
                                          footermarkLower,footermarkUpper, footerspaceLower, footerspaceUpper, 
                                          atleast,
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
    os_trace("\t%s %u\r\n", __func__, params.rawlen);

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
        // trigger_type = HOSAL_IRQ_TRIG_NEG_PULSE;
        // bl_set_gpio_intmod(params.recvpin, GLB_GPIO_INT_CONTROL_ASYNC, trigger_type);
        if (!resumed) // Check if we have already resumed.
            resume();
        return false;
    }

    
if (1){
    os_trace("BUFF_LEN[%d]: ", results->rawlen);
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
    os_trace("Attempting Samsung AC (extended) decode\r\n");
    // Check the extended size first, as it should fail fast due to longer
    // length.
    if (decodeSamsungAC(results, offset, kSamsungAcExtendedBits, true)) return true;
    // Now check for the more common length.
    os_trace("Attempting Samsung AC decode\r\n");
    if (decodeSamsungAC(results, offset, kSamsungAcBits, true)) return true;
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

#if DECODE_TOSHIBA
    os_trace("Attempting Toshiba AC 72bit decode\r\n");
    if (decodeToshibaAC(results, offset, kToshibaACBits, true)) return true;
    // os_trace("Attempting Toshiba AC 80bit decode\r\n");
    // if (decodeToshibaAC(results, offset, kToshibaACBitsLong, true)) return true;
    // os_trace("Attempting Toshiba AC 56bit decode\r\n");
    // if (decodeToshibaAC(results, offset, kToshibaACBitsShort, true)) return true;
#endif

#if DECODE_DAIKIN
    os_trace("Attempting Daikin AC decode\r\n");
    if (decodeDaikin(results, offset, kDaikinBits, true)) return true;
#endif

#if DECODE_DAIKIN2
    os_trace("Attempting Daikin 2 AC decode\r\n");
    if (decodeDaikin2(results, offset, kDaikin2Bits, true)) return true;
#endif

#if DECODE_DAIKIN152
    os_trace("Attempting Daikin 152 bit AC decode\r\n");
    if (decodeDaikin152(results, offset, kDaikin152Bits, true)) return true;
#endif

#if DECODE_DAIKIN160
    os_trace("Attempting Daikin 160 bit AC decode\r\n");
    if (decodeDaikin160(results, offset, kDaikin160Bits, true)) return true;
#endif

#if DECODE_DAIKIN216
    os_trace("Attempting Daikin 216 bit AC decode\r\n");
    if (decodeDaikin216(results, offset, kDaikin216Bits, true)) return true;
#endif

#if DECODE_DAIKIN128
    os_trace("Attempting Daikin 128 bit AC decode\r\n");
    if (decodeDaikin128(results, offset, kDaikin128Bits, true)) return true;
#endif

#if DECODE_DAIKIN64
    os_trace("Attempting Daikin 64 bit AC decode\r\n");
    if (decodeDaikin64(results, offset, kDaikin64Bits, true)) return true;
#endif



#if DECODE_GREE
    os_trace("Attempting Gree AC decode\r\n");
    if (decodeGree(results, offset, kGreeBits, true)) return true;
#endif

#if (DECODE_MITSUBISHI112 || DECODE_NAGAKAWA || DECODE_TCL112AC)
    os_trace("Attempting decodeMitsubishi, Tcl, Nagakawa 112bit AC decode\r\n");
    if (decodeMitsubishi112(results, offset, kMitsubishi112Bits, true)) return true;
#endif

#if DECODE_MIDEA
    os_trace("Attempting decodeMidea AC decode\r\n");        
    if (decodeMidea(results, offset, kMideaBits, true)) return true;

    // os_trace("Attempting decodeMidea 24bit AC decode\r\n");        
    // if (decodeMidea24(results, offset, kMidea24Bits, true)) return true;
#endif

#if DECODE_SHARP
    if (decodeSharpAc(results, offset, kSharpAcBits, true)) return true;
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

    touch_gpio_neg.port = params.recvpin;
    touch_gpio_pos.port = IR_RX_POS;

    bl_gpio_enable_input(params.recvpin, 1, 0);
    bl_gpio_enable_input(IR_RX_POS, 0, 1);

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
    hosal_gpio_irq_set(&touch_gpio_neg, HOSAL_IRQ_TRIG_NEG_PULSE, gpio_intr_neg, NULL);
    hosal_gpio_irq_set(&touch_gpio_pos, HOSAL_IRQ_TRIG_POS_PULSE, gpio_intr_pos, NULL);
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

#include "base64.h"
#include "m_mqtt.h"

static decode_results capture;
static uint32_t lastIrReceivedTime = 0;
static uint32_t irRecvCounter = 0;
static uint32_t old_time_learn_ir = 0;
static uint32_t old_time_learn_ir_auto = 0;



void IR_start_learn(void)
{
    old_time_learn_ir = sys_get_tick_ms() | 1;
    old_time_learn_ir_auto = 0;
}

void IR_stop_learn(void)
{
    old_time_learn_ir = 0;
}

void IR_start_learn_auto(void)
{
    old_time_learn_ir_auto = sys_get_tick_ms() | 1;
    old_time_learn_ir = 0;
}

void IR_stop_learn_auto(void)
{
    old_time_learn_ir_auto = 0;
}

static void learnIR(decode_results *results)
{
    // decode_results cb;
    // decode_results *results = &cb;
    // results->rawlen = 739;
    // results->rawbuf = results1->rawbuf;
    // memset(results->rawbuf + results1->rawlen, 100, sizeof(uint16_t)*(results->rawlen-results1->rawlen));
    uint16_t len_temp = getCorrectedRawLength(results);
    os_trace("%s step 1 %u\r\n", __func__, len_temp);
    uint16_t result_temp[len_temp];
    

    resultToRawArray(results, result_temp);
    os_trace("%s step 2\r\n", __func__);

    int size_rawData = getCorrectedRawLength(results) * sizeof(uint16_t);
    os_trace("%s step 3\r\n", __func__);

    uint8_t buffDataIR[2001];
    memset(buffDataIR, 0, sizeof(buffDataIR));
    b64_encode((unsigned char *)result_temp, size_rawData, (unsigned char *)buffDataIR);
    os_trace("%s step 4: len_buffDataIR: %u\r\n\r\n\r\n", __func__, strlen((const char*)buffDataIR));
    // os_trace("%s step 5: len_buffDataIR: %u\r\n\r\n\r\n", __func__, strlen((const char*)buffDataIR));

    char str[MAXIMUM_SIZE_MESSAGE];
    memset(str, 0, MAXIMUM_SIZE_MESSAGE);

    sprintf(str, "{\"%s\":3,\"data\":\"%s\"}", gs_dev_type[device_type], buffDataIR);
    // os_trace("datasend[%u]:", strlen(str));
    // os_trace("'%s'\r\n", str);
    m_mqtt_get_instance()._mqtt_publish(str, strlen(str));
}

void m_ir_task(void)
{
    os_trace("%s init\r\n", __func__);
    while (1)
    {
        if (decode(&capture, NULL, 0, 0))
        {
            os_trace("decode_type: %d, bits: %u, value: 0x%llx, command: %lx, address: 0x%llx\r\n", capture.decode_type, capture.bits, (uint64_t)capture.value, capture.command, (uint64_t)capture.address);
            state_ir_t ir_state = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
            state_ir_t ir_state_recv = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
            
            if (capture.decode_type == LG2) {
                LG_setRaw(capture.value, capture.decode_type, &ir_state);
                LG_setInfo(capture.value, capture.decode_type, &ir_state_recv);
            } else if (capture.decode_type == AQUA){
                AQUA_setRaw(capture.state, capture.decode_type, &ir_state);
            } else if (capture.decode_type == FUNIKI){
                Funiki_setRaw((uint8_t*)&capture.value, capture.decode_type, &ir_state);
            } else if (capture.decode_type == CASPER){
                Casper_setRaw((uint8_t*)&capture.value, capture.decode_type, &ir_state);
            } else if (capture.decode_type == SAMSUNG_AC){
                switch (capture.bits)
                {
                case kSamsungAcBits:
                    Samsung_setRaw((uint8_t*)&capture.value, kSamsungAcStateLength, capture.decode_type, &ir_state);
                    break;
                
                case kSamsungAcExtendedBits:
                    Samsung_setRaw((uint8_t*)&capture.value, kSamsungAcExtendedStateLength, capture.decode_type, &ir_state);
                    break;
                
                default:
                    break;
                }
                
            } else if (capture.decode_type == TOSHIBA_AC){
                switch (capture.bits)
                {
                case kToshibaACBits:
                    Toshiba_setRaw((uint8_t*)capture.state, kToshibaACStateLength, capture.decode_type, &ir_state);
                    break;
                
                case kToshibaACBitsShort:
                    Toshiba_setRaw((uint8_t*)capture.state, kToshibaACStateLengthShort, capture.decode_type, &ir_state);
                    break;
                
                case kToshibaACBitsLong:
                    Toshiba_setRaw((uint8_t*)capture.state, kToshibaACStateLengthLong, capture.decode_type, &ir_state);
                    break;
                
                default:
                    break;
                }
            } 
            else if (capture.decode_type == DAIKIN){
                Daikin_setRaw((uint8_t*)capture.state, kDaikinStateLength, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == DAIKIN2){
                Daikin2_setRaw((uint8_t*)capture.state, kDaikin2StateLength, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == DAIKIN64){
                Daikin64_setRaw(capture.value, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == DAIKIN152){
                Daikin152_setRaw((uint8_t*)capture.state, kDaikin152StateLength, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == DAIKIN160){
                Daikin160_setRaw((uint8_t*)capture.state, kDaikin160StateLength, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == DAIKIN216){
                Daikin216_setRaw((uint8_t*)capture.state, kDaikin216StateLength, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == DAIKIN128){
                Daikin128_setRaw((uint8_t*)capture.state, kDaikin128StateLength, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == GREE){
                Gree_setRaw((uint8_t*)capture.state, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == MITSUBISHI112){
                os_trace("MITSUBISHI112: 0x ");
                for(uint8_t i = 0; i < 14; i++) {
                    os_trace("%02X-", capture.state[i]);
                }
                os_trace("\r\n");
                Mitsubishi112_setRaw((uint8_t*)capture.state, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == NAGAKAWA){
                os_trace("NAGAKAWA: 0x ");
                for(uint8_t i = 0; i < 14; i++) {
                    os_trace("%02X-", capture.state[i]);
                }
                os_trace("\r\n");
                Nagakawa_setRaw((uint8_t*)capture.state, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == TCL112AC){
                os_trace("TCL: 0x ");
                for(uint8_t i = 0; i < 14; i++) {
                    os_trace("%02X-", capture.state[i]);
                }
                os_trace("\r\n");
                Tcl112_setRaw((uint8_t*)capture.state, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == MIDEA){
                Midea_setRaw(capture.value, capture.decode_type, &ir_state);
            }
            else if (capture.decode_type == SHARP_AC){
                SharpAc_setRaw((uint8_t*)capture.state, capture.decode_type, &ir_state);
            }

            if (old_time_learn_ir) {
                learnIR(&capture);
                old_time_learn_ir = 0;
            }
            else if(old_time_learn_ir_auto && capture.decode_type != UNKNOWN) 
            {
                char str[128];
                memset(str, 0, 128);

                sprintf(str, "{\"%s\":17,\"type\":1,\"brand\":\"%d\",\"model\":%d}", 
                gs_dev_type[device_type], capture.decode_type, ir_state_recv.model);
                os_trace("learn_auto_datasend[%u]:", strlen(str));
                os_trace("'%s'\r\n", str);
                m_mqtt_get_instance()._mqtt_publish(str, strlen(str));

                old_time_learn_ir_auto = 0;
            }
            else if (capture.decode_type != UNKNOWN)
            {
                char str[325];
                memset(str, 0, 325);

                sprintf(str, "{\"%s\":10,\"type\":1,\"brand\":\"%d\",\"model\":%d,\"power\":%u,\"mode\":%d,\"temp\":%.1f,\"fan\":%d}", 
                gs_dev_type[device_type], capture.decode_type, ir_state_recv.model, ir_state_recv.power, ir_state_recv.mode, ir_state_recv.degrees, ir_state_recv.fanspeed);
                os_trace("datarcv[%u]:", strlen(str));
                os_trace("'%s'\r\n", str);
                m_mqtt_get_instance()._mqtt_publish(str, strlen(str));
            }
            
            IR_logCommon(ir_state);
            irRecvCounter++;
            
            resume();
        }

        if (old_time_learn_ir) {
            if (sys_get_tick_ms() - old_time_learn_ir >= 120000) {
                old_time_learn_ir = 0;
            }
        } else if (old_time_learn_ir_auto) {
            if (sys_get_tick_ms() - old_time_learn_ir_auto >= 120000) {
                old_time_learn_ir_auto = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void m_ir_init(void)
{
    bl_gpio_enable_output(IRpin, 0, 0);
    IRsend_init(IRpin, false, true);
    IRrecv_init(IR_RX, 1024, 50, false);

    enableIRIn(1);

    xTaskCreate(m_ir_task,   /* Function that implements the task. */
                "m_ir_task", /* Text name for the task. */
                1024*3,        /* Stack size in words, not bytes. */
                NULL,        /* Parameter passed into the task. */
                1,           /* Priority at which the task is created. */
                NULL);
}