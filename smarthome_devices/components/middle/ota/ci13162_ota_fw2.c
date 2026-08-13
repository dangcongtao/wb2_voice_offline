/**
 * @file ci13162_ota_fw2.c
 * @brief Firmware V2 partition-table validation for CI13162 OTA.
 */
#include "ci13162_ota_fw2.h"

#include <string.h>

static const char *const s_ci13162_ota_fw2_part_names[CI13162_OTA_FW2_PART_COUNT] = {
    "user_code1",
    "user_code2",
    "asr",
    "dnn",
    "voice",
    "user_file"
};

uint32_t ci13162_ota_fw2_align_up(uint32_t value, uint32_t alignment)
{
    uint32_t mask;

    if ((alignment == 0U) || ((alignment & (alignment - 1U)) != 0U)) {
        return 0U;
    }

    mask = alignment - 1U;
    if ((value & mask) == 0U) {
        return value;
    }
    if (value > (0xFFFFFFFFU - mask)) {
        return 0U;
    }
    return (value + mask) & ~mask;
}

static uint32_t ci13162_ota_fw2_read_u32(const uint8_t *p)
{
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint16_t ci13162_ota_fw2_read_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static void ci13162_ota_fw2_copy_cstr(char *dst, size_t dst_size, const uint8_t *src, size_t src_len)
{
    size_t n;
    size_t i;

    if ((dst == NULL) || (dst_size == 0U)) {
        return;
    }

    n = src_len;
    if (n >= dst_size) {
        n = dst_size - 1U;
    }
    for (i = 0U; i < n; i++) {
        if (src[i] == 0U) {
            n = i;
            break;
        }
        dst[i] = (char)src[i];
    }
    dst[n] = '\0';
}

static int ci13162_ota_fw2_chip_ok(const char *chip_name)
{
    if (chip_name == NULL) {
        return 0;
    }
    /* Reference tool accepts CI13LC*; keep CI13162 as a compatible prefix. */
    if ((chip_name[0] == 'C') &&
        (chip_name[1] == 'I') &&
        (chip_name[2] == '1') &&
        (chip_name[3] == '3') &&
        (chip_name[4] == 'L') &&
        (chip_name[5] == 'C')) {
        return 1;
    }
    if ((chip_name[0] == 'C') &&
        (chip_name[1] == 'I') &&
        (chip_name[2] == '1') &&
        (chip_name[3] == '3') &&
        (chip_name[4] == '1') &&
        (chip_name[5] == '6') &&
        (chip_name[6] == '2')) {
        return 1;
    }
    return 0;
}

int32_t ci13162_ota_fw2_validate(const uint8_t *image,
                                 size_t available_length,
                                 uint32_t content_length,
                                 uint32_t firmware_max_size,
                                 ci13162_ota_fw2_layout_t *layout)
{
    const uint8_t *base;
    size_t off;
    size_t i;
    uint32_t aligned;

    if ((image == NULL) || (layout == NULL)) {
        return CI13162_OTA_FW2_ERR_ARG;
    }
    if (content_length == 0U) {
        return CI13162_OTA_FW2_ERR_SIZE;
    }
    if ((firmware_max_size > 0U) && (content_length > firmware_max_size)) {
        return CI13162_OTA_FW2_ERR_SIZE;
    }
    if (available_length < CI13162_OTA_FW2_MIN_IMAGE_SIZE) {
        return CI13162_OTA_FW2_ERR_TOO_SMALL;
    }
    if (content_length < CI13162_OTA_FW2_MIN_IMAGE_SIZE) {
        return CI13162_OTA_FW2_ERR_TOO_SMALL;
    }

    memset(layout, 0, sizeof(*layout));
    base = image + CI13162_OTA_FW2_TABLE_ADDR;
    off = 0U;

    layout->manufacturer_id = ci13162_ota_fw2_read_u32(base + off);
    off += 4U;
    layout->product_id0 = ci13162_ota_fw2_read_u32(base + off);
    off += 4U;
    layout->product_id1 = ci13162_ota_fw2_read_u32(base + off);
    off += 4U;

    ci13162_ota_fw2_copy_cstr(layout->hardware_name,
                              sizeof(layout->hardware_name),
                              base + off,
                              64U);
    off += 64U;
    off += 4U; /* hardware version */

    ci13162_ota_fw2_copy_cstr(layout->software_name,
                              sizeof(layout->software_name),
                              base + off,
                              64U);
    off += 64U;
    off += 4U; /* software version */
    off += 4U; /* bootloader version */

    ci13162_ota_fw2_copy_cstr(layout->chip_name,
                              sizeof(layout->chip_name),
                              base + off,
                              9U);
    off += 9U;

    layout->firmware_format = base[off];
    off += 1U;
    off += 4U; /* reserved */

    for (i = 0U; i < CI13162_OTA_FW2_PART_COUNT; i++) {
        memset(layout->parts[i].name, 0, sizeof(layout->parts[i].name));
        strncpy(layout->parts[i].name,
                s_ci13162_ota_fw2_part_names[i],
                sizeof(layout->parts[i].name) - 1U);
        layout->parts[i].version = ci13162_ota_fw2_read_u32(base + off);
        layout->parts[i].offset = ci13162_ota_fw2_read_u32(base + off + 4U);
        layout->parts[i].size = ci13162_ota_fw2_read_u32(base + off + 8U);
        layout->parts[i].crc = ci13162_ota_fw2_read_u32(base + off + 12U);
        layout->parts[i].status = base[off + 16U];
        off += 17U;
    }

    layout->nv_offset = ci13162_ota_fw2_read_u32(base + off);
    layout->nv_size = ci13162_ota_fw2_read_u32(base + off + 4U);
    off += 8U;
    layout->table_checksum = ci13162_ota_fw2_read_u16(base + off);

    if (layout->firmware_format != CI13162_OTA_FW2_FORMAT) {
        return CI13162_OTA_FW2_ERR_FORMAT;
    }
    if (ci13162_ota_fw2_chip_ok(layout->chip_name) == 0) {
        return CI13162_OTA_FW2_ERR_CHIP;
    }
    if ((layout->nv_offset == 0xFFFFFFFFU) || (layout->nv_size == 0xFFFFFFFFU) ||
        (layout->nv_size == 0U)) {
        return CI13162_OTA_FW2_ERR_NV;
    }
    if (((layout->nv_offset % CI13162_OTA_FW2_CHUNK_SIZE) != 0U) ||
        ((layout->nv_size % CI13162_OTA_FW2_CHUNK_SIZE) != 0U)) {
        return CI13162_OTA_FW2_ERR_ALIGN;
    }

    aligned = ci13162_ota_fw2_align_up(content_length, CI13162_OTA_FW2_CHUNK_SIZE);
    if (aligned == 0U) {
        return CI13162_OTA_FW2_ERR_SIZE;
    }
    if (aligned > layout->nv_offset) {
        return CI13162_OTA_FW2_ERR_OVERLAP;
    }

    layout->content_length = content_length;
    layout->aligned_fw_size = aligned;
    return CI13162_OTA_FW2_OK;
}

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
static void ci13162_ota_fw2_write_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 0xFFU);
    p[1] = (uint8_t)((value >> 8) & 0xFFU);
    p[2] = (uint8_t)((value >> 16) & 0xFFU);
    p[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static void ci13162_ota_fw2_write_u16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xFFU);
    p[1] = (uint8_t)((value >> 8) & 0xFFU);
}

