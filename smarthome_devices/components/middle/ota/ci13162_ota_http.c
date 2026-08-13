/**
 * @file ci13162_ota_http.c
 * @brief Bounded sequential raw-body HTTP/HTTPS reader for CI13162 OTA.
 */
#include "ci13162_ota_http.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <lwip/netdb.h>
#include <lwip/sockets.h>

#include "app_config.h"
#include "bl_wdt.h"
#include "http_parser.h"
#include "mbedtls/error.h"

#ifndef CI13162_OTA_HTTP_REQ_MAX
#define CI13162_OTA_HTTP_REQ_MAX 640U
#endif

#ifndef CI13162_OTA_HTTP_READ_MAX
#define CI13162_OTA_HTTP_READ_MAX 1024U
#endif

static void ci13162_ota_http_feed_wdt(void)
{
    bl_wdt_feed();
}

static int ci13162_ota_http_ci_equal(char a, char b)
{
    if ((a >= 'A') && (a <= 'Z')) {
        a = (char)(a - 'A' + 'a');
    }
    if ((b >= 'A') && (b <= 'Z')) {
        b = (char)(b - 'A' + 'a');
    }
    return (a == b) ? 1 : 0;
}

static int ci13162_ota_http_istarts(const char *s, const char *prefix)
{
    size_t i;

    if ((s == NULL) || (prefix == NULL)) {
        return 0;
    }
    for (i = 0U; prefix[i] != '\0'; i++) {
        if (s[i] == '\0') {
            return 0;
        }
        if (ci13162_ota_http_ci_equal(s[i], prefix[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

static int32_t ci13162_ota_http_copy_field(char *dst,
                                           size_t dst_size,
                                           const char *src,
                                           size_t src_len)
{
    if ((dst == NULL) || (dst_size == 0U) || (src == NULL)) {
        return CI13162_OTA_HTTP_ERR_ARG;
    }
    if (src_len >= dst_size) {
        return CI13162_OTA_HTTP_ERR_SPACE;
    }
    memcpy(dst, src, src_len);
    dst[src_len] = '\0';
    return CI13162_OTA_HTTP_OK;
}

int32_t ci13162_ota_http_parse_url(const char *url, ci13162_ota_http_url_t *out)
{
    struct http_parser_url purl;
    char schema[8];
    size_t url_len;
    size_t path_len;
    size_t query_len;
    size_t total_path;
    const char *path_ptr;
    const char *query_ptr;
    int parser_status;
    uint16_t port;

    if ((url == NULL) || (out == NULL)) {
        return CI13162_OTA_HTTP_ERR_ARG;
    }

    memset(out, 0, sizeof(*out));
    url_len = strlen(url);
    if (url_len == 0U) {
        return CI13162_OTA_HTTP_ERR_URL;
    }

    http_parser_url_init(&purl);
    parser_status = http_parser_parse_url(url, url_len, 0, &purl);
    if (parser_status != 0) {
        return CI13162_OTA_HTTP_ERR_URL;
    }

    if ((purl.field_set & (1U << UF_SCHEMA)) == 0U) {
        return CI13162_OTA_HTTP_ERR_URL;
    }
    if ((purl.field_set & (1U << UF_HOST)) == 0U) {
        return CI13162_OTA_HTTP_ERR_URL;
    }
    if ((purl.field_set & (1U << UF_USERINFO)) != 0U) {
        return CI13162_OTA_HTTP_ERR_URL;
    }
    if ((purl.field_set & (1U << UF_FRAGMENT)) != 0U) {
        return CI13162_OTA_HTTP_ERR_URL;
    }

    if (purl.field_data[UF_SCHEMA].len >= sizeof(schema)) {
        return CI13162_OTA_HTTP_ERR_URL;
    }
    memcpy(schema, url + purl.field_data[UF_SCHEMA].off, purl.field_data[UF_SCHEMA].len);
    schema[purl.field_data[UF_SCHEMA].len] = '\0';

    if ((purl.field_data[UF_SCHEMA].len == 5U) &&
        (ci13162_ota_http_istarts(schema, "https") != 0)) {
        out->is_https = 1U;
        port = 443U;
    } else if ((purl.field_data[UF_SCHEMA].len == 4U) &&
               (ci13162_ota_http_istarts(schema, "http") != 0)) {
        out->is_https = 0U;
        port = 80U;
    } else {
        return CI13162_OTA_HTTP_ERR_URL;
    }

    if (ci13162_ota_http_copy_field(out->host,
                                    sizeof(out->host),
                                    url + purl.field_data[UF_HOST].off,
                                    purl.field_data[UF_HOST].len) != CI13162_OTA_HTTP_OK) {
        return CI13162_OTA_HTTP_ERR_URL;
    }

    if ((purl.field_set & (1U << UF_PORT)) != 0U) {
        port = purl.port;
    }
    out->port = port;
    snprintf(out->port_str, sizeof(out->port_str), "%u", (unsigned int)port);

    path_ptr = "/";
    path_len = 1U;
    if ((purl.field_set & (1U << UF_PATH)) != 0U) {
        path_ptr = url + purl.field_data[UF_PATH].off;
        path_len = purl.field_data[UF_PATH].len;
        if (path_len == 0U) {
            path_ptr = "/";
            path_len = 1U;
        }
    }

    query_ptr = NULL;
    query_len = 0U;
    if ((purl.field_set & (1U << UF_QUERY)) != 0U) {
        query_ptr = url + purl.field_data[UF_QUERY].off;
        query_len = purl.field_data[UF_QUERY].len;
    }

    total_path = path_len;
    if (query_len > 0U) {
        total_path += 1U + query_len; /* '?' + query */
    }
    if (total_path >= sizeof(out->path)) {
        return CI13162_OTA_HTTP_ERR_URL;
    }

    memcpy(out->path, path_ptr, path_len);
    if (query_len > 0U) {
        out->path[path_len] = '?';
        memcpy(out->path + path_len + 1U, query_ptr, query_len);
        out->path[path_len + 1U + query_len] = '\0';
    } else {
        out->path[path_len] = '\0';
    }

    return CI13162_OTA_HTTP_OK;
}

static int32_t ci13162_ota_http_set_sock_timeout(int fd, uint32_t timeout_ms)
{
    struct timeval tv;
    int ret;

    if (fd < 0) {
        return CI13162_OTA_HTTP_ERR_ARG;
    }

    tv.tv_sec = (long)(timeout_ms / 1000U);
    tv.tv_usec = (long)((timeout_ms % 1000U) * 1000U);

    ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (ret != 0) {
        return CI13162_OTA_HTTP_ERR_IO;
    }
    ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if (ret != 0) {
        return CI13162_OTA_HTTP_ERR_IO;
    }
    return CI13162_OTA_HTTP_OK;
}

// Wait until the TCP socket is readable or the remaining timeout expires.
static int32_t ci13162_ota_http_wait_readable(int fd, uint32_t timeout_ms)
{
    fd_set read_fds;
    struct timeval tv;
    int ret;

    if (fd < 0) {
        return CI13162_OTA_HTTP_ERR_ARG;
    }
    if (timeout_ms == 0U) {
        return CI13162_OTA_HTTP_ERR_TIMEOUT;
    }

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    tv.tv_sec = (long)(timeout_ms / 1000U);
    tv.tv_usec = (long)((timeout_ms % 1000U) * 1000U);

    ci13162_ota_http_feed_wdt();
    ret = select(fd + 1, &read_fds, NULL, NULL, &tv);
    if (ret == 0) {
        return CI13162_OTA_HTTP_ERR_TIMEOUT;
    }
    if (ret < 0) {
        return CI13162_OTA_HTTP_ERR_IO;
    }
    return CI13162_OTA_HTTP_OK;
}

static int32_t ci13162_ota_http_connect_tcp(ci13162_ota_http_ctx_t *ctx)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *rp;
    int fd = -1;
    int err;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    ci13162_ota_http_feed_wdt();
    err = getaddrinfo(ctx->url.host, ctx->url.port_str, &hints, &res);
    if ((err != 0) || (res == NULL)) {
        return CI13162_OTA_HTTP_ERR_DNS;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        ci13162_ota_http_feed_wdt();
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (ci13162_ota_http_set_sock_timeout(fd, ctx->timeout_ms) != CI13162_OTA_HTTP_OK) {
            close(fd);
            fd = -1;
            continue;
        }
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0) {
        return CI13162_OTA_HTTP_ERR_CONNECT;
    }

    ctx->sock_fd = fd;
    return CI13162_OTA_HTTP_OK;
}

static int32_t ci13162_ota_http_tls_setup(ci13162_ota_http_ctx_t *ctx)
{
    int ret;

    mbedtls_net_init(&ctx->server_fd);
    ctx->net_initialized = 1U;
    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
    mbedtls_entropy_init(&ctx->entropy);
    ctx->tls_ready = 1U;

    ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy, NULL, 0);
    if (ret != 0) {
        return CI13162_OTA_HTTP_ERR_TLS;
    }

    ret = mbedtls_ssl_config_defaults(&ctx->conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        return CI13162_OTA_HTTP_ERR_TLS;
    }

    /* Match existing product OTA path: no server cert verification. */
    mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

    ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
    if (ret != 0) {
        return CI13162_OTA_HTTP_ERR_TLS;
    }

    /* Reuse the already-connected TCP socket. */
    ctx->server_fd.fd = ctx->sock_fd;
    mbedtls_ssl_set_bio(&ctx->ssl, &ctx->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    {
        TickType_t start = xTaskGetTickCount();
        TickType_t timeout_ticks = pdMS_TO_TICKS(ctx->timeout_ms);

        if (timeout_ticks == 0U) {
            timeout_ticks = 1U;
        }
        while ((ret = mbedtls_ssl_handshake(&ctx->ssl)) != 0) {
            ci13162_ota_http_feed_wdt();
            if ((ret != MBEDTLS_ERR_SSL_WANT_READ) &&
                (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
                return CI13162_OTA_HTTP_ERR_TLS;
            }
            if ((xTaskGetTickCount() - start) > timeout_ticks) {
                return CI13162_OTA_HTTP_ERR_TIMEOUT;
            }
            vTaskDelay(pdMS_TO_TICKS(1U));
        }
    }

    return CI13162_OTA_HTTP_OK;
}

static int32_t ci13162_ota_http_raw_write(ci13162_ota_http_ctx_t *ctx,
                                          const uint8_t *data,
                                          size_t length)
{
    size_t sent = 0U;
    int ret;

    while (sent < length) {
        ci13162_ota_http_feed_wdt();
        if (ctx->is_https != 0U) {
            ret = mbedtls_ssl_write(&ctx->ssl, data + sent, length - sent);
            if ((ret == MBEDTLS_ERR_SSL_WANT_READ) || (ret == MBEDTLS_ERR_SSL_WANT_WRITE)) {
                continue;
            }
            if (ret <= 0) {
                return CI13162_OTA_HTTP_ERR_IO;
            }
            sent += (size_t)ret;
        } else {
            ret = (int)write(ctx->sock_fd, data + sent, length - sent);
            if (ret < 0) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    return CI13162_OTA_HTTP_ERR_TIMEOUT;
                }
                return CI13162_OTA_HTTP_ERR_IO;
            }
            if (ret == 0) {
                return CI13162_OTA_HTTP_ERR_IO;
            }
            sent += (size_t)ret;
        }
    }
    return CI13162_OTA_HTTP_OK;
}

static int32_t ci13162_ota_http_raw_read(ci13162_ota_http_ctx_t *ctx,
                                         uint8_t *data,
                                         size_t length)
{
    TickType_t start;
    TickType_t timeout_ticks;
    TickType_t elapsed;
    uint32_t remain_ms;
    size_t want;
    int32_t wait_ret;
    int ret;

    if (length == 0U) {
        return 0;
    }

    start = xTaskGetTickCount();
    timeout_ticks = pdMS_TO_TICKS(ctx->timeout_ms);
    if (timeout_ticks == 0U) {
        timeout_ticks = 1U;
    }
    want = length;
    if (want > CI13162_OTA_HTTP_READ_MAX) {
        want = CI13162_OTA_HTTP_READ_MAX;
    }

    while ((xTaskGetTickCount() - start) <= timeout_ticks) {
        ci13162_ota_http_feed_wdt();
        if (ctx->is_https != 0U) {
            ret = mbedtls_ssl_read(&ctx->ssl, data, want);
            if ((ret == MBEDTLS_ERR_SSL_WANT_READ) ||
                (ret == MBEDTLS_ERR_SSL_WANT_WRITE)) {
                vTaskDelay(pdMS_TO_TICKS(1U));
                continue;
            }
            if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                return 0;
            }
            if (ret < 0) {
                return CI13162_OTA_HTTP_ERR_IO;
            }
            return ret;
        }

        elapsed = xTaskGetTickCount() - start;
        if (elapsed >= timeout_ticks) {
            return CI13162_OTA_HTTP_ERR_TIMEOUT;
        }
        remain_ms = (uint32_t)((timeout_ticks - elapsed) * portTICK_PERIOD_MS);
        wait_ret = ci13162_ota_http_wait_readable(ctx->sock_fd, remain_ms);
        if (wait_ret != CI13162_OTA_HTTP_OK) {
            return wait_ret;
        }

        ret = (int)recv(ctx->sock_fd, data, want, 0);
        if (ret < 0) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                continue;
            }
            os_trace("[CI13162][HTTP] recv errno %d\r\n", errno);
            return CI13162_OTA_HTTP_ERR_IO;
        }
        return ret;
    }

    return CI13162_OTA_HTTP_ERR_TIMEOUT;
}

