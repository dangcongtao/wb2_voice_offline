/**
 * @file ci13162_ota_protocol.h
 * @brief CI13162 UART OTA frame codec helpers (CRC16, pack/pop, fragment TX).
 */
#ifndef CI13162_OTA_PROTOCOL_H
#define CI13162_OTA_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CI13162_OTA_FRAME_HEAD0            0xA5U
#define CI13162_OTA_FRAME_HEAD1            0x0FU
#define CI13162_OTA_FRAME_TAIL             0xFFU
#define CI13162_OTA_FRAME_MIN_LEN          10U
#define CI13162_OTA_FRAME_HDR_LEN          7U /* A5 0F | len[2] | type | cmd | seq */

#define CI13162_OTA_MSG_CMD                0xA0U
#define CI13162_OTA_MSG_REQ                0xA1U
#define CI13162_OTA_MSG_ACK                0xA2U
#define CI13162_OTA_MSG_NOTIFY             0xA3U

#define CI13162_OTA_CMD_GET_INFO           0x04U
#define CI13162_OTA_CMD_CHECK_READY        0x05U
#define CI13162_OTA_CMD_BLOCK_INFO         0x06U
#define CI13162_OTA_CMD_ERASE              0x07U
#define CI13162_OTA_CMD_WRITE              0x08U
#define CI13162_OTA_CMD_BLOCK_DONE         0x09U
#define CI13162_OTA_CMD_VERIFY             0x0AU
#define CI13162_OTA_CMD_CHANGE_BAUD        0x0BU
#define CI13162_OTA_CMD_TEST_BAUD          0x0CU
#define CI13162_OTA_CMD_COMPLETE           0x0EU
#define CI13162_OTA_CMD_PROGRESS           0x11U
#define CI13162_OTA_CMD_SET_FW_FMT         0x12U
#define CI13162_OTA_CMD_HANDSHAKE          0x83U
#define CI13162_OTA_CMD_VERIFY_AGENT       0x8AU

#define CI13162_OTA_DEFAULT_MAX_PAYLOAD    8192U

/** Return codes for protocol helpers. */
#define CI13162_OTA_PROTO_OK               0
#define CI13162_OTA_PROTO_NEED_MORE        1
#define CI13162_OTA_PROTO_ERR_ARG          (-1)
#define CI13162_OTA_PROTO_ERR_SPACE        (-2)
#define CI13162_OTA_PROTO_ERR_FORMAT       (-3)
#define CI13162_OTA_PROTO_ERR_CRC          (-4)
#define CI13162_OTA_PROTO_ERR_IO           (-5)
#define CI13162_OTA_PROTO_ERR_OVERFLOW     (-6)

/**
 * @brief Incremental RX parser state (caller provides the byte buffer).
 *
 * Payload of a popped frame is only valid until the next push/pop call.
 */
typedef struct {
    uint8_t *buf;
    size_t capacity;
    size_t length;
    size_t max_payload;
    size_t pending_discard;
} ci13162_ota_rx_parser_t;

/** Decoded frame view. payload points into the parser buffer. */
typedef struct {
    uint8_t msg_type;
    uint8_t cmd;
    uint8_t seq;
    const uint8_t *payload;
    uint16_t payload_len;
} ci13162_ota_frame_t;

/** Scatter/gather payload segment for fragment TX. */
typedef struct {
    const uint8_t *data;
    uint16_t length;
} ci13162_ota_iovec_t;

/**
 * @brief Low-level TX callback used by fragment frame send helpers.
 *
 *  @param[in]  data    Bytes to transmit.
 *  @param[in]  length  Number of bytes.
 *  @param[in]  user    Opaque user pointer from the caller.
 *  @return     0 on success, negative on I/O failure.
 */
typedef int32_t (*ci13162_ota_tx_fn_t)(const void *data, size_t length, void *user);

/** @brief Compute CRC16-CCITT (poly 0x1021, init selectable, no final XOR).
 *
 *  @param[in]  data    Input bytes (may be NULL when len is 0).
 *  @param[in]  length  Number of input bytes.
 *  @param[in]  crc     Initial CRC value (use 0 for a fresh calculation).
 *  @return             Updated CRC16 value.
 */
uint16_t ci13162_ota_crc16_ccitt(const uint8_t *data, size_t length, uint16_t crc);

/** @brief Write a little-endian 16-bit value. */
void ci13162_ota_put_u16_le(uint8_t *dst, uint16_t value);

/** @brief Write a little-endian 32-bit value. */
void ci13162_ota_put_u32_le(uint8_t *dst, uint32_t value);

/** @brief Read a little-endian 16-bit value. */
uint16_t ci13162_ota_get_u16_le(const uint8_t *src);

/** @brief Read a little-endian 32-bit value. */
uint32_t ci13162_ota_get_u32_le(const uint8_t *src);

/** @brief Pack one OTA frame into a caller-provided buffer.
 *
 *  @param[out] out          Destination buffer.
 *  @param[in]  out_capacity Destination capacity in bytes.
 *  @param[in]  msg_type     Frame type (A0/A1/A2/A3).
 *  @param[in]  cmd          Command code.
 *  @param[in]  seq          Sequence number.
 *  @param[in]  payload      Optional payload bytes.
 *  @param[in]  payload_len  Payload length.
 *  @param[out] out_len      Packed frame length on success.
 *  @return     CI13162_OTA_PROTO_OK on success.
 *  @retval     CI13162_OTA_PROTO_ERR_ARG   Invalid argument.
 *  @retval     CI13162_OTA_PROTO_ERR_SPACE Output buffer too small.
 */