int ci13162_ota_fw2_self_test(void)
{
    uint8_t image[CI13162_OTA_FW2_MIN_IMAGE_SIZE];
    ci13162_ota_fw2_layout_t layout;
    uint8_t *base;
    size_t off;
    size_t i;
    int failed = 0;
    uint32_t content_length = CI13162_OTA_FW2_MIN_IMAGE_SIZE;

    memset(image, 0xFF, sizeof(image));
    base = image + CI13162_OTA_FW2_TABLE_ADDR;
    off = 0U;

    ci13162_ota_fw2_write_u32(base + off, 0x11111111U);
    off += 4U;
    ci13162_ota_fw2_write_u32(base + off, 0x22222222U);
    off += 4U;
    ci13162_ota_fw2_write_u32(base + off, 0x33333333U);
    off += 4U;
    memcpy(base + off, "hw", 2U);
    off += 64U;
    off += 4U;
    memcpy(base + off, "sw", 2U);
    off += 64U;
    off += 4U;
    off += 4U;
    memcpy(base + off, "CI13LC", 6U);
    off += 9U;
    base[off] = 2U;
    off += 1U;
    off += 4U;

    for (i = 0U; i < CI13162_OTA_FW2_PART_COUNT; i++) {
        ci13162_ota_fw2_write_u32(base + off, 1U);
        ci13162_ota_fw2_write_u32(base + off + 4U, (uint32_t)(i * 0x1000U));
        ci13162_ota_fw2_write_u32(base + off + 8U, 0x1000U);
        ci13162_ota_fw2_write_u32(base + off + 12U, 0U);
        base[off + 16U] = 0U;
        off += 17U;
    }

    ci13162_ota_fw2_write_u32(base + off, 0x100000U); /* nv_offset */
    ci13162_ota_fw2_write_u32(base + off + 4U, 0x1000U); /* nv_size */
    off += 8U;
    ci13162_ota_fw2_write_u16(base + off, 0xABCDU);

    if (ci13162_ota_fw2_validate(image,
                                 sizeof(image),
                                 content_length,
                                 3U * 1024U * 1024U,
                                 &layout) != CI13162_OTA_FW2_OK) {
        failed++;
    } else if (layout.nv_offset != 0x100000U) {
        failed++;
    } else if (layout.nv_size != 0x1000U) {
        failed++;
    } else if (layout.aligned_fw_size != 0x3000U) {
        failed++;
    }

    /* Bad format. */
    base[4U + 4U + 4U + 64U + 4U + 64U + 4U + 4U + 9U] = 1U;
    if (ci13162_ota_fw2_validate(image,
                                 sizeof(image),
                                 content_length,
                                 3U * 1024U * 1024U,
                                 &layout) != CI13162_OTA_FW2_ERR_FORMAT) {
        failed++;
    }

    if (ci13162_ota_fw2_align_up(0x1001U, 0x1000U) != 0x2000U) {
        failed++;
    }

    return failed;
}
#endif
