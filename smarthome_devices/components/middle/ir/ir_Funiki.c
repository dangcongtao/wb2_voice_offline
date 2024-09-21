#include "ir_Funiki.h"
#include "system_typedef.h"

const uint16_t kFunikiAcBitMark = 4400;
const uint16_t kFunikiAcHdrSpace = 4400;
const uint16_t kFunikiHdr1Mark = 1100; ///< uSeconds.
const uint16_t kFunikiHdr1Space = 110; ///< uSeconds.
const uint16_t kFunikiHdr2Mark = 300;  ///< uSeconds.

const uint16_t kSamsungAcSectionMark = 600;

bool decodeFunikiAC(decode_results *results, uint16_t offset, const uint16_t nbits, const bool strict)
{

    // os_trace("decodeFunikiAC() offset: %d, nbits: %d, kFunikiBits: %d\r\n", offset, nbits, kFunikiBits);

    if (results->rawlen == 200 && nbits != kFunikiAcBits)
    {
        return false;
    }

    if (results->rawlen == 300 && nbits != kFunikiAcExtendedBits)
    {
        return false;
    }

    if (results->rawlen != 200 && results->rawlen != 300)
    {
        // os_trace("funiki len invalid\r\n");
        return false;
    }

    for (uint16_t pos = 0; pos <= (nbits / 8) - kFunikiAcSectionLength;
         pos += kFunikiAcSectionLength)
    {
        uint16_t used;
        // Section Header + Section Data (7 bytes) + Section Footer
        // used = matchGenericState(results->rawbuf + offset, (uint8_t*)(&results->state[pos]),
        //                     results->rawlen - offset, kFunikiAcSectionLength * 8,
        //                     4400, 4400,
        //                     480, 1500,
        //                     480, 460,
        //                     500, 5300,
        //                     pos + kFunikiAcSectionLength >= nbits / 8,
        //                     50, 0, false);

        used = matchGenericStateLimit(results->rawbuf + offset, (uint8_t *)(&results->state[pos]),
                                      results->rawlen - offset, kFunikiAcSectionLength * 8,
                                      4000, 4800, 4000, 4800,
                                      100, 900, 900, 2000,
                                      100, 900, 100, 899,
                                      100, 900, 4800, 5800,
                                      pos + kFunikiAcSectionLength >= nbits / 8,
                                      50, 0, false);

        if (used == 0)
        {
            os_trace_rcv("%s step 2 not used failed\r\n", __func__);
            return false;
        }
        offset += used;
    }

    if (strict)
    {
        if (memcmp(&results->state[0], &results->state[kFunikiAcSectionLength], kFunikiAcSectionLength))
        {
            os_trace("%s invalid checksum\r\n", __func__);
            return false;
        }
    }

    results->decode_type = FUNIKI;
    results->bits = nbits;

    // don set it to 0, It states will be 0
    // results->command = 0;
    // results->address = 0;

    os_trace("%s success\r\n", __func__);
    return true;
}

void sendFunikiAc(const uint8_t *data, uint16_t nbytes, uint16_t repeat)
{
    vTaskEnterCritical();
    uint16_t repeatHeaderMark = 0;
    uint8_t duty = kDutyDefault;

    enableIROut(38, 50);

    for (uint16_t offset = 0; offset < nbytes; offset += kFunikiAcSectionLength)
    {
        sendGenericBytes(4400, 4400, 480, 1500, 480, 460, 500, 5300, data + offset, kFunikiAcSectionLength, 38, false, repeat, 50);
    }

    // mark(9000);
    // space(4500);

    // for (uint16_t offset = 0; offset < nbytes;) {

    //     sendData(502, 1650, 501, 600, *(data + offset), 8, false);

    //     offset++;
    //     if (offset == 6) {
    //         mark(480);
    //         space(8000);
    //     } else if (offset == 14) {
    //         mark(480);
    //         space(8000);
    //     }
    // }

    // mark(500);
    vTaskExitCritical();
}

