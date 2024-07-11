#ifndef __IRRECV_H_
#define __IRRECV_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ir_LG.h"
#include "ir_Samsung.h"
#include "IRsend.h"

#define DEBUG_IR_RCV_ENABLE    0
#define os_trace_rcv if (DEBUG_IR_RCV_ENABLE == 1) printf

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

#define kCasperAcStateLength  21
#define kCasperAcBits   (kCasperAcStateLength * 8)



#define DECODE_LG   1
#define DECODE_NEC  0
#define DECODE_SAMSUNG 1
#define DECODE_SAMSUNG_AC 1
#define DECODE_FUNIKI 1
#define DECODE_FUNIKIAC104 1
#define DECODE_CASPER   1
#define DECODE_TOSHIBA  1
#define DECODE_DAIKIN   1
#define DECODE_DAIKIN2  1
#define DECODE_DAIKIN152  1
#define DECODE_DAIKIN160  1
#define DECODE_DAIKIN216  1
#define DECODE_DAIKIN128  1
#define DECODE_DAIKIN176  1
#define DECODE_DAIKIN64  1
#define DECODE_DAIKIN312  1
#define DECODE_GREE   1
#define DECODE_MITSUBISHI112  1
#define DECODE_MITSUBISHIHEAVY  1
#define DECODE_NAGAKAWA   1
#define DECODE_TCL112AC   1
#define DECODE_MIDEA    1
#define DECODE_SHARP    1
#define DECODE_HITACHI_AC 1
#define DECODE_HITACHI_AC424 1

#define DECODE_HITACHI_AC344 1 
#define DECODE_PANASONIC_AC 1
#define DECODE_PANASONIC_AC32 1
#define DECODE_ELECTRA_AC 1
#define DECODE_AIRWELL 1
#define DECODE_FUJITSU_AC 1
#define DECODE_VESTEL_AC 1
#define DECODE_SANYO 1
#define DECODE_MITSUBISHI136 1
#define DECODE_MITSUBISHI_AC 1
#define DECODE_KELVINATOR 0
#define DECODE_CARRIER_AC64 1
#define DECODE_HAIER_AC 1
#define DECODE_HAIER_AC_YRW02 1
#define DECODE_HAIER_AC176 1
#define DECODE_HAIER_AC160 1
#define DECODE_TECO 1
#define DECODE_MIRAGE 1
#define DECODE_AMCOR 1
#define DECODE_COOLIX 1
#define DECODE_COOLIX48 1
#define DECODE_WHIRLPOOL_AC 1
#define DECODE_NEOCLIMA 1
#define DECODE_YORK 1
#define DECODE_CORONA_AC 1
#define DECODE_TROTEC 1
#define DECODE_TROTEC_3550 1
#define DECODE_ARGO 1
#define DECODE_DELONGHI_AC 1
#define DECODE_BOSCH144 1
#define DECODE_VOLTAS 1
#define DECODE_HITACHI_AC296 1
#define DECODE_GOOD_WEATHER 1
#define DECODE_SAIJODENKI 1


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

#if DECODE_FUNIKIAC104
bool decodeFunikiAC144(decode_results *results, uint16_t offset, const uint16_t nbits, const bool strict);
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
#if DECODE_DAIKIN176
bool decodeDaikin176(decode_results *results, uint16_t offset,
                             const uint16_t nbits,
                             const bool strict);
#endif
#if DECODE_DAIKIN312
bool decodeDaikin312(decode_results *results, uint16_t offset,
                             const uint16_t nbits,
                             const bool strict);
#endif