static const char *ci13162_ota_http_find_header_end(const char *buf, size_t len, size_t *end_off)
{
    size_t i;

    if ((buf == NULL) || (end_off == NULL)) {
        return NULL;
    }
    for (i = 0U; (i + 3U) < len; i++) {
        if ((buf[i] == '\r') && (buf[i + 1U] == '\n') &&
            (buf[i + 2U] == '\r') && (buf[i + 3U] == '\n')) {
            *end_off = i + 4U;
            return buf + i + 4U;
        }
    }
    return NULL;
}

static int32_t ci13162_ota_http_parse_status_line(const char *header, size_t header_len, int *status)
{
    size_t i;
    size_t sp1 = 0U;
    size_t sp2 = 0U;
    char code[4];

    if ((header == NULL) || (status == NULL) || (header_len < 12U)) {
        return CI13162_OTA_HTTP_ERR_HEADER;
    }
    if ((ci13162_ota_http_istarts(header, "HTTP/1.0") == 0) &&
        (ci13162_ota_http_istarts(header, "HTTP/1.1") == 0)) {
        return CI13162_OTA_HTTP_ERR_HEADER;
    }

    for (i = 0U; i < header_len; i++) {
        if (header[i] == ' ') {
            if (sp1 == 0U) {
                sp1 = i;
            } else {
                sp2 = i;
                break;
            }
        }
        if ((header[i] == '\r') || (header[i] == '\n')) {
            break;
        }
    }
    if ((sp1 == 0U) || (sp2 == 0U) || ((sp2 - sp1) != 4U)) {
        return CI13162_OTA_HTTP_ERR_HEADER;
    }

    memcpy(code, header + sp1 + 1U, 3U);
    code[3] = '\0';
    *status = atoi(code);
    return CI13162_OTA_HTTP_OK;
}

