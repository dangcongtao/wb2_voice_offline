#include "ir_Argo.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"

// Constants
// using SPACE modulation. MARK is always const 400u
const uint16_t kArgoHdrMark = 6400;
const uint16_t kArgoHdrSpace = 3300;
const uint16_t kArgoBitMark = 400;
const uint16_t kArgoOneSpace = 2200;
const uint16_t kArgoZeroSpace = 900;
const uint32_t kArgoGap = kDefaultMessageGap;  // Made up value. Complete guess.
const uint8_t  kArgoSensorCheck = 52;  // Part of the sensor message check calc.
const uint8_t  kArgoSensorFixed = 0b011;
const uint8_t  kArgoWrem3Preamble = 0b1011;
const uint8_t  kArgoWrem3Postfix_Timer = 0b1;
const uint8_t  kArgoWrem3Postfix_ACControl = 0b110000;

static uint8_t IRArgoAC_getChecksum(const uint8_t state[],
                                                const uint16_t length) {
  if (length < 1) {
    return -1;
  }
  return (state[length - 2] >> 2) + (state[length - 1] << 6);
}

static uint8_t IRArgoAC_calcChecksum(const uint8_t state[],
                                                 const uint16_t length) {
  // Corresponds to byte 11 being constant 0b01
  // Only add up bytes to 9. byte 10 is 0b01 constant anyway.
  // Assume that argo array is MSB first (left)
  return sumBytesState(state, length - 2, 2);
}

static bool IRArgoAC_validChecksum(const uint8_t state[],
                                    const uint16_t length) {
  return (IRArgoAC_getChecksum(state, length) == IRArgoAC_calcChecksum(state, length));
}