int32_t ci13162_ota_frame_pack(uint8_t *out,
                               size_t out_capacity,
                               uint8_t msg_type,
                               uint8_t cmd,
                               uint8_t seq,
                               const uint8_t *payload,
                               uint16_t payload_len,
                               size_t *out_len);

/** @brief Initialize an RX parser over a caller-owned buffer.
 *
 *  @param[out] parser       Parser state.
 *  @param[in]  buffer       Storage for reassembled RX bytes.
 *  @param[in]  capacity     Buffer capacity.
 *  @param[in]  max_payload  Maximum accepted payload length (0 => default).
 *  @return     CI13162_OTA_PROTO_OK on success.
 */
int32_t ci13162_ota_rx_parser_init(ci13162_ota_rx_parser_t *parser,
                                   uint8_t *buffer,
                                   size_t capacity,
                                   size_t max_payload);

/** @brief Reset parser length without clearing the buffer contents. */
void ci13162_ota_rx_parser_reset(ci13162_ota_rx_parser_t *parser);

/** @brief Append raw UART bytes into the parser buffer.
 *
 *  @return     CI13162_OTA_PROTO_OK on success.
 *  @retval     CI13162_OTA_PROTO_ERR_OVERFLOW Buffer full.
 */
int32_t ci13162_ota_rx_parser_push(ci13162_ota_rx_parser_t *parser,
                                   const uint8_t *data,
                                   size_t length);

/** @brief Try to extract one complete valid frame from the parser.
 *
 *  @param[in,out] parser Parser state.
 *  @param[out]    frame  Decoded frame (payload valid until next push/pop).
 *  @return     CI13162_OTA_PROTO_OK when a frame is returned.
 *  @retval     CI13162_OTA_PROTO_NEED_MORE More bytes required.
 *  @retval     CI13162_OTA_PROTO_ERR_ARG   Invalid argument.
 */
int32_t ci13162_ota_rx_parser_pop(ci13162_ota_rx_parser_t *parser,
                                  ci13162_ota_frame_t *frame);

/** @brief Send one frame using a TX callback (payload may be contiguous).
 *
 *  @note Suitable for small control frames. Large write responses should use
 *        ci13162_ota_frame_send_iov() or ci13162_ota_frame_send_write_rsp().
 */
int32_t ci13162_ota_frame_send(ci13162_ota_tx_fn_t tx,
                               void *user,
                               uint8_t msg_type,
                               uint8_t cmd,
                               uint8_t seq,
                               const uint8_t *payload,
                               uint16_t payload_len);

/** @brief Send one frame whose payload is assembled from iovec segments.
 *
 *  @note CRC is computed over type|cmd|seq|payload_parts without requiring a
 *        contiguous payload buffer. Header/CRC/tail are sent as small writes.
 */
int32_t ci13162_ota_frame_send_iov(ci13162_ota_tx_fn_t tx,
                                   void *user,
                                   uint8_t msg_type,
                                   uint8_t cmd,
                                   uint8_t seq,
                                   const ci13162_ota_iovec_t *parts,
                                   size_t part_count);

/** @brief Send A2/08 write response: payload = offset[4 LE] | data[size].
 *
 *  @param[in] tx        TX callback.
 *  @param[in] user      Callback user pointer.
 *  @param[in] seq       Sequence number.
 *  @param[in] offset    Requested flash/image offset.
 *  @param[in] data      Data bytes for this fragment.
 *  @param[in] data_len  Data length (must match the chip request size).
 *  @return     CI13162_OTA_PROTO_OK on success.
 */
int32_t ci13162_ota_frame_send_write_rsp(ci13162_ota_tx_fn_t tx,
                                         void *user,
                                         uint8_t seq,
                                         uint32_t offset,
                                         const uint8_t *data,
                                         uint16_t data_len);

/** @brief Build BLOCK_INFO payload: base[4] | size[4] | crc16[2]. */
int32_t ci13162_ota_build_block_info(uint8_t *out,
                                     size_t out_capacity,
                                     uint32_t base,
                                     uint32_t size,
                                     uint16_t block_crc,
                                     size_t *out_len);

/** @brief Build ERASE payload: base[4] | size[4] | chunk[4]. */
int32_t ci13162_ota_build_erase(uint8_t *out,
                                size_t out_capacity,
                                uint32_t base,
                                uint32_t size,
                                uint32_t chunk,
                                size_t *out_len);

/** @brief Build CHANGE_BAUD payload: baud[4 LE]. */
int32_t ci13162_ota_build_change_baud(uint8_t *out,
                                      size_t out_capacity,
                                      uint32_t baud,
                                      size_t *out_len);

/** @brief Build VERIFY payload: base[4] | size[4] | crc16[2]. */
int32_t ci13162_ota_build_verify(uint8_t *out,
                                 size_t out_capacity,
                                 uint32_t base,
                                 uint32_t size,
                                 uint16_t block_crc,
                                 size_t *out_len);

/** @brief Parse A1/08 request payload into offset/size. */
int32_t ci13162_ota_parse_write_req(const uint8_t *payload,
                                    uint16_t payload_len,
                                    uint32_t *offset,
                                    uint32_t *size);

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
/** @brief Host-callable regression tests for CRC/frame codec. */
int ci13162_ota_protocol_self_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CI13162_OTA_PROTOCOL_H */
