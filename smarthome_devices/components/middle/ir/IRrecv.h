#ifndef __IRRECV_H_
#define __IRRECV_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ir_LG.h"
#include "ir_Samsung.h"
#include "IRsend.h"

#define MS_TO_USEC(x) ((x) * 1000U)  // Convert milli-Seconds to micro-Seconds.


#define kSamsungBits  32
#define kSamsung36Bits  36
#define kSamsungAcStateLength  14
#define kSamsungAcBits  (kSamsungAcStateLength * 8)
#define kSamsungAcStateLength  14
#define kSamsungAcBits (kSamsungAcStateLength * 8)
#define kSamsungAcExtendedStateLength 21
#define kSamsungAcExtendedBits  (kSamsungAcExtendedStateLength * 8)
#define kSamsungAcDefaultRepeat  kNoRepeat

#define kFunikiAcStateLength  12
#define kFunikiAcBits   (kFunikiAcStateLength * 8)

#define kFunikiAcExtendedStateLength  18
#define kFunikiAcExtendedBits   (kFunikiAcExtendedStateLength * 8)

#define kAquaAcStateLength  14
#define kAquaAcBits   (kAquaAcStateLength * 8)

#define kCasperAcStateLength  21
#define kCasperAcBits   (kCasperAcStateLength * 8)



#define DECODE_LG   1
#define DECODE_NEC  1
#define DECODE_SAMSUNG 1
#define DECODE_SAMSUNG_AC 1
#define DECODE_FUNIKI 1
#define DECODE_AQUA   1
#define DECODE_CASPER   1
#define DECODE_TOSHIBA  1
#define DECODE_DAIKIN   1
#define DECODE_DAIKIN2  1
#define DECODE_DAIKIN152  1
#define DECODE_DAIKIN160  1
#define DECODE_DAIKIN216  1
#define DECODE_DAIKIN128  1
#define DECODE_DAIKIN64  1
#define DECODE_GREE   1
#define DECODE_MITSUBISHI112  1
#define DECODE_NAGAKAWA   1
#define DECODE_TCL112AC   1
#define DECODE_MIDEA    1
#define DECODE_SHARP    1

#define IR_RX 1
#define DECODE_AC 1
#define DEBUG_LOG_RX  0

#define kHeader  2        // Usual nr. of header entries.
#define kFooter  2        // Usual nr. of footer (stop bits) entries.
#define kStartOffset  1   // Usual rawbuf entry to start from.

#define kMarkExcess  50
#define kRawBuf  100  // Default length of raw capture buffer
#define kRepeat UINT64_MAX

#define kHitachiAc2StateLength  53
#define kStateSizeMax   kHitachiAc2StateLength

/// Information for the interrupt handler
typedef struct {
  uint8_t recvpin;   // pin for IR data from detector
  uint8_t rcvstate;  // state machine
  uint16_t timer;    // state timer, counts 50uS ticks.
  uint16_t bufsize;  // max. nr. of entries in the capture buffer.
  uint16_t *rawbuf;  // raw data
  // uint16_t is used for rawlen as it saves 3 bytes of iram in the interrupt
  // handler. Don't ask why, I don't know. It just does.
  uint16_t rawlen;   // counter of entries in rawbuf.
  uint8_t overflow;  // Buffer overflow indicator.
  uint8_t timeout;   // Nr. of milliSeconds before we give up.
} irparams_t;

typedef struct {
  bool success;   // Was the match successful?
  uint64_t data;  // The data found.
  uint16_t used;  // How many buffer positions were used.
} match_result_t;

typedef struct {
    decode_type_t decode_type;  // NEC, SONY, RC5, UNKNOWN
  // value, address, & command are all mutually exclusive with state.
  // i.e. They MUST NOT be used at the same time as state, so we can use a union
  // structure to save us a handful of valuable bytes of memory.
  union {
    uint8_t state[kStateSizeMax];  // Multi-byte results.
    struct {
      uint64_t value;    // Decoded value
      uint32_t address;  // Decoded device address.
      uint32_t command;  // Decoded command.
    };
  };
  uint16_t bits;              // Number of bits in decoded value
  volatile uint16_t *rawbuf;  // Raw intervals in .5 us ticks
  uint16_t rawlen;            // Number of records in rawbuf.
  bool overflow;
  bool repeat;  // Is the result a repeat code?
} decode_results;


// receiver states
#define kIdleState  2
#define kMarkState  3
#define kSpaceState  4
#define kStopState  5
#define kTolerance 25   // default percent tolerance in measurements.
#define kUseDefTol 255  // Indicate to use the class default tolerance.
#define kRawTick     2  // Capture tick to uSec factor.

#define kTimeoutMs  15  // In MilliSeconds.
#define TIMEOUT_MS kTimeoutMs   // For legacy documentation.
#define kMaxTimeoutMs   (kRawTick * (UINT16_MAX / MS_TO_USEC(1)))