static uint8_t get_temp_funiki(uint8_t *states)
{
    // os_trace("temp_huy: %u\r\n", ((~(states[11])) & 0xF));
    uint8_t temp = 0;
    switch (states[11] & 0xF)
    {
    case 0xF:
        temp = 17;
        break;

    case 0x7:
        temp = 18;
        break;

    case 0x3:
        temp = 19;
        break;

    case 0xb:
        temp = 20;
        break;

    case 0x9:
        temp = 21;
        break;

    case 0x1:
        temp = 22;
        break;

    case 0x5:
        temp = 23;
        break;

    case 0xd:
        temp = 24;
        break;

    case 0xc:
        temp = 25;
        break;

    case 0x4:
        temp = 26;
        break;

    case 0x6:
        temp = 27;
        break;

    case 0xe:
        temp = 28;
        break;

    case 0xa:
        temp = 29;
        break;

    case 0x2:
        temp = 30;
        break;

    default:
        break;
    }
    return temp;
}

static opmode_t get_mode_funiki(uint8_t *states)
{
    // memset(str, 0, 20);
    switch ((states[11] >> 4) & 0xF)
    {
    case 0xF:
        // sprintf(str, "COOL");
        return opmode_kCool;
        break;

    case 0xE:
        // sprintf(str, "AUTO");
        return opmode_kAuto;
        break;

    case 0xC:
        // sprintf(str, "HEAT");
        return opmode_kHeat;
        break;

    case 0xD:
        if ((states[11] & 0xF) == 0x8)
        {
            // sprintf(str, "FAN");
            return opmode_kFan;
        }
        else
        {
            // sprintf(str, "DRY");
            return opmode_kDry;
        }
        break;

    default:
        return opmode_kAuto;
        break;
    }
}

static fanspeed_t get_fan_funiki(uint8_t *states)
{
    // memset(str, 0, 20);
    switch (states[9] & 0xF)
    {
    case 0x3:
        // sprintf(str, "HIGH");
        return fanspeed_kMax;
        break;

    case 0x6:
        // sprintf(str, "LOW");
        return fanspeed_kMin;
        break;

    case 0x5:
        // sprintf(str, "MEDIUM");
        return fanspeed_kMedium;
        break;

    case 0x2:
        //     sprintf(str, "AUTO");
        //     break;

        // case 0x7:

    default:
        return fanspeed_kAuto;
        break;
    }
}

static uint8_t is_power_on(uint8_t *states)
{
    if ((states[9] >> 4) & 0xF)
    {
        return 0;
    }
    return 1;
}

// static FunikiProtocol _;

static void Funiki_toCommon(const uint8_t *new_code, state_ir_t *ir_state)
{
    ir_state->protocol = FUNIKI;
    ir_state->mode = get_mode_funiki(new_code);
    ir_state->fanspeed = get_fan_funiki(new_code);
    ir_state->power = is_power_on(new_code);
    ir_state->degrees = get_temp_funiki(new_code);
}

void Funiki_setRaw(const uint8_t *new_code, const decode_type_t protocol, state_ir_t *ir_state)
{
    // memcpy(_.data.raw, new_code, kFunikiAcStateLength);
    Funiki_toCommon(new_code, ir_state);
    FunikiProtocol _;
    memset(&_, 0, sizeof(FunikiProtocol));
    memcpy(&_.data.raw, new_code, 12);
    // os_trace("fan: %u\r\n", _.data.fan);
    // _.data.raw = new_code;
}