static int32_t ci13162_ota_http_parse_headers(const char *header,
                                              size_t header_len,
                                              uint32_t *content_length)
{
    size_t line_start = 0U;
    size_t i;
    int status = 0;
    int have_length = 0;
    int have_chunked = 0;
    uint32_t clen = 0U;

    if ((header == NULL) || (content_length == NULL)) {
        return CI13162_OTA_HTTP_ERR_ARG;
    }

    if (ci13162_ota_http_parse_status_line(header, header_len, &status) != CI13162_OTA_HTTP_OK) {
        return CI13162_OTA_HTTP_ERR_HEADER;
    }
    if ((status >= 300) && (status < 400)) {
        return CI13162_OTA_HTTP_ERR_STATUS;
    }
    if (status != 200) {
        return CI13162_OTA_HTTP_ERR_STATUS;
    }

    /* Skip status line. */
    for (i = 0U; i + 1U < header_len; i++) {
        if ((header[i] == '\r') && (header[i + 1U] == '\n')) {
            line_start = i + 2U;
            break;
        }
    }

    while ((line_start + 1U) < header_len) {
        size_t line_end = line_start;
        size_t name_end;
        size_t value_start;
        size_t value_end;
        char name[32];
        size_t name_len;
        size_t j;

        if ((header[line_start] == '\r') && (header[line_start + 1U] == '\n')) {
            break;
        }

        while ((line_end + 1U) < header_len) {
            if ((header[line_end] == '\r') && (header[line_end + 1U] == '\n')) {
                break;
            }
            line_end++;
        }
        if ((line_end + 1U) >= header_len) {
            return CI13162_OTA_HTTP_ERR_HEADER;
        }

        name_end = line_start;
        while ((name_end < line_end) && (header[name_end] != ':')) {
            name_end++;
        }
        if ((name_end >= line_end) || (name_end == line_start)) {
            line_start = line_end + 2U;
            continue;
        }

        name_len = name_end - line_start;
        if (name_len >= sizeof(name)) {
            name_len = sizeof(name) - 1U;
        }
        for (j = 0U; j < name_len; j++) {
            char c = header[line_start + j];
            if ((c >= 'A') && (c <= 'Z')) {
                c = (char)(c - 'A' + 'a');
            }
            name[j] = c;
        }
        name[name_len] = '\0';

        value_start = name_end + 1U;
        while ((value_start < line_end) &&
               ((header[value_start] == ' ') || (header[value_start] == '\t'))) {
            value_start++;
        }
        value_end = line_end;
        while ((value_end > value_start) &&
               ((header[value_end - 1U] == ' ') || (header[value_end - 1U] == '\t'))) {
            value_end--;
        }

        if (strcmp(name, "content-length") == 0) {
            char len_buf[16];
            size_t vlen = value_end - value_start;
            size_t k;
            if ((vlen == 0U) || (vlen >= sizeof(len_buf))) {
                return CI13162_OTA_HTTP_ERR_LENGTH;
            }
            for (k = 0U; k < vlen; k++) {
                if ((header[value_start + k] < '0') || (header[value_start + k] > '9')) {
                    return CI13162_OTA_HTTP_ERR_LENGTH;
                }
                len_buf[k] = header[value_start + k];
            }
            len_buf[vlen] = '\0';
            clen = (uint32_t)strtoul(len_buf, NULL, 10);
            have_length = 1;
        } else if (strcmp(name, "transfer-encoding") == 0) {
            /* Reject any transfer-encoding, including chunked. */
            if ((value_end > value_start) &&
                (ci13162_ota_http_istarts(header + value_start, "identity") == 0)) {
                have_chunked = 1;
            }
            if (ci13162_ota_http_istarts(header + value_start, "chunked") != 0) {
                have_chunked = 1;
            }
        }

        line_start = line_end + 2U;
    }

    if (have_chunked != 0) {
        return CI13162_OTA_HTTP_ERR_HEADER;
    }
    if (have_length == 0) {
        return CI13162_OTA_HTTP_ERR_LENGTH;
    }
    if (clen == 0U) {
        return CI13162_OTA_HTTP_ERR_LENGTH;
    }

    *content_length = clen;
    return CI13162_OTA_HTTP_OK;
}

