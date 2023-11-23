#ifndef IR_NEC_H_
#define IR_NEC_H_

#include <stdint.h>
#include <stdbool.h>
#include "IRrecv.h"
#include "IRutils.h"
#include "IRsend.h"

// Constants
#define kNecTick 560
#define kNecHdrMarkTicks 16
#define kNecHdrMark (kNecHdrMarkTicks * kNecTick)
#define kNecHdrSpaceTicks 8
#define kNecHdrSpace (kNecHdrSpaceTicks * kNecTick)
#define kNecBitMarkTicks 1
#define kNecBitMark (kNecBitMarkTicks * kNecTick)
#define kNecOneSpaceTicks 3
#define kNecOneSpace (kNecOneSpaceTicks * kNecTick)
#define kNecZeroSpaceTicks 1
#define kNecZeroSpace (kNecZeroSpaceTicks * kNecTick)
#define kNecRptSpaceTicks 4
#define kNecRptSpace (kNecRptSpaceTicks * kNecTick)
#define kNecRptLength 4
#define kNecMinCommandLengthTicks 193
#define kNecMinCommandLength (kNecMinCommandLengthTicks * kNecTick)
#define kNecMinGap (kNecMinCommandLength - (kNecHdrMark + kNecHdrSpace + kNECBits * (kNecBitMark + kNecOneSpace) + kNecBitMark))
#define kNecMinGapTicks (kNecMinCommandLengthTicks - (kNecHdrMarkTicks + kNecHdrSpaceTicks + kNECBits * (kNecBitMarkTicks + kNecOneSpaceTicks) + kNecBitMarkTicks))

// IR codes and structure for kids ALOKA SleepyLights LED Lamp.
// https://aloka-designs.com/
// Ref: https://github.com/crankyoldgit/IRremoteESP8266/issues/1004
//
// May be useful for someone wanting to control the lamp.
//
// The lamp is toggled On and Off with the same power button.
// The colour, when selected, is the brightest and there are 4 levels of
// brightness that decrease on each send of the colour. A fifth send of the
// colour resets to brightest again.
//
// Remote buttons defined left to right, top line to bottom line on the remote.
#define kAlokaPower         0xFF609F
#define kAlokaLedWhite      0xFF906F
#define kAlokaLedGreen      0xFF9867
#define kAlokaLedBlue       0xFFD827
#define kAlokaLedPinkRed    0xFF8877
#define kAlokaLedRed        0xFFA857
#define kAlokaLedLightGreen 0xFFE817
#define kAlokaLedMidBlue    0xFF48B7
#define kAlokaLedPink       0xFF6897
#define kAlokaLedOrange     0xFFB24D
#define kAlokaLedYellow     0xFF00FF
#define kAlokaNightFade     0xFF50AF
#define kAlokaNightTimer    0xFF7887
#define kAlokaLedRainbow    0xFF708F
// Didn't have a better description for it...
#define kAlokaLedTreeGrow   0xFF58A7

#endif