void Funiki_setInfo(const uint8_t *new_code, const decode_type_t protocol, state_ir_t *ir_state)
{
    // memcpy(_.data.raw, new_code, kFunikiAcStateLength);
    uint8_t cmd_turbo[] = {0xAD, 0x52, 0xAF, 0x50, 0x45, 0xBA};
    if (!memcmp(new_code, cmd_turbo, 6))
    {
        ir_state->degrees = 0;
        os_trace("%s turbo cmd\r\n", __func__);
    }
    uint8_t cmd_swing[] = {0x4D, 0xB2, 0xD6, 0x29, 0x07, 0xF8};
    if (!memcmp(new_code, cmd_swing, 6))
    {
        ir_state->degrees = 0;
        os_trace("%s swing cmd\r\n", __func__);
    }
    uint8_t cmd_led[] = {0xAD, 0x52, 0xAF, 0x50, 0xA5, 0x5A};
    if (!memcmp(new_code, cmd_led, 6))
    {
        ir_state->degrees = 0;
        os_trace("%s led cmd\r\n", __func__);
    }
    uint8_t cmd_clean[] = {0xAD, 0x52, 0xAF, 0x50, 0x55, 0xAA};
    if (!memcmp(new_code, cmd_clean, 6))
    {
        ir_state->degrees = 0;
        os_trace("%s clean cmd\r\n", __func__);
    }

    Funiki_toCommon(new_code, ir_state);
    // FunikiProtocol _;
    // memset(&_, 0, sizeof(FunikiProtocol));
    // memcpy(&_.data.raw, new_code, 12);
    // os_trace("fan: %u\r\n", _.data.fan);

    // switch ((uint8_t)ir_state->mode)
    // {
    // case opmode_kDry:
    //     ir_state->mode = 1;
    //     break;

    // case opmode_kCool:
    //     ir_state->mode = 0;
    //     break;

    // case opmode_kAuto:
    //     ir_state->mode = 2;
    //     break;

    // case opmode_kHeat:
    //     ir_state->mode = 3;
    //     break;

    // case opmode_kFan:
    //     ir_state->mode = 4;
    //     break;

    // default:
    //     break;
    // }

    // switch (ir_state->fanspeed)
    // {
    // case fanspeed_kAuto:
    //     ir_state->fanspeed = 5;
    //     break;

    // case fanspeed_kMin:
    //     ir_state->fanspeed = 4;
    //     break;

    // case fanspeed_kMedium:
    //     ir_state->fanspeed = 2;
    //     break;

    // case fanspeed_kMax:
    //     ir_state->fanspeed = 1;
    //     break;

    // default:
    //     break;
    // }
}

static void hash_code(FunikiProtocol *data, state_ir_t state_ir)
{
    if (state_ir.power)
    {
        data->data.power = 0;
        switch ((uint8_t)state_ir.degrees)
        {
        case 17:
            data->data.temp = 0xF;
            break;

        case 18:
            data->data.temp = 0x7;
            break;

        case 19:
            data->data.temp = 0x3;
            break;

        case 20:
            data->data.temp = 0xb;
            break;

        case 21:
            data->data.temp = 0x9;
            break;

        case 22:
            data->data.temp = 0x1;
            break;

        case 23:
            data->data.temp = 0x5;
            break;

        case 24:
            data->data.temp = 0xD;
            break;

        case 25:
            data->data.temp = 0xC;
            break;

        case 26:
            data->data.temp = 0x4;
            break;

        case 27:
            data->data.temp = 0x6;
            break;

        case 28:
            data->data.temp = 0xE;
            break;

        case 29:
            data->data.temp = 0xA;
            break;

        case 30:
            data->data.temp = 0x2;
            break;

        default:
            break;
        }

        switch (state_ir.mode)
        {
        case opmode_kCool:
            data->data.mode = 0xF;
            break;

        case opmode_kAuto:
            data->data.mode = 0xE;
            break;

        case opmode_kHeat:
            data->data.mode = 0xC;
            break;

        case opmode_kFan:
            data->data.temp = 0x8;
        case opmode_kDry:
            data->data.mode = 0xD;
            break;

        default:
            break;
        }

        switch (state_ir.fanspeed)
        {
        case fanspeed_kMax:
            data->data.fan = 0x3;
            break;

        case fanspeed_kMin:
            data->data.fan = 0x6;
            break;

        case fanspeed_kMedium:
            data->data.fan = 0x5;
            break;

        case fanspeed_kAuto:
            data->data.fan = 0x2;
            break;

        default:
            break;
        }

        if (state_ir.mode == opmode_kAuto || state_ir.mode == opmode_kDry)
        {
            data->data.fan = 0x7;
        }
    }
    else
    {
        data->data.power = 1;
        data->data.temp = 0x8;
        data->data.fan = 0x1;
        data->data.mode = 0xF;
    }

    data->data.raw[0] = 0x4d;
    data->data.raw[1] = 0xb2;

    data->data.raw[2] = (~data->data.raw[3] & 0xFF);
    data->data.raw[4] = (~data->data.raw[5] & 0xFF);

    memcpy(&data->data.raw[6], &data->data.raw[0], 6);
}