#define kNECBits 32

uint8_t _tolerance;

bool matchMark(uint32_t measured, uint32_t desired, uint8_t tolerance,
                       int16_t excess);
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
                              const bool MSBfirst);

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
                        const bool MSBfirst);

match_result_t matchData(
    volatile uint16_t *data_ptr, const uint16_t nbits, const uint16_t onemark,
    const uint32_t onespace, const uint16_t zeromark, const uint32_t zerospace,
    const uint8_t tolerance, const int16_t excess, const bool MSBfirst,
    const bool expectlastspace);

#if DECODE_LG
bool decodeLG(decode_results *results, uint16_t,
            const uint16_t nbits,
            const bool strict);
#endif

#if DECODE_NEC
bool decodeNEC(decode_results *results, uint16_t offset,
                       const uint16_t nbits, const bool strict);
#endif

#if DECODE_SAMSUNG_AC
bool decodeSamsungAC(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict);
#endif

#if DECODE_FUNIKI
bool decodeFunikiAC(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict);
#endif

#if DECODE_AQUA
bool decodeAquaAC(decode_results *results, uint16_t offset, const uint16_t nbits, const bool strict) ;
#endif

#if DECODE_CASPER
bool decodeCasperAC(decode_results *results, uint16_t offset, const uint16_t nbits, const bool strict);
#endif

#if DECODE_TOSHIBA
bool decodeToshibaAC(decode_results* results, uint16_t offset,
                             const uint16_t nbits, const bool strict);
#endif

#if DECODE_DAIKIN
bool decodeDaikin(decode_results *results, uint16_t offset,
                          const uint16_t nbits, const bool strict);
#endif

#if DECODE_DAIKIN2
bool decodeDaikin2(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_DAIKIN152
bool decodeDaikin152(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict);
#endif
#if DECODE_DAIKIN160
bool decodeDaikin160(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict);
#endif
#if DECODE_DAIKIN216
bool decodeDaikin216(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict);
#endif
#if DECODE_DAIKIN128
bool decodeDaikin128(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict);
#endif
#if DECODE_DAIKIN64
bool decodeDaikin64(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict);
#endif

#if DECODE_GREE
bool decodeGree(decode_results* results, uint16_t offset,
                        const uint16_t nbits, bool const strict);
#endif

#if DECODE_MITSUBISHI112
bool decodeMitsubishi112(decode_results *results, uint16_t offset,
                                 const uint16_t nbits, const bool strict);
#endif

#if DECODE_MIDEA
bool decodeMidea(decode_results *results, uint16_t offset,
                         const uint16_t nbits, const bool strict);
bool decodeMidea24(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict);
#endif

#if DECODE_SHARP
bool decodeSharpAc(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

bool matchSpace(uint32_t measured, uint32_t desired, uint8_t tolerance,
                        int16_t excess);

bool matchAtLeast(uint32_t measured, uint32_t desired,
                          uint8_t tolerance, uint16_t delta);

bool matchLimit(uint32_t measured, uint32_t lower_limit, uint32_t upper_limit);

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
    const bool expectlastspace);
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
                      const bool MSBfirst);

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
                        const bool MSBfirst );

void LG_setRaw(const uint32_t new_code, const decode_type_t protocol, state_ir_t *ir_state);
bool isValidLgAc(void);

void AQUA_setRaw(const uint8_t* new_code, const decode_type_t protocol, state_ir_t* state_ir);
void aqua_getString(void);

void Funiki_setRaw(const uint8_t* new_code, const decode_type_t protocol, state_ir_t *ir_state);
// void Funiki_getString(void);

void Casper_setRaw(const uint8_t* new_code, const decode_type_t protocol, state_ir_t *ir_state);

void Samsung_setRaw(const uint8_t* new_code, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);

void Toshiba_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state); 

void Daikin_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) ;
void Daikin2_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state) ;
void Daikin152_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin160_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin216_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin128_setRaw(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin64_setRaw(const uint64_t new_state, const decode_type_t protocol, state_ir_t *ir_state);

void Gree_setRaw(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state);

void Mitsubishi112_setRaw(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state);

void Nagakawa_setRaw(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state);

void Tcl112_setRaw(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state);

void Midea_setRaw(const uint64_t new_code, const decode_type_t protocol, state_ir_t *ir_state);

void SharpAc_setRaw(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state);

// void enableIRIn(const bool pullup);
void ir_loop(void);
void IRrecv_init(const uint32_t recvpin, const uint16_t bufsize,
                 const uint8_t timeout, const bool save_buffer);

void Irrecv_pause(void);
void resume(void);

void m_ir_init(void);
void IR_start_learn(void);

#endif