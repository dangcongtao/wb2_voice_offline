#ifndef __IR_REMOTE_WB2_H
#define __IR_REMOTE_WB2_H

typedef enum {
  UNKNOWN = -1,
  UNUSED,
  DAIKIN2,
  DAIKIN216,
  NEC,
  DAIKIN160,
  DAIKIN128,  // (5) // power toggle
  DAIKIN152,
  SAMSUNG,
  WHYNTER,
  AMCOR,
  COOLIX48,  // (10)
  CARRIER_AC64,
  SANYO_AC152, // MIDEA - COMFEE 
  CORONA_AC,
  DAIKIN,
  DELONGHI_AC,  // (15)
  ELECTRA_AC,
  FUJITSU_AC,
  SANYO_LC7461,
  GREE,
  YORK,  // (20)
  RCMM,
  KELVINATOR,
  LG2,
  SHERWOOD,
  NEOCLIMA,
  SANYO_AC88,
  SAMSUNG_AC,
  SHARP_AC,
  TCL112AC,
  RAW,  // Technically not a protocol, but an encoding. (30)
  PRONTO,  // Technically not a protocol, but an encoding.
  TROTEC,
  VESTEL_AC,
  WHIRLPOOL_AC,
  MAGIQUEST,  // (35)
  LASERTAG,
  CARRIER_AC,
  HAIER_AC,
  MITSUBISHI2,
  HITACHI_AC,  // (40)
  TECHNIBEL_AC,
  HITACHI_AC2,
  GICABLE,
  BOSCH144,
  UNKNOWN_98435,  // (45)
  ARGO,
  LUTRON,
  SHARP,
  NEC_LIKE,
  PIONEER,  // (50)
  MWM,
  RC5,
  DENON,
  TROTEC_3550,  // (55)
  SAMSUNG36,
  GLOBALCACHE,  // Technically not a protocol, but an encoding.
  LEGOPF,
  HAIER_AC176,
  ZEPEAL,  // 60
  RC6,
  NIKAI,
  UNKNOWN_9845,
  INAX,
  SONY,  // 65
  UNKNOWN_94843,
  UNKNOWN_3852,
  UNKNOWN_12,
  AIWA_RC_T501,
  JVC,  // 70
  UNKNOWN_8545,
  MITSUBISHI112,
  UNKNOWN_9483,
  SONY_38K,
  EPSON,  // 75
  SYMPHONY,
  HITACHI_AC3,
  UNKNOWN_9835,
  UNKNOWN_4934,
  COOLIX,
  DOSHISHA,
  MULTIBRACKETS,
  UNKNOWN_48532,
  UNKNOWN_9432,
  ELITESCREENS,  // 85
  DISH,
  MIDEA24,
  UNKNOWN_985,
  UNKNOWN_9543245,  // 90
  ARRIS,
  METZ,
  TRANSCOLD,
  HITACHI_AC1,
  HITACHI_AC424,
  HITACHI_AC344,  // 95
  AIRWELL,
  MILESTAG2,
  ECOCLIM,
  XMP,
  TRUMA,  // 100
  VOLTAS,
  AQUA,
  HAIER_AC160,
  CASPER,
  FUNIKI, // midea
  NAGAKAWA,  // 105
  PANASONIC_AC,
  PANASONIC_AC32, // power toggle
  TOSHIBA_AC,
  SANYO_AC,
  MITSUBISHI_HEAVY_88,
  MITSUBISHI_HEAVY_152,
  MITSUBISHI_AC,
  MITSUBISHI136,
  LG,
  HAIER_AC_YRW02, // AQUA
  TECO,
  MIRAGE,
  HITACHI_AC264,
  HITACHI_AC296,
  DAIKIN64,
  DAIKIN176,
  FUJITSU2_AC,
  FUJITSU_NEW_AC,
  GOODWEATHER,
  DAIKIN312,
  CASPER2024_AC,
  SAIJODENKI,
  MITSUBISHI_HEAVY_160,
  PANASONIC_AC_2,
  FUNIKIAC104,
  AKITO,
  FUNIKIAC83,
  DAIKIN200,
  // Add new entries before this one, and update it to point to the last entry.
  kLastDecodeType = FUNIKIAC83,
  MIDEA = 1934,
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

#define kMitsubishiMinRepeat kSingleRepeat
#define kMitsubishiACStateLength  18
#define kMitsubishiACBits  (kMitsubishiACStateLength * 8)
#define kMitsubishiACMinRepeat kSingleRepeat
#define kMitsubishi136StateLength  17
#define kMitsubishi136Bits (kMitsubishi136StateLength * 8)
#define kMitsubishi136MinRepeat  kNoRepeat
#define kMitsubishi112StateLength  14
#define kMitsubishi112Bits  (kMitsubishi112StateLength * 8)
#define kMitsubishi112MinRepeat kNoRepeat

#define kMitsubishiHeavy88StateLength  11
#define kMitsubishiHeavy88Bits (kMitsubishiHeavy88StateLength * 8)
#define kMitsubishiHeavy88MinRepeat kNoRepeat

#define kMitsubishiHeavy152StateLength 19
#define kMitsubishiHeavy152Bits (kMitsubishiHeavy152StateLength * 8)
#define kMitsubishiHeavy152MinRepeat kNoRepeat

#define kMitsubishiHeavy160StateLength 20
#define kMitsubishiHeavy160Bits (kMitsubishiHeavy160StateLength * 8)
#define kMitsubishiHeavy160MinRepeat kNoRepeat

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

#define kCarrierAc64Bits  64
#define kCarrierAc64MinRepeat  kNoRepeat

#define kCarrierAcBits  32
#define kCarrierAcMinRepeat  kNoRepeat

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
#define kHitachiAc424Bits         (kHitachiAc424StateLength*8)
#define kHitachiAc264StateLength  33
#define kHitachiAc264Bits  (kHitachiAc264StateLength * 8)
#define kHitachiAc296StateLength  37
#define kHitachiAc296Bits (kHitachiAc296StateLength * 8)

/* Panasonic*/
#define kPanasonicBits  (48)
#define kPanasonicManufacturer (0x4004)
#define kPanasonic40Manufacturer (0x34)
#define kPanasonic40Bits (40)
#define kPanasonicAcStateLength (27)
#define kPanasonicAcStateShortLength (16)
#define kPanasonicAcBits (kPanasonicAcStateLength * 8)
#define kPanasonicAcShortBits (kPanasonicAcStateShortLength * 8)
#define kPanasonicAcDefaultRepeat  kNoRepeat
#define kPanasonicAc32Bits (32)
#define kPanasonicAc32DefaultRepeat  kNoRepeat

/*Electra*/
#define kElectraAcStateLength 13
#define kElectraAcBits (kElectraAcStateLength * 8)
#define kElectraAcMinRepeat kNoRepeat

#define kAirwellBits  34
#define kAirwellMinRepeats  2

#define kFujitsuAcMinRepeat kNoRepeat
#define kFujitsuAcStateLength  16
#define kFujitsuAcStateLengthLong  18
#define kFujitsuAcStateLengthShort 7
#define kFujitsuAcBits (kFujitsuAcStateLength * 8)
#define kFujitsuAcLongBits (kFujitsuAcStateLengthLong * 8)
#define kFujitsuAcMinBits ((kFujitsuAcStateLengthShort - 1) * 8)

#define  kVestelAcBits  56

#define kSanyoAcStateLength  9
#define kSanyoAcBits (kSanyoAcStateLength * 8)
#define kSanyoAc88StateLength  11
#define kSanyoAc88Bits (kSanyoAc88StateLength * 8)
#define kSanyoAc88MinRepeat  2
#define kSanyoAc152StateLength  19
#define kSanyoAc152Bits (kSanyoAc152StateLength * 8)
#define kSanyoAc152MinRepeat kNoRepeat
#define kSanyoSA8650BBits  12
#define kSanyoLC7461AddressBits  13
#define kSanyoLC7461CommandBits  8
#define kSanyoLC7461Bits ((kSanyoLC7461AddressBits + kSanyoLC7461CommandBits) * 2)

#define kKelvinatorStateLength  16
#define kKelvinatorBits (kKelvinatorStateLength * 8)
#define kKelvinatorDefaultRepeat kNoRepeat

#define kHaierACStateLength (9)
#define kHaierACBits (kHaierACStateLength * 8)
#define kHaierAcDefaultRepeat (kNoRepeat)
#define kHaierACYRW02StateLength (14)
#define kHaierACYRW02Bits (kHaierACYRW02StateLength * 8)
#define kHaierAcYrw02DefaultRepeat (kNoRepeat)
#define kHaierAC160StateLength (20)
#define kHaierAC160Bits (kHaierAC160StateLength * 8)
#define kHaierAc160DefaultRepeat (kNoRepeat)
#define kHaierAC176StateLength (22)
#define kHaierAC176Bits (kHaierAC176StateLength * 8)
#define kHaierAc176DefaultRepeat (kNoRepeat)

#define kTecoBits  35
#define kTecoDefaultRepeat  kNoRepeat

#define kMirageStateLength  15
#define kMirageBits  (kMirageStateLength * 8)
#define kMirageMinRepeat  kNoRepeat

#define kAmcorStateLength  8
#define kAmcorBits  (kAmcorStateLength * 8)
#define kAmcorDefaultRepeat  kSingleRepeat

#define kCoolixBits  24
#define kCoolix48Bits (kCoolixBits * 2)
#define kCoolixDefaultRepeat kSingleRepeat

#define kWhirlpoolAcStateLength  21
#define kWhirlpoolAcBits (kWhirlpoolAcStateLength * 8)
#define kWhirlpoolAcDefaultRepeat  kNoRepeat

#define kNeoclimaStateLength  12
#define kNeoclimaBits (kNeoclimaStateLength * 8)
#define kNeoclimaMinRepeat  kNoRepeat

#define kYorkBits  136
#define kYorkStateLength  17

#define kCoronaAcStateLengthShort  7
#define kCoronaAcStateLength (kCoronaAcStateLengthShort * 3)
#define kCoronaAcBitsShort (kCoronaAcStateLengthShort * 8)
#define kCoronaAcBits (kCoronaAcStateLength * 8)

#define kTrotecStateLength  9
#define kTrotecBits  (kTrotecStateLength * 8)
#define kTrotecDefaultRepeat  kNoRepeat

#define kArgoStateLength (12)
#define kArgoShortStateLength (4)
#define kArgoBits (kArgoStateLength * 8)
#define kArgoShortBits (kArgoShortStateLength * 8)
#define kArgo3AcControlStateLength (6)  // Bytes
#define kArgo3iFeelReportStateLength (2)  // Bytes
#define kArgo3TimerStateLength (9)  // Bytes
#define kArgo3ConfigStateLength (4)  // Bytes
#define kArgoDefaultRepeat (kNoRepeat)

#define kDelonghiAcBits  64
#define kDelonghiAcDefaultRepeat  kNoRepeat

#define kBosch144StateLength  18
#define kBosch144Bits (kBosch144StateLength * 8)

#define kVoltasBits 80
#define kVoltasStateLength 10

#define kGoodweatherBits  48
#define kGoodweatherMinRepeat  kNoRepeat

#define kSaijoDenkiBits  72
#define kSaijoDenkiStateLength  9
#define kSaijoDenkiDefaultRepeat  kNoRepeat

#define kFuniki104Bits  104
#define kFuniki104StateLength  13
#define kFuniki104DefaultRepeat  kNoRepeat

#define kFuniki80Bits  83
#define kFuniki80StateLength  11
#define kFuniki80DefaultRepeat  kNoRepeat

#endif