void Funiki_send(state_ir_t state_ir)
{

    // switch ((uint8_t)state_ir.mode)
    // {
    // case 1:
    //     state_ir.mode = opmode_kDry;
    //     break;

    // case 0:
    //     state_ir.mode = opmode_kCool;
    //     break;

    // case 2:
    //     state_ir.mode = opmode_kAuto;
    //     break;

    // case 3:
    //     state_ir.mode = opmode_kHeat;
    //     break;

    // case 4:
    //     state_ir.mode = opmode_kFan;
    //     break;

    // default:
    //     break;
    // }

    // switch (state_ir.fanspeed)
    // {
    // case 5:
    //     state_ir.fanspeed = fanspeed_kAuto;
    //     break;

    // case 4:
    //     state_ir.fanspeed = fanspeed_kMin;
    //     break;

    // case 2:
    //     state_ir.fanspeed = fanspeed_kMedium;
    //     break;

    // case 1:
    //     state_ir.fanspeed = fanspeed_kMax;
    //     break;

    // default:
    //     break;
    // }

    FunikiProtocol _;
    hash_code(&_, state_ir);

    // os_trace("FUNIKI_SEND: 0x");
    // for (uint8_t i = 0; i < 18; i++) {
    //     if (i == 6) {
    //         os_trace("\t|\t");
    //     } else if (i == 12) {
    //             break;
    //     }
    //     os_trace("%02x", _.data.raw[i]);
    // }
    // os_trace("\r\n");

    sendFunikiAc(_.data.raw, kFunikiAcStateLength, 0);
}

const uint16_t kFunikiAC144HdrMark = 9600;  // kDaikinBitMark * 8
const uint16_t kFunikiAC144HdrSpace = 4500; // kDaikinBitMark * 4
const uint16_t kFunikiAC144BitMark = 428;
const uint16_t kFunikiAC144ZeroSpace = 428;
const uint16_t kFunikiAC144OneSpace = 1620;
const uint16_t kFunikiAC144HeaderLength = 10;

bool decodeFunikiAC144(decode_results *results, uint16_t offset, const uint16_t nbits, const bool strict)
{

    // if (results->rawlen < (2 * (nbits + kDaikinHeaderLength) +
    //                      kDaikinSections * (kHeader + kFooter) + kFooter - 1) +
    //                      offset)
    // {
    //     return false;
    // }

    // Compliance
    if (strict && nbits != kFuniki104Bits)
    {
        return false;
    }

    match_result_t data_result;

    // Header #1 - Doesn't count as data.
    data_result = matchDataLimit(&(results->rawbuf[offset]), kFunikiAC144HeaderLength,
                                 kFunikiAC144BitMark - 328, kFunikiAC144BitMark + 500, kFunikiAC144OneSpace - 500, kFunikiAC144OneSpace + 500,
                                 kFunikiAC144BitMark - 328, kFunikiAC144BitMark + 500, kFunikiAC144ZeroSpace - 328, kFunikiAC144ZeroSpace + 328,
                                 25, 50, false, true);

    if (data_result.success == false)
    {
        data_result = matchDataLimit(&(results->rawbuf[offset]), kFunikiAC144HeaderLength - 1,
                                     kFunikiAC144BitMark - 328, kFunikiAC144BitMark + 500, kFunikiAC144OneSpace - 500, kFunikiAC144OneSpace + 500,
                                     kFunikiAC144BitMark - 328, kFunikiAC144BitMark + 500, kFunikiAC144ZeroSpace - 328, kFunikiAC144ZeroSpace + 328,
                                     25, 50, false, true);
        if (data_result.success == false)
        {
            // os_trace("%s Fail\r\n", __func__);
            return false; // Fail
        }
    }

    offset += data_result.used;

    if (data_result.data)
    {
        // os_trace("%s The header bits should be zero.\r\n", __func__);
        return false; // The header bits should be zero.
    }

    uint16_t used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                                           results->rawlen - offset, kFuniki104Bits,
                                           kFunikiAC144HdrMark - 2000, kFunikiAC144HdrMark + 2000, kFunikiAC144HdrSpace - 1000, kFunikiAC144HdrSpace + 1000,
                                           kFunikiAC144BitMark - 328, kFunikiAC144BitMark + 500, kFunikiAC144OneSpace - 500, kFunikiAC144OneSpace + 500,
                                           kFunikiAC144BitMark - 328, kFunikiAC144BitMark + 500, kFunikiAC144ZeroSpace - 328, kFunikiAC144ZeroSpace + 328,
                                           kFunikiAC144BitMark - 328, kFunikiAC144BitMark + 500, 75000, 125000,
                                           true,
                                           45, 50, false);

    if (used == 0)
    {
        os_trace("%s used == 0\r\n", __func__);
        return false;
    }

    results->decode_type = FUNIKIAC104;
    results->bits = nbits;

    // don set it to 0, It states will be 0
    // results->command = 0;
    // results->address = 0;

    os_trace("%s success\r\n", __func__);
    return true;
}

