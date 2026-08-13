/**
 * @file ci13162_ota_http.h
 * @brief Bounded sequential raw-body HTTP/HTTPS reader for CI13162 OTA.
 */
#ifndef CI13162_OTA_HTTP_H
#define CI13162_OTA_HTTP_H

#include <stddef.h>
#include <stdint.h>

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CI13162_OTA_HTTP_HOST_MAX          128U
#define CI13162_OTA_HTTP_PATH_MAX          384U
#define CI13162_OTA_HTTP_PORT_STR_MAX      8U
#define CI13162_OTA_HTTP_DEFAULT_TIMEOUT_MS 15000U

#define CI13162_OTA_HTTP_OK                0
#define CI13162_OTA_HTTP_ERR_ARG           (-1)
#define CI13162_OTA_HTTP_ERR_URL           (-2)
#define CI13162_OTA_HTTP_ERR_DNS           (-3)
#define CI13162_OTA_HTTP_ERR_CONNECT       (-4)
#define CI13162_OTA_HTTP_ERR_TLS           (-5)
#define CI13162_OTA_HTTP_ERR_IO            (-6)
#define CI13162_OTA_HTTP_ERR_STATUS        (-7)
#define CI13162_OTA_HTTP_ERR_HEADER        (-8)
#define CI13162_OTA_HTTP_ERR_LENGTH        (-9)
#define CI13162_OTA_HTTP_ERR_TIMEOUT       (-10)
#define CI13162_OTA_HTTP_ERR_EOF           (-11)
#define CI13162_OTA_HTTP_ERR_SPACE         (-12)
#define CI13162_OTA_HTTP_ERR_STATE         (-13)

/** Parsed URL fields used by the sequential reader. */
typedef struct {
    char host[CI13162_OTA_HTTP_HOST_MAX];
    char path[CI13162_OTA_HTTP_PATH_MAX]; /* path + optional ?query */
    char port_str[CI13162_OTA_HTTP_PORT_STR_MAX];
    uint16_t port;
    uint8_t is_https;
} ci13162_ota_http_url_t;

/**
 * @brief HTTP/HTTPS reader context.
 *
 * Caller allocates this object (stack/static/heap). io_buf is a separate
 * caller-owned scratch buffer used for request/response header assembly.
 */
typedef struct {
    ci13162_ota_http_url_t url;
    uint8_t *io_buf;
    size_t io_buf_size;
    size_t pending_off;
    size_t pending_len;
    uint32_t timeout_ms;
    uint32_t content_length;
    uint32_t body_read;
    int sock_fd;
    uint8_t is_https;
    uint8_t opened;
    uint8_t header_done;
    uint8_t net_initialized;
    uint8_t tls_ready;
    mbedtls_net_context server_fd;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
} ci13162_ota_http_ctx_t;

/** @brief Parse an absolute http(s) URL into host/port/path(+query).
 *
 *  @param[in]  url  NUL-terminated absolute URL.
 *  @param[out] out  Parsed fields.
 *  @return     CI13162_OTA_HTTP_OK on success.
 *  @retval     CI13162_OTA_HTTP_ERR_URL Unsupported or malformed URL.
 *  @note       Path always begins with '/'. Query string is preserved.
 *              Fragment identifiers are rejected. Userinfo is rejected.
 */
int32_t ci13162_ota_http_parse_url(const char *url, ci13162_ota_http_url_t *out);

/** @brief Open a sequential raw-body download for the given URL.
 *
 *  Requirements enforced on the response:
 *  - HTTP status 200
 *  - Content-Length present and non-zero
 *  - no Transfer-Encoding: chunked
 *  - no redirect status codes
 *
 *  @param[in,out] ctx          Caller-allocated context (zeroed by this call).
 *  @param[in]     url          Absolute http or https URL.
 *  @param[in]     io_buf       Scratch buffer for headers and partial body.
 *  @param[in]     io_buf_size  Scratch capacity (recommend >= 1024).
 *  @param[in]     timeout_ms   Socket/TLS I/O timeout (0 => default).
 *  @return     CI13162_OTA_HTTP_OK on success.
 */
int32_t ci13162_ota_http_open(ci13162_ota_http_ctx_t *ctx,
                              const char *url,
                              uint8_t *io_buf,
                              size_t io_buf_size,
                              uint32_t timeout_ms);

/** @brief Return Content-Length advertised by the server. */
uint32_t ci13162_ota_http_content_length(const ci13162_ota_http_ctx_t *ctx);

/** @brief Return how many body bytes have already been delivered. */
uint32_t ci13162_ota_http_body_read(const ci13162_ota_http_ctx_t *ctx);

/** @brief Return non-zero when body_read == content_length. */
int ci13162_ota_http_is_complete(const ci13162_ota_http_ctx_t *ctx);

/** @brief Read the next sequential body bytes into out.
 *
 *  @param[in,out] ctx      Open reader context.
 *  @param[out]    out      Destination buffer.
 *  @param[in]     out_len  Destination capacity.
 *  @return     >0 bytes copied, 0 when the body is already complete,
 *              negative error code on failure (including premature EOF).
 */
int32_t ci13162_ota_http_read(ci13162_ota_http_ctx_t *ctx,
                              uint8_t *out,
                              size_t out_len);

/** @brief Read exactly need body bytes or fail.
 *
 *  @return     CI13162_OTA_HTTP_OK on success.
 *  @retval     CI13162_OTA_HTTP_ERR_EOF Premature end of body.
 */
int32_t ci13162_ota_http_read_exact(ci13162_ota_http_ctx_t *ctx,
                                    uint8_t *out,
                                    size_t need);

/** @brief Close sockets/TLS and mark the context unused. Safe to call twice. */
void ci13162_ota_http_close(ci13162_ota_http_ctx_t *ctx);

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
/** @brief Offline tests for URL/header parsing helpers. */
int ci13162_ota_http_self_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CI13162_OTA_HTTP_H */
