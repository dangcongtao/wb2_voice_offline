#include "IRsend.h"
#include <stdio.h>
#include "hardware.h"
#include "IRtimer.h"
#include "bl_timer.h"

static int8_t periodOffset = kPeriodOffset;
static bool modulation;
static uint16_t onTimePeriod;
static uint16_t offTimePeriod;
static int8_t periodOffset;
static uint8_t _dutycycle;
static uint8_t outputOn;
static uint8_t outputOff;

void IR_logCommon(state_ir_t ir_state)
{
  char result[256];
  memset(result, 0, sizeof(result));
  int seek = 0;

  seek += sprintf(&result[seek], "power: ");
  if (ir_state.power == 1)
  {
    seek += sprintf(&result[seek], "ON");
  }
  else
  {
    seek += sprintf(&result[seek], "OFF");
  }

  seek += sprintf(&result[seek], ", mode: ");
  switch (ir_state.mode)
  {
  case opmode_kCool:
    seek += sprintf(&result[seek], "COOL");
    break;

  case opmode_kDry:
    seek += sprintf(&result[seek], "DRY");
    break;

  case opmode_kFan:
    seek += sprintf(&result[seek], "FAN");
    break;

  case opmode_kHeat:
    seek += sprintf(&result[seek], "HEAT");
    break;

  case opmode_kOff:
    seek += sprintf(&result[seek], "OFF");
    break;

  default:
    seek += sprintf(&result[seek], "AUTO");
    break;
  }

  seek += sprintf(&result[seek], ", temp: %.01f, fan_speed: ", ir_state.degrees);

  switch (ir_state.fanspeed)
  {
  case fanspeed_kHigh:
    seek += sprintf(&result[seek], "HIGH");
    break;

  case fanspeed_kLow:
    seek += sprintf(&result[seek], "LOW");
    break;

  case fanspeed_kMax:
    seek += sprintf(&result[seek], "MAX");
    break;

  case fanspeed_kMedium:
    seek += sprintf(&result[seek], "MEDIUM");
    break;

  case fanspeed_kMediumHigh:
    seek += sprintf(&result[seek], "MEDIUM HIGH");
    break;

  case fanspeed_kMin:
    seek += sprintf(&result[seek], "MIN");
    break;

  default:
    seek += sprintf(&result[seek], "AUTO");
    break;
  }

  seek += sprintf(&result[seek], "\r\n");

  os_trace(result);
}

uint32_t calcUSecPeriod(uint32_t hz, bool use_offset)
{
  if (hz == 0)
    hz = 1; // Avoid Zero hz. Divide by Zero is nasty.
  uint32_t period =
      (1000000UL + hz / 2) / hz; // The equiv of round(1000000/hz).
  // Apply the offset and ensure we don't result in a <= 0 value.
  if (use_offset)
    return max((uint32_t)1, period + periodOffset);
  else
    return max((uint32_t)1, period);
}

void enableIROut(uint32_t freq, uint8_t duty)
{
  // Set the duty cycle to use if we want freq. modulation.
  if (modulation)
  {
    _dutycycle = min(duty, kDutyMax);
  }
  else
  {
    _dutycycle = kDutyMax;
  }
  if (freq < 1000) // Were we given kHz? Supports the old call usage.
    freq *= 1000;
#ifdef UNIT_TEST
  _freq_unittest = freq;
#endif // UNIT_TEST
  uint32_t period = calcUSecPeriod(freq, true);
  // Nr. of uSeconds the LED will be on per pulse.
  onTimePeriod = (period * _dutycycle) / kDutyMax;
  // Nr. of uSeconds the LED will be off per pulse.
  offTimePeriod = period - onTimePeriod;
}

void ledOn()
{
#ifndef UNIT_TEST
  bl_gpio_output_set(IRpin, outputOn);
#endif
}

void ledOff()
{
#ifndef UNIT_TEST
  bl_gpio_output_set(IRpin, outputOff);
#endif
}

uint16_t mark(uint16_t usec) {
  // Handle the simple case of no required frequency modulation.
  if (!modulation || _dutycycle >= 100) {
    ledOn();
    bl_timer_delay_us(usec);
    ledOff();
    return 1;
  }

  // Not simple, so do it assuming frequency modulation.
  uint16_t counter = 0;
  IRtimer_t usecTimer;
  reset(&usecTimer);
  // Cache the time taken so far. This saves us calling time, and we can be
  // assured that we can't have odd math problems. i.e. unsigned under/overflow.
  uint32_t elapsed1 = elapsed(&usecTimer);

  while (elapsed1 < usec) {  // Loop until we've met/exceeded our required time.
    ledOn();
    // Calculate how long we should pulse on for.
    // e.g. Are we to close to the end of our requested mark time (usec)?
    bl_timer_delay_us(min((uint32_t)onTimePeriod, usec - elapsed1));
    ledOff();
    counter++;
    if (elapsed1 + onTimePeriod >= usec)
      return counter;  // LED is now off & we've passed our allotted time.
    // Wait for the lesser of the rest of the duty cycle, or the time remaining.
    bl_timer_delay_us(
        min(usec - elapsed1 - onTimePeriod, (uint32_t)offTimePeriod));
    elapsed1 = elapsed(&usecTimer);  // Update & recache the actual elapsed time.
  }
  return counter;
}

