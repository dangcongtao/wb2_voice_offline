#ifndef __IRRECV_H_
#define __IRRECV_H_

#include <stdint.h>
#include <stdbool.h>

#include "ir_LG.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) < (b)) ? (b) : (a))
#define MS_TO_USEC(x) ((x) * 1000U)  // Convert milli-Seconds to micro-Seconds.

#define DECODE_LG   1

typedef enum {
  UNKNOWN = -1,
  UNUSED = 0,
  RC5,
  RC6,
  NEC,
  SONY,
  PANASONIC,  // (5)
  JVC,
  SAMSUNG,
  WHYNTER,
  AIWA_RC_T501,
  LG,  // (10)
  SANYO,
  MITSUBISHI,
  DISH,
  SHARP,
  COOLIX,  // (15)
  DAIKIN,
  DENON,
  KELVINATOR,
  SHERWOOD,
  MITSUBISHI_AC,  // (20)
  RCMM,
  SANYO_LC7461,
  RC5X,
  GREE,
  PRONTO,  // Technically not a protocol, but an encoding. (25)
  NEC_LIKE,
  ARGO,
  TROTEC,
  NIKAI,
  RAW,  // Technically not a protocol, but an encoding. (30)
  GLOBALCACHE,  // Technically not a protocol, but an encoding.
  TOSHIBA_AC,
  FUJITSU_AC,
  MIDEA,
  MAGIQUEST,  // (35)
  LASERTAG,
  CARRIER_AC,
  HAIER_AC,
  MITSUBISHI2,
  HITACHI_AC,  // (40)
  HITACHI_AC1,
  HITACHI_AC2,
  GICABLE,
  HAIER_AC_YRW02,
  WHIRLPOOL_AC,  // (45)
  SAMSUNG_AC,
  LUTRON,
  ELECTRA_AC,
  PANASONIC_AC,
  PIONEER,  // (50)
  LG2,
  MWM,
  DAIKIN2,
  VESTEL_AC,
  TECO,  // (55)
  SAMSUNG36,
  TCL112AC,
  LEGOPF,
  MITSUBISHI_HEAVY_88,
  MITSUBISHI_HEAVY_152,  // 60
  DAIKIN216,
  SHARP_AC,
  GOODWEATHER,
  INAX,
  DAIKIN160,  // 65
  NEOCLIMA,
  DAIKIN176,
  DAIKIN128,
  AMCOR,
  DAIKIN152,  // 70
  MITSUBISHI136,
  MITSUBISHI112,
  HITACHI_AC424,
  SONY_38K,
  EPSON,  // 75
  SYMPHONY,
  HITACHI_AC3,
  DAIKIN64,
  AIRWELL,
  DELONGHI_AC,  // 80
  DOSHISHA,
  MULTIBRACKETS,
  CARRIER_AC40,
  CARRIER_AC64,
  HITACHI_AC344,  // 85
  CORONA_AC,
  MIDEA24,
  ZEPEAL,
  SANYO_AC,
  VOLTAS,  // 90
  METZ,
  TRANSCOLD,
  TECHNIBEL_AC,
  MIRAGE,
  ELITESCREENS,  // 95
  PANASONIC_AC32,
  MILESTAG2,
  ECOCLIM,
  XMP,
  TRUMA,  // 100
  HAIER_AC176,
  TEKNOPOINT,
  KELON,
  TROTEC_3550,
  SANYO_AC88,  // 105
  BOSE,
  ARRIS,
  RHOSS,
  AIRTON,
  COOLIX48,  // 110
  HITACHI_AC264,
  KELON168,
  HITACHI_AC296,
  DAIKIN200,
  HAIER_AC160,  // 115
  CARRIER_AC128,
  TOTO,
  CLIMABUTLER,
  TCL96AC,
  BOSCH144,  // 120
  SANYO_AC152,
  DAIKIN312,
  GORENJE,
  WOWWEE,
  CARRIER_AC84,  // 125
  YORK,
  // Add new entries before this one, and update it to point to the last entry.
  kLastDecodeType = YORK,
} decode_type_t;

#define IR_RX 1
#define DECODE_AC 1

#define kHeader  2        // Usual nr. of header entries.
#define kFooter  2        // Usual nr. of footer (stop bits) entries.
#define kStartOffset  1   // Usual rawbuf entry to start from.

#define kMarkExcess  50

#define kHitachiAc2StateLength  53
#define kStateSizeMax   53

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
    struct {
      uint64_t value;    // Decoded value
      uint32_t address;  // Decoded device address.
      uint32_t command;  // Decoded command.
    };
    uint8_t state[kStateSizeMax];  // Multi-byte results.
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

#if DECODE_LG
bool decodeLG(decode_results *results, uint16_t,
            const uint16_t nbits,
            const bool strict);
#endif

void enableIRIn(const bool pullup);
void ir_loop(void);
void IRrecv_init(const uint32_t recvpin, const uint16_t bufsize,
                 const uint8_t timeout, const bool save_buffer);

#endif