#if DECODE_KELVINATOR
bool decodeKelvinator(decode_results *results, uint16_t offset,
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

#if DECODE_MITSUBISHIHEAVY
bool decodeMitsubishiHeavy(decode_results* results, uint16_t offset,
                                   const uint16_t nbits, const bool strict);
                    
bool decodeMitsubishiHeavy160(decode_results* results, uint16_t offset,
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
#if DECODE_HITACHI_AC
bool decodeHitachiAC(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict,
                             const bool MSBfirst);
#endif
#if DECODE_HITACHI_AC424
bool decodeHitachiAC424(decode_results *results, uint16_t offset,
                                const uint16_t nbits,
                                const bool strict);
#endif
#if DECODE_HITACHI_AC296
bool decodeHitachiAc296(decode_results *results, uint16_t offset,
                                const uint16_t nbits,
                                const bool strict);
#endif
#if DECODE_GOOD_WEATHER
bool decodeGoodweather(decode_results* results, uint16_t offset,
                               const uint16_t nbits,
                               const bool strict);
#endif

#if DECODE_PANASONIC_AC
bool decodePanasonicAC(decode_results *results, uint16_t offset,
                               const uint16_t nbits, const bool strict);
#endif

#if DECODE_PANASONIC_AC32
bool decodePanasonicAC32(decode_results *results, uint16_t offset,
                                 const uint16_t nbits, const bool strict);
#endif

#if DECODE_ELECTRA_AC
bool decodeElectraAC(decode_results *results, uint16_t offset,
                             const uint16_t nbits,
                             const bool strict);
#endif

#if DECODE_FUJITSU_AC
bool decodeFujitsuAC(decode_results* results, uint16_t offset,
                             const uint16_t nbits,
                             const bool strict);
bool decodeFujitsuNewAC(decode_results* results, uint16_t offset,
                             const uint16_t nbits,
                             const bool strict);
#endif

#if DECODE_AIRWELL
bool decodeAirwell(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_VESTEL_AC
bool decodeVestelAc(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_MITSUBISHI_AC
bool decodeMitsubishiAC(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_MITSUBISHI136
bool decodeMitsubishi136(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_SANYO
bool decodeSanyoAc(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);

bool decodeSanyoAc88(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict);

bool decodeSanyoAc152(decode_results *results, uint16_t offset,
                              const uint16_t nbits, const bool strict);
#endif

#if DECODE_CARRIER_AC64
bool decodeCarrierAC64(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);

bool decodeCarrierAC(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict);
#endif

#if DECODE_HAIER_AC
bool decodeHaierAC(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_HAIER_AC_YRW02
bool decodeHaierACYRW02(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_HAIER_AC176
bool decodeHaierAC176(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_HAIER_AC160
bool decodeHaierAC160(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_TECO
bool decodeTeco(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_MIRAGE
bool decodeMirage(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_AMCOR
bool decodeAmcor(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_COOLIX
bool decodeCOOLIX(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif
#if DECODE_COOLIX48
bool decodeCoolix48(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_WHIRLPOOL_AC
bool decodeWhirlpoolAC(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_NEOCLIMA
bool decodeNeoclima(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_YORK
bool decodeYork(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_CORONA_AC
bool decodeCoronaAc(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_TROTEC
bool decodeTrotec(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_TROTEC_3550
bool decodeTrotec3550(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_ARGO
bool decodeArgoWREM3(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
bool decodeArgo(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_DELONGHI_AC
bool decodeDelonghiAc(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_BOSCH144
bool decodeBosch144(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_VOLTAS
bool decodeVoltas(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict);
#endif

#if DECODE_SAIJODENKI
bool decodeSaijoDenki(decode_results *results, uint16_t offset,
                        const uint16_t nbits, const bool strict);
#endif

bool matchSpace(uint32_t measured, uint32_t desired, uint8_t tolerance,
                        int16_t excess);

bool matchAtLeast(uint32_t measured, uint32_t desired,
                          uint8_t tolerance, uint16_t delta);
bool matchAtLeastLimit(uint32_t measured, uint32_t lower_limit);

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

  uint16_t matchManchester(volatile const uint16_t *data_ptr,
                           uint64_t *result_ptr,
                           const uint16_t remaining,
                           const uint16_t nbits,
                           const uint16_t hdrmark,
                           const uint32_t hdrspace,
                           const uint16_t clock_period,
                           const uint16_t footermark,
                           const uint32_t footerspace,
                           const bool atleast,
                           const uint8_t tolerance,
                           const int16_t excess,
                           const bool MSBfirst,
                           const bool GEThomas);

void LG_setRaw(const uint32_t new_code, const decode_type_t protocol, state_ir_t *ir_state);
bool isValidLgAc(void);

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
/* Hitachi */
void HitachiAC1_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);

void HitachiAC344_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void HitachiAC424_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);


/*------------SET_INFO----------------*/
void LG_setInfo(const uint32_t new_code, const decode_type_t protocol, state_ir_t *ir_state);
void Casper_setInfo(const uint8_t* new_code, state_ir_t *ir_state);
void Funiki_setInfo(const uint8_t* new_code, const decode_type_t protocol, state_ir_t *ir_state);
void Samsung_setInfo(const uint8_t* new_code, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin2_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin152_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin160_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin128_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin64_setInfo(const uint64_t new_state, const decode_type_t protocol, state_ir_t *ir_state);
void Daikin216_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void IRDaikin176_setRaw(const uint8_t new_code[], state_ir_t *ir_state);
void Tcl112_setInfo(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state);
void Mitsubishi112_setInfo(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state);
void Toshiba_setInfo(const uint8_t* newState, const uint16_t length, const decode_type_t protocol, state_ir_t *ir_state);
void Gree_setInfo(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state);
void IRKelvinatorAC_setInfo(const uint8_t new_code[], state_ir_t *ir_state);
void SharpAc_setInfo(const uint8_t* newState, const decode_type_t protocol, state_ir_t *ir_state);
void HitachiAC1_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void HitachiAC344_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void HitachiAC424_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void Nagakawa_setInfo(const uint8_t *newState, const decode_type_t protocol, state_ir_t *ir_state);
// Panasonic
void PanasonicAC_setInfo(const uint8_t* newState, state_ir_t *ir_state);
void IRPanasonicAc32_setInfo(const uint32_t state,  state_ir_t *ir_state);
void ElectraAc_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);

void IRMitsubishiHeavy88Ac_setInfo(const uint8_t *data, state_ir_t *ir_state);
void IRMitsubishiHeavy152Ac_setInfo(const uint8_t *data, state_ir_t *ir_state);
void AirwellAc_setInfo(const uint64_t state, state_ir_t *ir_state);
bool IRFujitsuAC_setInfo(const uint8_t newState[], const uint16_t length, state_ir_t *ir_state);
void VestelAc_setInfo(const uint64_t newState, state_ir_t *ir_state);
void IRSanyoAc_setInfo(const uint8_t newState[], state_ir_t *ir_state);

void IRMitsubishiAC_setInfo(const uint8_t *data, state_ir_t *ir_state);
void IRMitsubishi136_setInfo(const uint8_t *data, state_ir_t *ir_state);
void IRCarrierAc64_setInfo(const uint64_t state, state_ir_t *ir_state);

void IRHaierACYRW02_setRaw(const uint8_t new_code[], state_ir_t *ir_state);
void IRHaierAC_setRaw(const uint8_t new_code[], state_ir_t *ir_state);
void IRHaierAC_setRaw(const uint8_t new_code[], state_ir_t *ir_state);
void IRTecoAc_setInfo(const uint64_t new_code, state_ir_t *ir_state);
void IRMirageAc_setInfo(const uint8_t *data, state_ir_t *ir_state);
void IRAmcorAc_setInfo(const uint8_t state[], state_ir_t *ir_state);
void IRWhirlpoolAc_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void IRNeoclimaAc_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void Midea_setInfo(const uint64_t new_code, const decode_type_t protocol, state_ir_t *ir_state);
void IRCoronaAc_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void IRTrotec_setRaw(const uint8_t state[], state_ir_t *ir_state);
void IRTrotec3550_setRaw(const uint8_t state[], state_ir_t *ir_state);
void IRArgoAC_setRaw(const uint8_t state[], const uint16_t length, state_ir_t *ir_state);
void IRDelonghiAc_setRaw(const uint64_t state, state_ir_t *ir_state);
void IRBosch144AC_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void IRVoltas_setRaw(const uint8_t new_code[], state_ir_t *ir_state);
void HitachiAC264_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void HitachiAc296_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void GoodweatherAc_setRaw(const uint64_t state, state_ir_t *ir_state);
void Casper2024Ac_setInfo(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state);
void SaijoDenki_setInfo(const uint8_t state[], state_ir_t *ir_state);
void MitsubishiHeavy160_setInfo(const uint8_t state[], state_ir_t *ir_state);
void IRCarrierAc32_setInfo(const uint64_t state, state_ir_t *ir_state);
void FunikiAc104_setInfo(const uint8_t* newState, state_ir_t *ir_state);

// void enableIRIn(const bool pullup);
void ir_loop(void);
void IRrecv_init(const uint32_t recvpin, const uint16_t bufsize,
                 const uint8_t timeout, const bool save_buffer);

void Irrecv_pause(void);
void resume(void);

void m_ir_init(void);
void m_ir_deinit(void);
void IR_start_learn(void);
void IR_stop_learn(void);
void IR_start_learn_auto(void);
void IR_stop_learn_auto(void);
bool decode_for(decode_results *results, uint8_t max_skip);

#endif