static int32_t ci13162_ota_http_send_request(ci13162_ota_http_ctx_t *ctx)
{
    int n;

    if ((ctx->io_buf == NULL) || (ctx->io_buf_size < CI13162_OTA_HTTP_REQ_MAX)) {
        return CI13162_OTA_HTTP_ERR_SPACE;
    }

    n = snprintf((char *)ctx->io_buf,
                 ctx->io_buf_size,
                 "GET %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "User-Agent: hunonic-wb2-ci13162-ota\r\n"
                 "Connection: close\r\n"
                 "Accept: */*\r\n"
                 "\r\n",
                 ctx->url.path,
                 ctx->url.host);
    if ((n <= 0) || ((size_t)n >= ctx->io_buf_size)) {
        return CI13162_OTA_HTTP_ERR_SPACE;
    }

    return ci13162_ota_http_raw_write(ctx, ctx->io_buf, (size_t)n);
}

static int32_t ci13162_ota_http_read_headers(ci13162_ota_http_ctx_t *ctx)
{
    size_t used = 0U;
    size_t header_end = 0U;
    int32_t nread;
    int32_t pret;
    const char *body_ptr;
    uint32_t content_length = 0U;
    TickType_t start;
    TickType_t timeout_ticks;

    if ((ctx->io_buf == NULL) || (ctx->io_buf_size < 128U)) {
        return CI13162_OTA_HTTP_ERR_ARG;
    }

    start = xTaskGetTickCount();
    timeout_ticks = pdMS_TO_TICKS(ctx->timeout_ms);
    if (timeout_ticks == 0U) {
        timeout_ticks = 1U;
    }

    while (1) {
        ci13162_ota_http_feed_wdt();
        if ((xTaskGetTickCount() - start) > timeout_ticks) {
            return CI13162_OTA_HTTP_ERR_TIMEOUT;
        }
        if (used >= ctx->io_buf_size) {
            return CI13162_OTA_HTTP_ERR_SPACE;
        }

        nread = ci13162_ota_http_raw_read(ctx, ctx->io_buf + used, ctx->io_buf_size - used);
        if (nread == CI13162_OTA_HTTP_ERR_TIMEOUT) {
            continue;
        }
        if (nread < 0) {
            return nread;
        }
        if (nread == 0) {
            return CI13162_OTA_HTTP_ERR_EOF;
        }
        used += (size_t)nread;

        body_ptr = ci13162_ota_http_find_header_end((const char *)ctx->io_buf, used, &header_end);
        if (body_ptr != NULL) {
            break;
        }
    }

    pret = ci13162_ota_http_parse_headers((const char *)ctx->io_buf, header_end, &content_length);
    if (pret != CI13162_OTA_HTTP_OK) {
        return pret;
    }

    ctx->content_length = content_length;
    ctx->body_read = 0U;
    ctx->header_done = 1U;

    /* Preserve any body bytes already received after the header. */
    if (header_end < used) {
        ctx->pending_off = header_end;
        ctx->pending_len = used - header_end;
    } else {
        ctx->pending_off = 0U;
        ctx->pending_len = 0U;
    }

    return CI13162_OTA_HTTP_OK;
}