void FunikiAc104_setInfo(const uint8_t *newState, state_ir_t *ir_state)
{
    os_trace("%s\r\n", __func__);
    FunikiAC104Protocol _;
    memcpy(_.raw, newState, kFuniki104StateLength);

    ir_state->protocol = FUNIKIAC104;
    ir_state->power = _.Power;
    ir_state->mode = _.Mode | (_.Auto << 2);
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + 16;
    os_trace("%s auto: %u\r\n", __func__, _.Auto);
}

uint8_t sendFunikiAc104AC(const uint8_t data[], const uint16_t nbytes)
{
    if (nbytes != kFuniki104StateLength)
    {
        os_trace("%s length data invalid\r\n", __func__);
        return 0;
    }
    vTaskEnterCritical();
    for (uint16_t r = 0; r <= kFuniki104DefaultRepeat; r++)
    {
        sendGeneric(0, 0, // No header for the header
                    kFunikiAC144BitMark, kFunikiAC144OneSpace, kFunikiAC144BitMark,
                    kFunikiAC144ZeroSpace, 0, 0,
                    (uint64_t)0b0000000000, kFunikiAC144HeaderLength, 38, false, 0, 50);

        sendGenericBytes(kFunikiAC144HdrMark, kFunikiAC144HdrSpace, kFunikiAC144BitMark,
                         kFunikiAC144OneSpace, kFunikiAC144BitMark, kFunikiAC144ZeroSpace,
                         kFunikiAC144BitMark, kDefaultMessageGap, data, nbytes,
                         38000, // Complete guess of the modulation frequency.
                         false, // Send data in LSB order per byte
                         0, 50);
    }
    vTaskExitCritical();
    return 1;
}

