#ifndef __IR_REMOTE_WB2_H
#define __IR_REMOTE_WB2_H

typedef enum {
  UNKNOWN = -1,
  UNUSED = 0,
  DAIKIN2,
  DAIKIN216,
  NEC,
  DAIKIN160,
  DAIKIN128,  // (5) // power toggle
  DAIKIN152,
  SAMSUNG,
  WHYNTER,
  AIWA_RC_T501,
  LG,  // (10)
  SANYO,
  FUNIKI, // MIDEA - COMFEE 
  DISH,
  DAIKIN,
  COOLIX,  // (15)
  SHARP,
  DENON,
  KELVINATOR,
  GREE,
  MITSUBISHI_AC,  // (20)
  RCMM,
  SANYO_LC7461,
  LG2,
  SHERWOOD,
  PRONTO,  // Technically not a protocol, but an encoding. (25)
  NEC_LIKE,
  ARGO,
  SHARP_AC,
  TCL112AC,
  RAW,  // Technically not a protocol, but an encoding. (30)
  TOSHIBA_AC,
  GLOBALCACHE,  // Technically not a protocol, but an encoding.
  FUJITSU_AC,
  MIDEA,
  MAGIQUEST,  // (35)
  LASERTAG,
  CARRIER_AC,
  HAIER_AC,
  MITSUBISHI2,
  HITACHI_AC,  // (40)
  TECHNIBEL_AC,
  HITACHI_AC2,
  GICABLE,
  HAIER_AC_YRW02,
  WHIRLPOOL_AC,  // (45)
  SAMSUNG_AC,
  LUTRON,
  ELECTRA_AC,
  PANASONIC_AC,
  PIONEER,  // (50)
  MWM,
  RC5,
  VESTEL_AC,
  TECO,  // (55)
  SAMSUNG36,
  TROTEC,
  LEGOPF,
  MITSUBISHI_HEAVY_88,
  MITSUBISHI_HEAVY_152,  // 60
  RC6,
  NIKAI,
  GOODWEATHER,
  INAX,
  SONY,  // 65
  NEOCLIMA,
  DAIKIN176,
  PANASONIC,
  AMCOR,
  JVC,  // 70
  MITSUBISHI136,
  MITSUBISHI112,
  MIRAGE,
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
  ELITESCREENS,  // 85
  CORONA_AC,
  MIDEA24,
  ZEPEAL,
  SANYO_AC,
  VOLTAS,  // 90
  METZ,
  TRANSCOLD,
  HITACHI_AC1,
  HITACHI_AC424,
  HITACHI_AC344,  // 95
  PANASONIC_AC32,
  MILESTAG2,
  ECOCLIM,
  XMP,
  TRUMA,  // 100
  HAIER_AC176,
  AQUA,
  CASPER,
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
  MITSUBISHI,
  NAGAKAWA,
  TEKNOPOINT,
  // Add new entries before this one, and update it to point to the last entry.
  kLastDecodeType = TEKNOPOINT,
} decode_type_t;

#define kSingleRepeat 1

#define kNoRepeat  0
#define kDaikinStateLength (35)
#define kDaikinBits (kDaikinStateLength * 8)
#define kDaikinStateLengthShort (kDaikinStateLength - 8)
#define kDaikinBitsShort (kDaikinStateLengthShort * 8)
#define kDaikinDefaultRepeat (kNoRepeat)
#define kDaikin2StateLength (39)
#define kDaikin2Bits (kDaikin2StateLength * 8)
#define kDaikin2DefaultRepeat (kNoRepeat)
#define kDaikin64Bits (64)
#define kDaikin64DefaultRepeat (kNoRepeat)
#define kDaikin160StateLength (20)
#define kDaikin160Bits (kDaikin160StateLength * 8)
#define kDaikin160DefaultRepeat (kNoRepeat)
#define kDaikin128StateLength (16)
#define kDaikin128Bits (kDaikin128StateLength * 8)
#define kDaikin128DefaultRepeat (kNoRepeat)
#define kDaikin152StateLength (19)
#define kDaikin152Bits (kDaikin152StateLength * 8)
#define kDaikin152DefaultRepeat (kNoRepeat)
#define kDaikin176StateLength (22)
#define kDaikin176Bits (kDaikin176StateLength * 8)
#define kDaikin176DefaultRepeat (kNoRepeat)
#define kDaikin200StateLength (25)
#define kDaikin200Bits (kDaikin200StateLength * 8)
#define kDaikin200DefaultRepeat (kNoRepeat)
#define kDaikin216StateLength (27)
#define kDaikin216Bits (kDaikin216StateLength * 8)
#define kDaikin216DefaultRepeat (kNoRepeat)
#define kDaikin312StateLength (39)
#define kDaikin312Bits (kDaikin312StateLength * 8)
#define kDaikin312DefaultRepeat (kNoRepeat)

#define kToshibaACStateLength 9
#define kToshibaACBits  (kToshibaACStateLength * 8)
#define kToshibaACMinRepeat  kSingleRepeat
#define kToshibaACStateLengthShort  (kToshibaACStateLength - 2)
#define kToshibaACBitsShort  (kToshibaACStateLengthShort * 8)
#define kToshibaACStateLengthLong  (kToshibaACStateLength + 1)
#define kToshibaACBitsLong (kToshibaACStateLengthLong * 8)

#define kGreeStateLength  8
#define kGreeBits  (kGreeStateLength * 8)
#define kGreeDefaultRepeat  kNoRepeat

#define kMitsubishi112StateLength  14
#define kMitsubishi112Bits  (kMitsubishi112StateLength * 8)
#define kMitsubishi112MinRepeat kNoRepeat

#define kNagakawaStateLength  14

#define kTcl112AcStateLength  14
#define kTcl112AcBits  (kTcl112AcStateLength * 8)
#define kTcl112AcDefaultRepeat (kNoRepeat)

#define kMideaBits  48
#define kMideaMinRepeat  (kNoRepeat)
#define kMidea24Bits 24
#define kMidea24MinRepeat  kSingleRepeat

#define kSharpAddressBits  5
#define kSharpCommandBits  8
#define kSharpBits  (kSharpAddressBits + kSharpCommandBits + 2)  // 15
#define kSharpAcStateLength  13
#define kSharpAcBits  (kSharpAcStateLength * 8)  // 104
#define kSharpAcDefaultRepeat  kNoRepeat

/* hitachi */
#define kHitachiAcStateLength     28
#define kHitachiAcBits            kHitachiAcStateLength*8
#define kHitachiAcDefaultRepeat   kNoRepeat
#define kHitachiAc1StateLength    13
#define kHitachiAc1Bits           kHitachiAc1StateLength*8
#define kHitachiAc2StateLength    53
#define kHitachiAc2Bits           kHitachiAc2StateLength*8
#define kHitachiAc3StateLength    27
#define kHitachiAc3Bits           kHitachiAc3StateLength*8
#define kHitachiAc3MinStateLength 15
#define kHitachiAc3MinBits        kHitachiAc3MinStateLength*8
#define kHitachiAc344StateLength  43
#define kHitachiAc344Bits         kHitachiAc344StateLength*8
#define kHitachiAc424StateLength  53
#define kHitachiAc424Bits         kHitachiAc424StateLength*8


#endif