int32_t ci13162_ota_http_open(ci13162_ota_http_ctx_t *ctx,
                              const char *url,
                              uint8_t *io_buf,
                              size_t io_buf_size,
                              uint32_t timeout_ms)
{
    int32_t ret;

    if ((ctx == NULL) || (url == NULL) || (io_buf == NULL) || (io_buf_size < 256U)) {
        return CI13162_OTA_HTTP_ERR_ARG;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->io_buf = io_buf;
    ctx->io_buf_size = io_buf_size;
    ctx->timeout_ms = (timeout_ms == 0U) ? CI13162_OTA_HTTP_DEFAULT_TIMEOUT_MS : timeout_ms;
    ctx->sock_fd = -1;

    ret = ci13162_ota_http_parse_url(url, &ctx->url);
    if (ret != CI13162_OTA_HTTP_OK) {
        return ret;
    }
    ctx->is_https = ctx->url.is_https;

    os_trace("[CI13162][HTTP] Connecting %s:%s %s\r\n",
             ctx->url.host,
             ctx->url.port_str,
             ctx->url.path);
    ret = ci13162_ota_http_connect_tcp(ctx);
    if (ret != CI13162_OTA_HTTP_OK) {
        os_trace("[CI13162][HTTP] Connect failed (%d)\r\n", (int)ret);
        ci13162_ota_http_close(ctx);
        return ret;
    }

    if (ctx->is_https != 0U) {
        ret = ci13162_ota_http_tls_setup(ctx);
        if (ret != CI13162_OTA_HTTP_OK) {
            os_trace("[CI13162][HTTP] TLS failed (%d)\r\n", (int)ret);
            ci13162_ota_http_close(ctx);
            return ret;
        }
    }

    os_trace("[CI13162][HTTP] Sending request\r\n");
    ret = ci13162_ota_http_send_request(ctx);
    if (ret != CI13162_OTA_HTTP_OK) {
        os_trace("[CI13162][HTTP] Send failed (%d)\r\n", (int)ret);
        ci13162_ota_http_close(ctx);
        return ret;
    }

    os_trace("[CI13162][HTTP] Reading headers\r\n");
    ret = ci13162_ota_http_read_headers(ctx);
    if (ret != CI13162_OTA_HTTP_OK) {
        os_trace("[CI13162][HTTP] Headers failed (%d)\r\n", (int)ret);
        ci13162_ota_http_close(ctx);
        return ret;
    }

    os_trace("[CI13162][HTTP] Opened, content-length %u, pending body %u\r\n",
             (unsigned int)ctx->content_length,
             (unsigned int)ctx->pending_len);
    ctx->opened = 1U;
    return CI13162_OTA_HTTP_OK;
}

uint32_t ci13162_ota_http_content_length(const ci13162_ota_http_ctx_t *ctx)
{
    if (ctx == NULL) {
        return 0U;
    }
    return ctx->content_length;
}

uint32_t ci13162_ota_http_body_read(const ci13162_ota_http_ctx_t *ctx)
{
    if (ctx == NULL) {
        return 0U;
    }
    return ctx->body_read;
}

int ci13162_ota_http_is_complete(const ci13162_ota_http_ctx_t *ctx)
{
    if ((ctx == NULL) || (ctx->opened == 0U)) {
        return 0;
    }
    return (ctx->body_read >= ctx->content_length) ? 1 : 0;
}

int32_t ci13162_ota_http_read(ci13162_ota_http_ctx_t *ctx,
                              uint8_t *out,
                              size_t out_len)
{
    uint32_t remaining;
    size_t to_copy;
    int32_t nread;
    size_t produced = 0U;

    if ((ctx == NULL) || (out == NULL) || (out_len == 0U)) {
        return CI13162_OTA_HTTP_ERR_ARG;
    }
    if ((ctx->opened == 0U) || (ctx->header_done == 0U)) {
        return CI13162_OTA_HTTP_ERR_STATE;
    }
    if (ctx->body_read >= ctx->content_length) {
        return 0;
    }

    remaining = ctx->content_length - ctx->body_read;
    if ((uint32_t)out_len > remaining) {
        out_len = (size_t)remaining;
    }

    /* First drain any body bytes captured with the response headers. */
    if (ctx->pending_len > 0U) {
        to_copy = ctx->pending_len;
        if (to_copy > out_len) {
            to_copy = out_len;
        }
        memcpy(out, ctx->io_buf + ctx->pending_off, to_copy);
        ctx->pending_off += to_copy;
        ctx->pending_len -= to_copy;
        ctx->body_read += (uint32_t)to_copy;
        produced += to_copy;
        out += to_copy;
        out_len -= to_copy;
        if (out_len == 0U) {
            return (int32_t)produced;
        }
    }

    while (out_len > 0U) {
        if (ctx->body_read >= ctx->content_length) {
            break;
        }
        remaining = ctx->content_length - ctx->body_read;
        to_copy = out_len;
        if ((uint32_t)to_copy > remaining) {
            to_copy = (size_t)remaining;
        }

        nread = ci13162_ota_http_raw_read(ctx, out, to_copy);
        if (nread == CI13162_OTA_HTTP_ERR_TIMEOUT) {
            if (produced > 0U) {
                return (int32_t)produced;
            }
            return CI13162_OTA_HTTP_ERR_TIMEOUT;
        }
        if (nread < 0) {
            return nread;
        }
        if (nread == 0) {
            /* Peer closed before Content-Length was fully delivered. */
            return CI13162_OTA_HTTP_ERR_EOF;
        }

        ctx->body_read += (uint32_t)nread;
        produced += (size_t)nread;
        out += (size_t)nread;
        out_len -= (size_t)nread;
    }

    return (int32_t)produced;
}

int32_t ci13162_ota_http_read_exact(ci13162_ota_http_ctx_t *ctx,
                                    uint8_t *out,
                                    size_t need)
{
    size_t got = 0U;
    int32_t n;

    if ((ctx == NULL) || ((need > 0U) && (out == NULL))) {
        return CI13162_OTA_HTTP_ERR_ARG;
    }

    while (got < need) {
        n = ci13162_ota_http_read(ctx, out + got, need - got);
        if (n == 0) {
            return CI13162_OTA_HTTP_ERR_EOF;
        }
        if (n < 0) {
            return n;
        }
        got += (size_t)n;
    }
    return CI13162_OTA_HTTP_OK;
}

void ci13162_ota_http_close(ci13162_ota_http_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->tls_ready != 0U) {
        mbedtls_ssl_close_notify(&ctx->ssl);
        mbedtls_ssl_free(&ctx->ssl);
        mbedtls_ssl_config_free(&ctx->conf);
        mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
        mbedtls_entropy_free(&ctx->entropy);
        ctx->tls_ready = 0U;
    }
    if (ctx->net_initialized != 0U) {
        /* Do not close the fd twice: mbedtls_net_free closes server_fd.fd. */
        if (ctx->sock_fd >= 0) {
            ctx->server_fd.fd = ctx->sock_fd;
            ctx->sock_fd = -1;
        }
        mbedtls_net_free(&ctx->server_fd);
        ctx->net_initialized = 0U;
    } else if (ctx->sock_fd >= 0) {
        close(ctx->sock_fd);
        ctx->sock_fd = -1;
    }

    ctx->opened = 0U;
    ctx->header_done = 0U;
    ctx->pending_off = 0U;
    ctx->pending_len = 0U;
}

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
int ci13162_ota_http_self_test(void)
{
    ci13162_ota_http_url_t url;
    int failed = 0;
    uint32_t clen = 0U;
    static const char header_ok[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 12\r\n"
        "Content-Type: application/octet-stream\r\n"
        "\r\n";
    static const char header_chunked[] =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n";
    static const char header_redirect[] =
        "HTTP/1.1 302 Found\r\n"
        "Location: http://example.com/x\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    if (ci13162_ota_http_parse_url("https://cdn.example.com:8443/fw/v2.bin?token=1",
                                   &url) != CI13162_OTA_HTTP_OK) {
        failed++;
    } else if (url.is_https != 1U) {
        failed++;
    } else if (url.port != 8443U) {
        failed++;
    } else if (strcmp(url.host, "cdn.example.com") != 0) {
        failed++;
    } else if (strcmp(url.path, "/fw/v2.bin?token=1") != 0) {
        failed++;
    }

    if (ci13162_ota_http_parse_url("http://example.com", &url) != CI13162_OTA_HTTP_OK) {
        failed++;
    } else if (strcmp(url.path, "/") != 0) {
        failed++;
    }

    if (ci13162_ota_http_parse_url("ftp://example.com/a", &url) != CI13162_OTA_HTTP_ERR_URL) {
        failed++;
    }
    if (ci13162_ota_http_parse_url("http://user:pass@example.com/a", &url) !=
        CI13162_OTA_HTTP_ERR_URL) {
        failed++;
    }
    if (ci13162_ota_http_parse_url("http://example.com/a#frag", &url) !=
        CI13162_OTA_HTTP_ERR_URL) {
        failed++;
    }

    /* Header parser is file-local; exercise it here under BUILD_TEST only. */
    if (ci13162_ota_http_parse_headers(header_ok, sizeof(header_ok) - 1U, &clen) !=
        CI13162_OTA_HTTP_OK) {
        failed++;
    } else if (clen != 12U) {
        failed++;
    }
    if (ci13162_ota_http_parse_headers(header_chunked, sizeof(header_chunked) - 1U, &clen) !=
        CI13162_OTA_HTTP_ERR_HEADER) {
        failed++;
    }
    if (ci13162_ota_http_parse_headers(header_redirect, sizeof(header_redirect) - 1U, &clen) !=
        CI13162_OTA_HTTP_ERR_STATUS) {
        failed++;
    }

    return failed;
}
#endif
