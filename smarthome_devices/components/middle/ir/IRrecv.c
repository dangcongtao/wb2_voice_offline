#include "IRrecv.h"

#include "hardware.h"
#include "bl_timer.h"
#include "hosal_gpio.h"
#include "IRutils.h"
#include "bl602_glb.h"

irparams_t params;
irparams_t *params_save;
hosal_gpio_irq_trigger_t trigger_type;

uint8_t _tolerance;

hosal_gpio_dev_t touch_gpio;

/// Interrupt handler for changes on the GPIO pin handling incoming IR messages.
static void gpio_intr(void)
{
    uint32_t now = bl_timer_now_us();
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
    os_trace("[%d]%d\r\n", params.rawlen, params.rawbuf[rawlen]);
    params.rawlen++;

    start = now;

    if (trigger_type == HOSAL_IRQ_TRIG_NEG_PULSE) {
        trigger_type = HOSAL_IRQ_TRIG_POS_PULSE;
    } else {
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
#if defined(ESP32)
    timerAlarmDisable(timer);
    gpio_intr_enable((gpio_num_t)params.recvpin);
#endif // ESP32
}

uint8_t _validTolerance(const uint8_t percentage) {
    return (percentage > 100) ? _tolerance : percentage;
}

uint32_t ticksLow(const uint32_t usecs, const uint8_t tolerance,
                          const uint16_t delta) {
  // max() used to ensure the result can't drop below 0 before the cast.
  return ((uint32_t)max(
      (int32_t)(usecs * (1.0 - _validTolerance(tolerance) / 100.0) - delta),
      (int32_t)0));
}

uint32_t ticksHigh(const uint32_t usecs, const uint8_t tolerance,
                           const uint16_t delta) {
  return ((uint32_t)(usecs * (1.0 + _validTolerance(tolerance) / 100.0)) + 1 +
          delta);
}

bool match(uint32_t measured, uint32_t desired, uint8_t tolerance,
                   uint16_t delta) {
  measured *= kRawTick;  // Convert to uSecs.
  os_trace("Matching: ");
  os_trace("%u", ticksLow(desired, tolerance, delta));
  os_trace(" <= ");
  os_trace("%u",measured);
  os_trace(" <= ");
  os_trace("%u", ticksHigh(desired, tolerance, delta));
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
#endif  // UNIT_TEST
  return (measured >= ticksLow(desired, tolerance, delta) &&
          measured <= ticksHigh(desired, tolerance, delta));
}

bool matchMark(uint32_t measured, uint32_t desired, uint8_t tolerance,
                       int16_t excess) {
os_trace("Matching MARK ");
  os_trace("%u",(measured * kRawTick));
  os_trace(" vs ");
  os_trace("%u",desired);
  os_trace(" + ");
  os_trace("%u",excess);
  os_trace(". \r\n");
  return match(measured, desired + excess, tolerance, 0);
}

bool matchSpace(uint32_t measured, uint32_t desired, uint8_t tolerance,
                        int16_t excess) {
  os_trace("Matching SPACE ");
  os_trace("%u",measured * kRawTick);
  os_trace(" vs ");
  os_trace("%u",desired);
  os_trace(" - ");
  os_trace("%u",excess);
  os_trace(". \r\n");
  return match(measured, desired - excess, tolerance, 0);
}

match_result_t matchData(
    volatile uint16_t *data_ptr, const uint16_t nbits, const uint16_t onemark,
    const uint32_t onespace, const uint16_t zeromark, const uint32_t zerospace,
    const uint8_t tolerance, const int16_t excess, const bool MSBfirst,
    const bool expectlastspace) {
  match_result_t result;
  result.success = false;  // Fail by default.
  result.data = 0;
  if (expectlastspace) {  // We are expecting data with a final space.
    for (result.used = 0; result.used < nbits * 2;
         result.used += 2, data_ptr += 2) {
      // Is the bit a '1'?
      if (matchMark(*data_ptr, onemark, tolerance, excess) &&
          matchSpace(*(data_ptr + 1), onespace, tolerance, excess)) {
        result.data = (result.data << 1) | 1;
      } else if (matchMark(*data_ptr, zeromark, tolerance, excess) &&
                 matchSpace(*(data_ptr + 1), zerospace, tolerance, excess)) {
        result.data <<= 1;  // The bit is a '0'.
      } else {
        if (!MSBfirst) result.data = reverseBits(result.data, result.used / 2);
        return result;  // It's neither, so fail.
      }
    }
    result.success = true;
  } else {  // We are expecting data without a final space.
    // Match all but the last bit, as it may not match easily.
    result = matchData(data_ptr, nbits ? nbits - 1 : 0, onemark, onespace,
                       zeromark, zerospace, tolerance, excess, true, true);
    if (result.success) {
      // Is the bit a '1'?
      if (matchMark(*(data_ptr + result.used), onemark, tolerance, excess))
        result.data = (result.data << 1) | 1;
      else if (matchMark(*(data_ptr + result.used), zeromark, tolerance,
               excess))
        result.data <<= 1;  // The bit is a '0'.
      else
        result.success = false;
      if (result.success) result.used++;
    }
  }
  if (!MSBfirst) result.data = reverseBits(result.data, nbits);
  return result;
}

uint16_t matchBytes(volatile uint16_t *data_ptr, uint8_t *result_ptr,
                            const uint16_t remaining, const uint16_t nbytes,
                            const uint16_t onemark, const uint32_t onespace,
                            const uint16_t zeromark, const uint32_t zerospace,
                            const uint8_t tolerance, const int16_t excess,
                            const bool MSBfirst, const bool expectlastspace) {
  // Check if there is enough capture buffer to possibly have the desired bytes.
  if (remaining + expectlastspace < (nbytes * 8 * 2) + 1)
    return 0;  // Nope, so abort.
  uint16_t offset = 0;
  for (uint16_t byte_pos = 0; byte_pos < nbytes; byte_pos++) {
    bool lastspace = (byte_pos + 1 == nbytes) ? expectlastspace : true;
    match_result_t result = matchData(data_ptr + offset, 8, onemark, onespace,
                                      zeromark, zerospace, tolerance, excess,
                                      MSBfirst, lastspace);
    if (result.success == false) return 0;  // Fail
    result_ptr[byte_pos] = (uint8_t)result.data;
    offset += result.used;
  }
  return offset;
}

bool matchAtLeast(uint32_t measured, uint32_t desired,
                          uint8_t tolerance, uint16_t delta) {
  measured *= kRawTick;  // Convert to uSecs.
  os_trace("Matching ATLEAST ");
  os_trace("%u",measured);
  os_trace(" vs ");
  os_trace("%u",desired);
  os_trace(". Matching: ");
  os_trace("%u",measured);
  os_trace(" >= ");
  os_trace("%u", ticksLow(min(desired, (uint32_t)MS_TO_USEC(params.timeout)),
                  tolerance, delta));
  os_trace(" [min(");
  os_trace("%u", ticksLow(desired, tolerance, delta));
  os_trace(", ");
  os_trace("%u", ticksLow(MS_TO_USEC(params.timeout), tolerance, delta));
  os_trace(")]");
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
#endif  // UNIT_TEST
  // We really should never get a value of 0, except as the last value
  // in the buffer. If that is the case, then assume infinity and return true.
  if (measured == 0) return true;
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
                              const bool MSBfirst) {
  // If we are expecting byte sizes, check it's a factor of 8 or fail.
  if (!use_bits && nbits % 8 != 0)  return 0;
  // Calculate if we expect a trailing space in the data section.
  const bool kexpectspace = footermark || (onespace != zerospace);
  // Calculate how much remaining buffer is required.
  uint16_t min_remaining = nbits * 2 - (kexpectspace ? 0 : 1);

  if (hdrmark) min_remaining++;
  if (hdrspace) min_remaining++;
  if (footermark) min_remaining++;
  // Don't need to extend for footerspace because it could be the end of message

  // Check if there is enough capture buffer to possibly have the message.
  if (remaining < min_remaining) return 0;  // Nope, so abort.
  uint16_t offset = 0;

  // Header
  if (hdrmark && !matchMark(*(data_ptr + offset++), hdrmark, tolerance, excess))
    return 0;
  if (hdrspace && !matchSpace(*(data_ptr + offset++), hdrspace, tolerance,
                              excess))
    return 0;

  // Data
  if (use_bits) {  // Bits.
    match_result_t result = matchData(data_ptr + offset, nbits,
                                              onemark, onespace,
                                              zeromark, zerospace, tolerance,
                                              excess, MSBfirst, kexpectspace);
    if (!result.success) return 0;
    *result_bits_ptr = result.data;
    offset += result.used;
  } else {  // bytes
    uint16_t data_used = matchBytes(data_ptr + offset, result_bytes_ptr,
                                            remaining - offset, nbits / 8,
                                            onemark, onespace,
                                            zeromark, zerospace, tolerance,
                                            excess, MSBfirst, kexpectspace);
    if (!data_used) return 0;
    offset += data_used;
  }
  // Footer
  if (footermark && !matchMark(*(data_ptr + offset++), footermark, tolerance,
                               excess))
    return 0;
  // If we have something still to match & haven't reached the end of the buffer
  if (footerspace && offset < remaining) {
      if (atleast) {
        if (!matchAtLeast(*(data_ptr + offset), footerspace, tolerance, excess))
          return 0;
      } else {
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
                              const bool MSBfirst) {
  return _matchGeneric(data_ptr, result_ptr, NULL, true, remaining, nbits,
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

    // Clear the entry we are currently pointing to when we got the timeout.
    // i.e. Stopped collecting IR data.
    // It's junk as we never wrote an entry to it and can only confuse decoding.
    // This is done here rather than logically the best place in read_timeout()
    // as it saves a few bytes of ICACHE_RAM as that routine is bound to an
    // interrupt. decode() is not stored in ICACHE_RAM.
    // Another better option would be to zero the entire irparams.rawbuf[] on
    // resume() but that is a much more expensive operation compare to this.
    // However, don't do this if rawbuf is already full as we stomp over the heap.
    // See: https://github.com/crankyoldgit/IRremoteESP8266/issues/1516
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
        os_trace("Attempting LG decode\r\n");
        if (decodeLG(results, offset, kLgBits, false)) {
            os_trace("decode LG success\r\n");
            return true;
        }
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

void enableIRIn(const bool pullup)
{
    // ESP32's seem to require explicitly setting the GPIO to INPUT etc.
    // This wasn't required on the ESP8266s, but it shouldn't hurt to make sure.

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
       trigger_type = HOSAL_IRQ_TRIG_NEG_PULSE;
    hosal_gpio_irq_set(&touch_gpio, HOSAL_IRQ_TRIG_NEG_PULSE, gpio_intr, NULL);
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

void ir_loop(void)
{

    if (params.rawlen) params.rcvstate = kStopState;
#if IR_RX
#if REPORT_UNKNOWNS
    if (decode(&capture))
    {
#else // REPORT_UNKNOWNS
        if (decode(&capture, NULL, 0, 0) 
        // &&
        //     capture.decode_type != UNKNOWN
            )
        {
#endif // REPORT_UNKNOWNS
            lastIrReceivedTime = bl_timer_now_us();
            memset(lastIrReceived, 0, sizeof(lastIrReceived));
            int seek = 0;
            char result_str[128];
            memset(result_str, 0, sizeof(result_str));
            resultToHexidecimal(&capture, result_str);
            seek += sprintf(lastIrReceived, "%d,", capture.decode_type, result_str);
#if REPORT_RAW_UNKNOWNS
            if (capture.decode_type == UNKNOWN)
            {
                lastIrReceived += ';';
                for (uint16_t i = 1; i < capture.rawlen; i++)
                {
                    uint32_t usecs;
                    for (usecs = capture.rawbuf[i] * kRawTick; usecs > UINT16_MAX;
                         usecs -= UINT16_MAX)
                    {
                        lastIrReceived += uint64ToString(UINT16_MAX);
                        lastIrReceived += F(",0,");
                    }
                    lastIrReceived += uint64ToString(usecs, 10);
                    if (i < capture.rawlen - 1)
                        lastIrReceived += ',';
                }
            }
#endif // REPORT_RAW_UNKNOWNS
            // If it isn't an AC code, add the bits.
            if (!hasACState(capture.decode_type)) {
                seek += sprintf(&lastIrReceived[seek], ",%d", capture.bits);
            }
            os_trace("\r\nDECODE: %s\r\n", lastIrReceived);
#if MQTT_ENABLE
            mqtt_client.publish(MqttRecv.c_str(), lastIrReceived.c_str());
            mqttSentCounter++;
            debug("Incoming IR message sent to MQTT:");
            debug(lastIrReceived.c_str());
#endif // MQTT_ENABLE
            irRecvCounter++;
#if USE_DECODED_AC_SETTINGS
            if (decodeCommonAc(&capture))
                lastClimateSource = F("IR");
#endif // USE_DECODED_AC_SETTINGS
        }
#endif // IR_RX
        // delay(100);
    // }
}