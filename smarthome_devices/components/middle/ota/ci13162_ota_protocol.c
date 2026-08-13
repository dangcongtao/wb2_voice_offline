/**
 * @file ci13162_ota_protocol.c
 * @brief CI13162 UART OTA frame codec implementation.
 */
#include "ci13162_ota_protocol.h"

#include <string.h>

uint16_t ci13162_ota_crc16_ccitt(const uint8_t *data, size_t length, uint16_t crc)
{
    size_t i;
    uint8_t bit;

    if ((data == NULL) && (length != 0U)) {
        return crc;
    }

    for (i = 0U; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x8000U) != 0U) {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}

void ci13162_ota_put_u16_le(uint8_t *dst, uint16_t value)
{
    if (dst == NULL) {
        return;
    }
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)((value >> 8) & 0xFFU);
}

void ci13162_ota_put_u32_le(uint8_t *dst, uint32_t value)
{
    if (dst == NULL) {
        return;
    }
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)((value >> 8) & 0xFFU);
    dst[2] = (uint8_t)((value >> 16) & 0xFFU);
    dst[3] = (uint8_t)((value >> 24) & 0xFFU);
}

uint16_t ci13162_ota_get_u16_le(const uint8_t *src)
{
    if (src == NULL) {
        return 0U;
    }
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

uint32_t ci13162_ota_get_u32_le(const uint8_t *src)
{
    if (src == NULL) {
        return 0U;
    }
    return ((uint32_t)src[0]) |
           ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

int32_t ci13162_ota_frame_pack(uint8_t *out,
                               size_t out_capacity,
                               uint8_t msg_type,
                               uint8_t cmd,
                               uint8_t seq,
                               const uint8_t *payload,
                               uint16_t payload_len,
                               size_t *out_len)
{
    size_t frame_len;
    uint16_t crc;
    uint8_t body_prefix[3];

    if ((out == NULL) || (out_len == NULL)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    if ((payload_len > 0U) && (payload == NULL)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }

    frame_len = (size_t)CI13162_OTA_FRAME_MIN_LEN + (size_t)payload_len;
    if (out_capacity < frame_len) {
        return CI13162_OTA_PROTO_ERR_SPACE;
    }

    out[0] = CI13162_OTA_FRAME_HEAD0;
    out[1] = CI13162_OTA_FRAME_HEAD1;
    ci13162_ota_put_u16_le(&out[2], payload_len);
    out[4] = msg_type;
    out[5] = cmd;
    out[6] = seq;
    if (payload_len > 0U) {
        memcpy(&out[7], payload, payload_len);
    }

    body_prefix[0] = msg_type;
    body_prefix[1] = cmd;
    body_prefix[2] = seq;
    crc = ci13162_ota_crc16_ccitt(body_prefix, 3U, 0U);
    if (payload_len > 0U) {
        crc = ci13162_ota_crc16_ccitt(payload, payload_len, crc);
    }
    ci13162_ota_put_u16_le(&out[7U + payload_len], crc);
    out[9U + payload_len] = CI13162_OTA_FRAME_TAIL;

    *out_len = frame_len;
    return CI13162_OTA_PROTO_OK;
}

int32_t ci13162_ota_rx_parser_init(ci13162_ota_rx_parser_t *parser,
                                   uint8_t *buffer,
                                   size_t capacity,
                                   size_t max_payload)
{
    if ((parser == NULL) || (buffer == NULL) || (capacity < CI13162_OTA_FRAME_MIN_LEN)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }

    parser->buf = buffer;
    parser->capacity = capacity;
    parser->length = 0U;
    parser->max_payload = (max_payload == 0U) ? CI13162_OTA_DEFAULT_MAX_PAYLOAD : max_payload;
    parser->pending_discard = 0U;
    return CI13162_OTA_PROTO_OK;
}

static void ci13162_ota_rx_discard(ci13162_ota_rx_parser_t *parser, size_t count);

void ci13162_ota_rx_parser_reset(ci13162_ota_rx_parser_t *parser)
{
    if (parser == NULL) {
        return;
    }
    parser->length = 0U;
    parser->pending_discard = 0U;
}

int32_t ci13162_ota_rx_parser_push(ci13162_ota_rx_parser_t *parser,
                                   const uint8_t *data,
                                   size_t length)
{
    size_t discard;

    if ((parser == NULL) || (parser->buf == NULL)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    if ((length > 0U) && (data == NULL)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    if (length == 0U) {
        return CI13162_OTA_PROTO_OK;
    }

    if (parser->pending_discard > 0U) {
        discard = (parser->pending_discard < parser->length)
                      ? parser->pending_discard
                      : parser->length;
        ci13162_ota_rx_discard(parser, discard);
        parser->pending_discard -= discard;
        if (parser->pending_discard != 0U) {
            return CI13162_OTA_PROTO_ERR_FORMAT;
        }
    }

    if ((parser->length + length) > parser->capacity) {
        return CI13162_OTA_PROTO_ERR_OVERFLOW;
    }

    memcpy(parser->buf + parser->length, data, length);
    parser->length += length;
    return CI13162_OTA_PROTO_OK;
}

static void ci13162_ota_rx_discard(ci13162_ota_rx_parser_t *parser, size_t count)
{
    if ((parser == NULL) || (count == 0U)) {
        return;
    }
    if (count >= parser->length) {
        parser->length = 0U;
        return;
    }
    memmove(parser->buf, parser->buf + count, parser->length - count);
    parser->length -= count;
}

int32_t ci13162_ota_rx_parser_pop(ci13162_ota_rx_parser_t *parser,
                                  ci13162_ota_frame_t *frame)
{
    size_t i;
    uint16_t payload_len;
    size_t frame_len;
    uint16_t got_crc;
    uint16_t calc_crc;
    uint8_t body_prefix[3];

    if ((parser == NULL) || (parser->buf == NULL) || (frame == NULL)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }

    if (parser->pending_discard > 0U) {
        ci13162_ota_rx_discard(parser, parser->pending_discard);
        parser->pending_discard = 0U;
    }

    while (parser->length >= CI13162_OTA_FRAME_MIN_LEN) {
        /* Find header A5 0F. */
        i = 0U;
        while (i + 1U < parser->length) {
            if ((parser->buf[i] == CI13162_OTA_FRAME_HEAD0) &&
                (parser->buf[i + 1U] == CI13162_OTA_FRAME_HEAD1)) {
                break;
            }
            i++;
        }
        if (i > 0U) {
            /* Keep a trailing A5 that might start the next header. */
            if ((i == parser->length) &&
                (parser->buf[parser->length - 1U] == CI13162_OTA_FRAME_HEAD0)) {
                parser->buf[0] = CI13162_OTA_FRAME_HEAD0;
                parser->length = 1U;
                return CI13162_OTA_PROTO_NEED_MORE;
            }
            if ((i == parser->length) ||
                ((i + 1U >= parser->length) &&
                 (parser->buf[parser->length - 1U] != CI13162_OTA_FRAME_HEAD0))) {
                parser->length = 0U;
                return CI13162_OTA_PROTO_NEED_MORE;
            }
            ci13162_ota_rx_discard(parser, i);
            if (parser->length < CI13162_OTA_FRAME_MIN_LEN) {
                return CI13162_OTA_PROTO_NEED_MORE;
            }
        }

        if (parser->length < 4U) {
            return CI13162_OTA_PROTO_NEED_MORE;
        }

        payload_len = ci13162_ota_get_u16_le(&parser->buf[2]);
        if ((size_t)payload_len > parser->max_payload) {
            /* Resync past the bad header. */
            ci13162_ota_rx_discard(parser, 1U);
            continue;
        }

        frame_len = (size_t)CI13162_OTA_FRAME_MIN_LEN + (size_t)payload_len;
        if (frame_len > parser->capacity) {
            ci13162_ota_rx_discard(parser, 1U);
            continue;
        }
        if (parser->length < frame_len) {
            return CI13162_OTA_PROTO_NEED_MORE;
        }

        if (parser->buf[frame_len - 1U] != CI13162_OTA_FRAME_TAIL) {
            ci13162_ota_rx_discard(parser, 1U);
            continue;
        }

        body_prefix[0] = parser->buf[4];
        body_prefix[1] = parser->buf[5];
        body_prefix[2] = parser->buf[6];
        calc_crc = ci13162_ota_crc16_ccitt(body_prefix, 3U, 0U);
        if (payload_len > 0U) {
            calc_crc = ci13162_ota_crc16_ccitt(&parser->buf[7], payload_len, calc_crc);
        }
        got_crc = ci13162_ota_get_u16_le(&parser->buf[7U + payload_len]);
        if (got_crc != calc_crc) {
            ci13162_ota_rx_discard(parser, 1U);
            continue;
        }

        frame->msg_type = parser->buf[4];
        frame->cmd = parser->buf[5];
        frame->seq = parser->buf[6];
        frame->payload_len = payload_len;
        frame->payload = (payload_len > 0U) ? &parser->buf[7] : NULL;
        parser->pending_discard = frame_len;
        return CI13162_OTA_PROTO_OK;
    }

    /* Keep a possible partial header byte. */
    if ((parser->length == 1U) && (parser->buf[0] != CI13162_OTA_FRAME_HEAD0)) {
        parser->length = 0U;
    }
    return CI13162_OTA_PROTO_NEED_MORE;
}

static int32_t ci13162_ota_tx_all(ci13162_ota_tx_fn_t tx,
                                  void *user,
                                  const void *data,
                                  size_t length)
{
    int32_t ret;

    if (tx == NULL) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    if ((length > 0U) && (data == NULL)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    if (length == 0U) {
        return CI13162_OTA_PROTO_OK;
    }

    ret = tx(data, length, user);
    if (ret < 0) {
        return CI13162_OTA_PROTO_ERR_IO;
    }
    return CI13162_OTA_PROTO_OK;
}

int32_t ci13162_ota_frame_send_iov(ci13162_ota_tx_fn_t tx,
                                   void *user,
                                   uint8_t msg_type,
                                   uint8_t cmd,
                                   uint8_t seq,
                                   const ci13162_ota_iovec_t *parts,
                                   size_t part_count)
{
    uint8_t header[7];
    uint8_t trailer[3];
    uint8_t body_prefix[3];
    uint16_t payload_len;
    uint16_t crc;
    size_t i;
    int32_t ret;

    if (tx == NULL) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    if ((part_count > 0U) && (parts == NULL)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }

    payload_len = 0U;
    for (i = 0U; i < part_count; i++) {
        if ((parts[i].length > 0U) && (parts[i].data == NULL)) {
            return CI13162_OTA_PROTO_ERR_ARG;
        }
        if ((uint32_t)payload_len + (uint32_t)parts[i].length > 0xFFFFU) {
            return CI13162_OTA_PROTO_ERR_SPACE;
        }
        payload_len = (uint16_t)(payload_len + parts[i].length);
    }

    header[0] = CI13162_OTA_FRAME_HEAD0;
    header[1] = CI13162_OTA_FRAME_HEAD1;
    ci13162_ota_put_u16_le(&header[2], payload_len);
    header[4] = msg_type;
    header[5] = cmd;
    header[6] = seq;

    body_prefix[0] = msg_type;
    body_prefix[1] = cmd;
    body_prefix[2] = seq;
    crc = ci13162_ota_crc16_ccitt(body_prefix, 3U, 0U);
    for (i = 0U; i < part_count; i++) {
        if (parts[i].length > 0U) {
            crc = ci13162_ota_crc16_ccitt(parts[i].data, parts[i].length, crc);
        }
    }
    ci13162_ota_put_u16_le(&trailer[0], crc);
    trailer[2] = CI13162_OTA_FRAME_TAIL;

    ret = ci13162_ota_tx_all(tx, user, header, sizeof(header));
    if (ret != CI13162_OTA_PROTO_OK) {
        return ret;
    }
    for (i = 0U; i < part_count; i++) {
        if (parts[i].length > 0U) {
            ret = ci13162_ota_tx_all(tx, user, parts[i].data, parts[i].length);
            if (ret != CI13162_OTA_PROTO_OK) {
                return ret;
            }
        }
    }
    return ci13162_ota_tx_all(tx, user, trailer, sizeof(trailer));
}

int32_t ci13162_ota_frame_send(ci13162_ota_tx_fn_t tx,
                               void *user,
                               uint8_t msg_type,
                               uint8_t cmd,
                               uint8_t seq,
                               const uint8_t *payload,
                               uint16_t payload_len)
{
    ci13162_ota_iovec_t part;

    if ((payload_len > 0U) && (payload == NULL)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }

    if (payload_len == 0U) {
        return ci13162_ota_frame_send_iov(tx, user, msg_type, cmd, seq, NULL, 0U);
    }

    part.data = payload;
    part.length = payload_len;
    return ci13162_ota_frame_send_iov(tx, user, msg_type, cmd, seq, &part, 1U);
}

int32_t ci13162_ota_frame_send_write_rsp(ci13162_ota_tx_fn_t tx,
                                         void *user,
                                         uint8_t seq,
                                         uint32_t offset,
                                         const uint8_t *data,
                                         uint16_t data_len)
{
    uint8_t offset_le[4];
    ci13162_ota_iovec_t parts[2];

    if ((data_len > 0U) && (data == NULL)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }

    ci13162_ota_put_u32_le(offset_le, offset);
    parts[0].data = offset_le;
    parts[0].length = 4U;
    parts[1].data = data;
    parts[1].length = data_len;
    return ci13162_ota_frame_send_iov(tx,
                                      user,
                                      CI13162_OTA_MSG_ACK,
                                      CI13162_OTA_CMD_WRITE,
                                      seq,
                                      parts,
                                      2U);
}

int32_t ci13162_ota_build_block_info(uint8_t *out,
                                     size_t out_capacity,
                                     uint32_t base,
                                     uint32_t size,
                                     uint16_t block_crc,
                                     size_t *out_len)
{
    if ((out == NULL) || (out_len == NULL) || (out_capacity < 10U)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    ci13162_ota_put_u32_le(&out[0], base);
    ci13162_ota_put_u32_le(&out[4], size);
    ci13162_ota_put_u16_le(&out[8], block_crc);
    *out_len = 10U;
    return CI13162_OTA_PROTO_OK;
}

int32_t ci13162_ota_build_erase(uint8_t *out,
                                size_t out_capacity,
                                uint32_t base,
                                uint32_t size,
                                uint32_t chunk,
                                size_t *out_len)
{
    if ((out == NULL) || (out_len == NULL) || (out_capacity < 12U)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    ci13162_ota_put_u32_le(&out[0], base);
    ci13162_ota_put_u32_le(&out[4], size);
    ci13162_ota_put_u32_le(&out[8], chunk);
    *out_len = 12U;
    return CI13162_OTA_PROTO_OK;
}

int32_t ci13162_ota_build_change_baud(uint8_t *out,
                                      size_t out_capacity,
                                      uint32_t baud,
                                      size_t *out_len)
{
    if ((out == NULL) || (out_len == NULL) || (out_capacity < 4U)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    ci13162_ota_put_u32_le(out, baud);
    *out_len = 4U;
    return CI13162_OTA_PROTO_OK;
}

int32_t ci13162_ota_build_verify(uint8_t *out,
                                 size_t out_capacity,
                                 uint32_t base,
                                 uint32_t size,
                                 uint16_t block_crc,
                                 size_t *out_len)
{
    return ci13162_ota_build_block_info(out, out_capacity, base, size, block_crc, out_len);
}

int32_t ci13162_ota_parse_write_req(const uint8_t *payload,
                                    uint16_t payload_len,
                                    uint32_t *offset,
                                    uint32_t *size)
{
    if ((payload == NULL) || (offset == NULL) || (size == NULL) || (payload_len < 8U)) {
        return CI13162_OTA_PROTO_ERR_ARG;
    }
    *offset = ci13162_ota_get_u32_le(&payload[0]);
    *size = ci13162_ota_get_u32_le(&payload[4]);
    return CI13162_OTA_PROTO_OK;
}

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
int ci13162_ota_protocol_self_test(void)
{
    static const uint8_t hs_expected[] = {
        0xA5, 0x0F, 0x00, 0x00, 0xA1, 0x83, 0x00, 0x67, 0xC4, 0xFF
    };
    static const uint8_t baud_expected[] = {
        0xA5, 0x0F, 0x04, 0x00, 0xA0, 0x0B, 0x00, 0x00, 0x08, 0x07, 0x00, 0x9B, 0x36, 0xFF
    };
    uint8_t packed[32];
    uint8_t rx_buf[64];
    uint8_t baud_payload[4];
    size_t packed_len = 0U;
    size_t payload_len = 0U;
    ci13162_ota_rx_parser_t parser;
    ci13162_ota_frame_t frame;
    int failed = 0;
    uint16_t crc;

    crc = ci13162_ota_crc16_ccitt((const uint8_t *)"\xA1\x83\x00", 3U, 0U);
    if (crc != 0xC467U) {
        failed++;
    }

    if (ci13162_ota_frame_pack(packed,
                               sizeof(packed),
                               CI13162_OTA_MSG_REQ,
                               CI13162_OTA_CMD_HANDSHAKE,
                               0U,
                               NULL,
                               0U,
                               &packed_len) != CI13162_OTA_PROTO_OK) {
        failed++;
    } else if ((packed_len != sizeof(hs_expected)) ||
               (memcmp(packed, hs_expected, sizeof(hs_expected)) != 0)) {
        failed++;
    }

    if (ci13162_ota_build_change_baud(baud_payload, sizeof(baud_payload), 460800U, &payload_len) !=
        CI13162_OTA_PROTO_OK) {
        failed++;
    } else if (ci13162_ota_frame_pack(packed,
                                      sizeof(packed),
                                      CI13162_OTA_MSG_CMD,
                                      CI13162_OTA_CMD_CHANGE_BAUD,
                                      0U,
                                      baud_payload,
                                      (uint16_t)payload_len,
                                      &packed_len) != CI13162_OTA_PROTO_OK) {
        failed++;
    } else if ((packed_len != sizeof(baud_expected)) ||
               (memcmp(packed, baud_expected, sizeof(baud_expected)) != 0)) {
        failed++;
    }

    if (ci13162_ota_rx_parser_init(&parser, rx_buf, sizeof(rx_buf), 64U) != CI13162_OTA_PROTO_OK) {
        failed++;
    } else {
        /* Feed handshake frame in two fragments. */
        if (ci13162_ota_rx_parser_push(&parser, hs_expected, 4U) != CI13162_OTA_PROTO_OK) {
            failed++;
        }
        if (ci13162_ota_rx_parser_pop(&parser, &frame) != CI13162_OTA_PROTO_NEED_MORE) {
            failed++;
        }
        if (ci13162_ota_rx_parser_push(&parser, hs_expected + 4, sizeof(hs_expected) - 4U) !=
            CI13162_OTA_PROTO_OK) {
            failed++;
        }
        if (ci13162_ota_rx_parser_pop(&parser, &frame) != CI13162_OTA_PROTO_OK) {
            failed++;
        } else if ((frame.msg_type != CI13162_OTA_MSG_REQ) ||
                   (frame.cmd != CI13162_OTA_CMD_HANDSHAKE) ||
                   (frame.payload_len != 0U)) {
            failed++;
        }
    }

    return failed;
}
#endif