bool decodeArgo(decode_results *results, uint16_t offset,
                        const uint16_t nbits,
                        const bool strict) {
  if (strict && nbits != kArgoBits) return false;

  // Match Header + Data
  if (!matchGenericState(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    kArgoHdrMark, kArgoHdrSpace,
                    kArgoBitMark, kArgoOneSpace,
                    kArgoBitMark, kArgoZeroSpace,
                    0, 0,  // Footer (None, allegedly. This seems very wrong.)
                    true, 50, 0, false)) return false;

  // Compliance
  // Verify we got a valid checksum.
  if (strict && !IRArgoAC_validChecksum(results->state, kArgoStateLength)) {
    os_trace("%s invalid checksum\r\n", __func__);
    return false;
  }
  // Success
  os_trace("%s success\r\n", __func__);
  results->decode_type = ARGO;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

static argoIrMessageType_t _messageType;
static uint16_t _length = kArgoStateLength;

void IRArgoAC_setRaw(const uint8_t state[], const uint16_t length, state_ir_t *ir_state) {
    ArgoProtocol _;
    memset(&_, 0, sizeof(ArgoProtocol));
    memcpy(_.raw, state, length);
    if (length == kArgoShortStateLength) {
        _messageType = IFEEL_TEMP_REPORT;
    }
    _messageType = AC_CONTROL;
    _length = length;

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kArgoTempDelta;
    
}

static bool IRArgoAC_WREM3_hasValidPreamble(const uint8_t state[],
                                      const uint16_t length) {
  if (length < 1) {
    return false;
  }
  uint8_t preamble = state[0] & 0x0F;
  return preamble == kArgoWrem3Preamble;
}

static argoIrMessageType_t IRArgoAC_WREM3_getMessageType(
  const uint8_t state[], const uint16_t length) {
  if (length < 1) {
    return (argoIrMessageType_t)(-1);
  }
  return (argoIrMessageType_t)(state[0] >> 6);
}

static uint8_t IRArgoAC_WREM3_calcChecksum(const uint8_t state[],
                                                      const uint16_t length) {
  if (length < 1) {
    return -1;  // Nothing to compute on
  }

  uint16_t payloadSizeBits = (length - 1) * 8;  // Last byte carries checksum

  argoIrMessageType_t msgType = IRArgoAC_WREM3_getMessageType(state, length);
  if (msgType == IFEEL_TEMP_REPORT) {
    payloadSizeBits += 5;  // For WREM3::iFeel the checksum is 3-bit
  } else if (msgType == TIMER_COMMAND) {
    payloadSizeBits += 3;  // For WREM3::Timer the checksum is 5-bit
  }  // Otherwise: full 8-bit checksum

  uint8_t checksum = sumBytesState(state, payloadSizeBits / 8, 0);

  // Add stray bits from last byte to the checksum (if any)
  const uint8_t maskPayload = 0xFF >> (8 - (payloadSizeBits % 8));
  checksum += (state[length-1] & maskPayload);

  const uint8_t maskChecksum = 0xFF >> (payloadSizeBits % 8);
  return checksum & maskChecksum;
}

uint8_t IRArgoAC_WREM3_getChecksum(const uint8_t state[],
                                                     const uint16_t length) {
  if (length < 1) {
    return -1;
  }
  argoIrMessageType_t msgType = IRArgoAC_WREM3_getMessageType(state, length);
  if (msgType == IFEEL_TEMP_REPORT) {
    return (state[length - 1] & 0b11100000) >> 5;
  }
  if (msgType == TIMER_COMMAND) {
    return state[length - 1] >> 3;
  }
  return (state[length - 1]);
}

static bool IRArgoAC_WREM3_validChecksum(const uint8_t state[],
                                    const uint16_t length) {
  return (IRArgoAC_WREM3_getChecksum(state, length) == IRArgoAC_WREM3_calcChecksum(state, length));
}

static bool IRArgoAC_WREM3_isValidWrem3Message(const uint8_t state[],
                                         const uint16_t nbits,
                                         bool verifyChecksum) {
  if ((nbits % 8) != 0) {
    return false;  // WREM-3 protocol always has a full byte length commands
  }
  uint16_t stateLengthBytes = min((uint16_t)(nbits / 8),
                                       kStateSizeMax);
  if (!IRArgoAC_WREM3_hasValidPreamble(state, stateLengthBytes)) {
    return false;
  }

  argoIrMessageType_t messageType = IRArgoAC_WREM3_getMessageType(state, stateLengthBytes);

  switch (messageType) {
    case AC_CONTROL :
      if (stateLengthBytes != kArgo3AcControlStateLength) { return false; }
      break;
  case CONFIG_PARAM_SET:
      if (stateLengthBytes != kArgo3ConfigStateLength) { return false; }
      break;
  case TIMER_COMMAND:
      if (stateLengthBytes != kArgo3TimerStateLength) { return false; }
      break;
    case IFEEL_TEMP_REPORT:
      if (stateLengthBytes != kArgo3iFeelReportStateLength) { return false; }
      break;
    default:
      return false;
  }

  // Compliance: Verify we got a valid checksum.
  if (verifyChecksum &&
      !IRArgoAC_WREM3_validChecksum(state, stateLengthBytes)) {
    return false;
  }
  return true;
}


bool decodeArgoWREM3(decode_results *results, uint16_t offset,
                             const uint16_t nbits,
                             const bool strict) {
  if (strict
      && nbits != kArgo3AcControlStateLength * 8
      && nbits != kArgo3ConfigStateLength * 8
      && nbits != kArgo3iFeelReportStateLength * 8
      && nbits != kArgo3TimerStateLength * 8) {
    return false;
  }

  uint16_t bytesRead = matchGenericState(results->rawbuf + offset, results->state,
                  results->rawlen - offset, nbits,
                  kArgoHdrMark, kArgoHdrSpace,
                  kArgoBitMark, kArgoOneSpace,
                  kArgoBitMark, kArgoZeroSpace,
                  kArgoBitMark, kArgoGap,  // difference vs decodeArgo
                  true, 50, 0,
                  false);
  if (!bytesRead) {
    return false;
  }

  // If 'strict', assert it is a valid WREM-3 'model' protocolar message
  // vs. just 'any ARGO'
  if (strict &&
      !IRArgoAC_WREM3_isValidWrem3Message(results->state, nbits, true)) {
        os_trace("%s invValid Wrem3 Message\r\n", __func__);
    return false;
  }
  os_trace("%s success\r\n", __func__);
  // Success: Matched ARGO protocol and WREM3-model
  // Note that unfortunately decode_type does not allow to persist model...
  // so we will be re-detecting it later :)
  results->decode_type = ARGO;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}