void space(uint32_t time)
{
  ledOff();
  if (time == 0)
    return;
  bl_timer_delay_us(time);
}

void sendData(uint16_t onemark, uint32_t onespace, uint16_t zeromark,
              uint32_t zerospace, uint64_t data, uint16_t nbits,
              bool MSBfirst)
{
  if (nbits == 0) // If we are asked to send nothing, just return.
    return;
  if (MSBfirst)
  { // Send the MSB first.
    // Send 0's until we get down to a bit size we can actually manage.
    while (nbits > sizeof(data) * 8)
    {
      mark(zeromark);
      space(zerospace);
      nbits--;
    }
    // Send the supplied data.
    for (uint64_t mask = 1ULL << (nbits - 1); mask; mask >>= 1)
      if (data & mask)
      { // Send a 1
        mark(onemark);
        space(onespace);
      }
      else
      { // Send a 0
        mark(zeromark);
        space(zerospace);
      }
  }
  else
  { // Send the Least Significant Bit (LSB) first / MSB last.
    for (uint16_t bit = 0; bit < nbits; bit++, data >>= 1)
      if (data & 1)
      { // Send a 1
        mark(onemark);
        space(onespace);
      }
      else
      { // Send a 0
        mark(zeromark);
        space(zerospace);
      }
  }
}

void sendGenericBytes(const uint16_t headermark, const uint32_t headerspace,
                         const uint16_t onemark, const uint32_t onespace,
                         const uint16_t zeromark, const uint32_t zerospace,
                         const uint16_t footermark, const uint32_t gap,
                         const uint8_t *dataptr, const uint16_t nbytes,
                         const uint16_t frequency, const bool MSBfirst,
                         const uint16_t repeat, const uint8_t dutycycle) 
{
  // Setup
  enableIROut(frequency, dutycycle);
  // We always send a message, even for repeat=0, hence '<= repeat'.
  for (uint16_t r = 0; r <= repeat; r++) {
    // Header
    if (headermark) mark(headermark);
    if (headerspace) space(headerspace);

    // Data
    for (uint16_t i = 0; i < nbytes; i++)
      sendData(onemark, onespace, zeromark, zerospace, *(dataptr + i), 8,
               MSBfirst);

    // Footer
    if (footermark) mark(footermark);
    space(gap);
  }
}

void sendGeneric1(const uint16_t headermark, const uint32_t headerspace,
                  const uint16_t onemark, const uint32_t onespace,
                  const uint16_t zeromark, const uint32_t zerospace,
                  const uint16_t footermark, const uint32_t gap,
                  const uint32_t mesgtime, const uint64_t data,
                  const uint16_t nbits, const uint16_t frequency,
                  const bool MSBfirst, const uint16_t repeat,
                  const uint8_t dutycycle)
{
  os_trace("%s dutycycle: %u\r\n", __func__, dutycycle);
  vTaskEnterCritical();
  // Setup
  enableIROut(frequency, dutycycle);
  IRtimer_t usecs;

  // We always send a message, even for repeat=0, hence '<= repeat'.
  for (uint16_t r = 0; r <= repeat; r++)
  {
    reset(&usecs);

    // Header
    if (headermark)
      mark(headermark);
    if (headerspace)
      space(headerspace);

    // Data
    sendData(onemark, onespace, zeromark, zerospace, data, nbits, MSBfirst);

    // Footer
    if (footermark)
      mark(footermark);
    uint32_t elapsed1 = elapsed(&usecs);
    // Avoid potential unsigned integer underflow. e.g. when mesgtime is 0.
    if (elapsed1 >= mesgtime)
      space(gap);
    else
      space(max(gap, mesgtime - elapsed1));
  }
  vTaskExitCritical();
}

void sendGeneric(const uint16_t headermark, const uint32_t headerspace,
                 const uint16_t onemark, const uint32_t onespace,
                 const uint16_t zeromark, const uint32_t zerospace,
                 const uint16_t footermark, const uint32_t gap,
                 const uint64_t data, const uint16_t nbits,
                 const uint16_t frequency, const bool MSBfirst,
                 const uint16_t repeat, const uint8_t dutycycle)
{
  sendGeneric1(headermark, headerspace, onemark, onespace, zeromark, zerospace,
               footermark, gap, 0U, data, nbits, frequency, MSBfirst, repeat,
               dutycycle);
}

void IRsend_begin(void) 
{
#ifndef UNIT_TEST
  gpio_set_output(IRpin);
#endif
  ledOff();  // Ensure the LED is in a known safe state when we start.
}

void IRsend_init(uint16_t IRsendPin, bool inverted, bool use_modulation)
{
  periodOffset = kPeriodOffset;
  if (inverted)
  {
    outputOn = LOW;
    outputOff = HIGH;
  }
  else
  {
    outputOn = HIGH;
    outputOff = LOW;
  }
  modulation = use_modulation;
  if (modulation)
  {
    _dutycycle = kDutyDefault;
  }
  else
  {
    _dutycycle = kDutyMax;
  }

  // gpio_set_output(IRpin);
  // ledOn();
}