const uint16_t kFunikiAC83HdrMark = 9000;  // kDaikinBitMark * 8
const uint16_t kFunikiAC83HdrSpace = 4500; // kDaikinBitMark * 4
const uint16_t kFunikiAC83BitMark = 560;
const uint16_t kFunikiAC83ZeroSpace = 600;
const uint16_t kFunikiAC83OneSpace = 1600;
bool decodeFunikiAC83(decode_results *results, uint16_t offset,
                      const uint16_t nbits, const bool strict)
{
    uint16_t idx = results->rawlen;
    for (uint16_t i = 0; i < 5; i++)
    {
    }

    // Header + Data + Footer
    uint16_t my_offset = matchGenericStateLimit(results->rawbuf + offset, results->state,
                                                results->rawlen - offset, nbits - 3,
                                                kFunikiAC83HdrMark - 2000, kFunikiAC83HdrMark + 2000, kFunikiAC83HdrSpace - 1000, kFunikiAC83HdrSpace + 1000,
                                                kFunikiAC83BitMark - 400, kFunikiAC83BitMark + 350, kFunikiAC83OneSpace - 500, kFunikiAC83OneSpace + 500,
                                                kFunikiAC83BitMark - 400, kFunikiAC83BitMark + 350, kFunikiAC83ZeroSpace - 400, kFunikiAC83ZeroSpace + 400,
                                                0, 0, 0, 0,
                                                true, kUseDefTol, kMarkExcess, false);
    if (!my_offset)
    {
        // os_trace("%s match state failed\r\n", __func__);
        return false;
    }

    offset += my_offset;

    match_result_t data_result = matchDataLimit(&(results->rawbuf[offset]), 3,
                                                kFunikiAC83BitMark - 400, kFunikiAC83BitMark + 350, kFunikiAC83OneSpace - 500, kFunikiAC83OneSpace + 500,
                                                kFunikiAC83BitMark - 400, kFunikiAC83BitMark + 350, kFunikiAC83ZeroSpace - 400, kFunikiAC83ZeroSpace + 400,
                                                25, 50, false, true);
    if (data_result.success == false)
    {
        os_trace("%s Fail\r\n", __func__);
        return false; // Fail
    }

    results->state[kFuniki80StateLength - 1] = (uint8_t)data_result.data;

    // Compliance
    //   if (strict)
    //     if (!IRSanyoAc_validChecksum(results->state, nbits / 8)) return false;

    os_trace("%s success\r\n", __func__);
    // Success
    results->decode_type = FUNIKIAC83;
    results->bits = nbits;
    // No need to record the state as we stored it as we decoded it.
    // As we use result->state, we don't record value, address, or command as it
    // is a union data type.
    return true;
}

void FunikiAC83_setInfo(const uint8_t *newState, state_ir_t *ir_state)
{
    os_trace("%s\r\n", __func__);
    FunikiAC104Protocol _;
    memcpy(_.raw, newState, kFuniki80StateLength);

    ir_state->protocol = FUNIKIAC83;
    ir_state->power = _.Power;
    ir_state->mode = _.Mode | (_.Auto << 2);
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kFunikiAC83MinTemp;
    os_trace("%s SwingV: %u\r\n", __func__, _.SwingV);
}

static uint8_t sendFunikiAc83(const uint8_t data[])
{
    vTaskEnterCritical();
    for (uint16_t r = 0; r <= kFuniki80DefaultRepeat; r++)
    {

        sendGenericBytes(kFunikiAC83HdrMark, kFunikiAC83HdrSpace, kFunikiAC83BitMark,
                         kFunikiAC83OneSpace, kFunikiAC83BitMark, kFunikiAC83ZeroSpace,
                         0, 0, data, kFuniki80StateLength - 1,
                         38000, // Complete guess of the modulation frequency.
                         false, // Send data in LSB order per byte
                         0, 50);

        sendGeneric(0, 0, // No header for the header
                    kFunikiAC83BitMark, kFunikiAC83OneSpace, kFunikiAC83BitMark,
                    kFunikiAC83ZeroSpace, kFunikiAC83BitMark, kDefaultMessageGap,
                    (uint64_t)data[10], 3, 38, false, 0, 50); // len footer: 3
    }
    vTaskExitCritical();
    return 1;
}

void FunikiAc83_send(state_ir_t state_ir)
{
    FunikiAC104Protocol _;
    memset(&_, 0, sizeof(FunikiAC104Protocol));

    // state reset
    _.raw[3] = 0x60;
    _.raw[10] = 0x05;
    _.SwingV = 0x1;

    // settings
    _.Power = state_ir.power;
    _.Temp = (uint8_t)state_ir.degrees - 16;

    switch ((uint8_t)state_ir.mode)
    {
    case 3:
    case 1:
        _.Temp = 0;
    case 0:
    case 2:
        _.Mode = state_ir.mode;
        break;
    case 4:
        _.Temp = 0;
        _.Mode = 0;
        _.Auto = 1;
        break;

    default:
        _.Mode = 0; // cool
        break;
    }

    _.Fan = state_ir.fanspeed;
#if DEBUG_ENABLE == 1
    os_trace("%s: 0x ", __func__);
    for (uint8_t i = 0; i < kFuniki80StateLength; i++)
    {
        os_trace("%02X-", _.raw[i]);
    }
    os_trace("\r\n");
#endif
    sendFunikiAc83(_.raw);
}
