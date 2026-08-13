/**
 * @file ci13162_ota_fw2.h
 * @brief Firmware V2 partition-table validation helpers for CI13162 OTA.
 */
#ifndef CI13162_OTA_FW2_H
#define CI13162_OTA_FW2_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CI13162_OTA_FW2_TABLE_ADDR         0x2000U
#define CI13162_OTA_FW2_TABLE_SIZE         278U
#define CI13162_OTA_FW2_MIN_IMAGE_SIZE     (CI13162_OTA_FW2_TABLE_ADDR + CI13162_OTA_FW2_TABLE_SIZE)
#define CI13162_OTA_FW2_CHUNK_SIZE         0x1000U
#define CI13162_OTA_FW2_PART_COUNT         6U
#define CI13162_OTA_FW2_FORMAT             2U

#define CI13162_OTA_FW2_OK                 0
#define CI13162_OTA_FW2_ERR_ARG            (-1)
#define CI13162_OTA_FW2_ERR_TOO_SMALL      (-2)
#define CI13162_OTA_FW2_ERR_FORMAT         (-3)
#define CI13162_OTA_FW2_ERR_CHIP           (-4)
#define CI13162_OTA_FW2_ERR_NV             (-5)
#define CI13162_OTA_FW2_ERR_ALIGN          (-6)
#define CI13162_OTA_FW2_ERR_OVERLAP        (-7)
#define CI13162_OTA_FW2_ERR_SIZE           (-8)

/** One partition entry from the FW2 table. */
typedef struct {
    char name[16];
    uint32_t version;
    uint32_t offset;
    uint32_t size;
    uint32_t crc;
    uint8_t status;
} ci13162_ota_fw2_part_t;

/**
 * @brief Parsed and validated FW2 layout fields needed by the OTA task.
 */
typedef struct {
    uint32_t manufacturer_id;
    uint32_t product_id0;
    uint32_t product_id1;
    char hardware_name[65];
    char software_name[65];
    char chip_name[10];
    uint8_t firmware_format;
    uint32_t nv_offset;
    uint32_t nv_size;
    uint16_t table_checksum;
    ci13162_ota_fw2_part_t parts[CI13162_OTA_FW2_PART_COUNT];
    uint32_t content_length;   /* Raw image size (HTTP Content-Length). */
    uint32_t aligned_fw_size;  /* content_length rounded up to 4 KiB. */
} ci13162_ota_fw2_layout_t;

/** @brief Align value up to a power-of-two boundary.
 *
 *  @param[in] value      Input value.
 *  @param[in] alignment  Alignment (must be power of two and non-zero).
 *  @return               Aligned value, or 0 on invalid alignment/overflow.
 */
uint32_t ci13162_ota_fw2_align_up(uint32_t value, uint32_t alignment);

/** @brief Validate a full FW2 partition table at absolute offset 0x2000.
 *
 *  The buffer must contain firmware image bytes starting at offset 0 and must
 *  cover at least [0x2000, 0x2000+278). content_length is the full image size
 *  (typically HTTP Content-Length) and is used to derive the 4 KiB-aligned
 *  firmware write size and to ensure it does not overlap NV.
 *
 *  @param[in]  image              Image bytes from offset 0.
 *  @param[in]  available_length   Valid byte count in image.
 *  @param[in]  content_length     Full firmware size in bytes.
 *  @param[in]  firmware_max_size  Upper bound accepted by product config.
 *  @param[out] layout             Parsed layout on success.
 *  @return     CI13162_OTA_FW2_OK on success.
 *  @retval     CI13162_OTA_FW2_ERR_TOO_SMALL Image/buffer too small.
 *  @retval     CI13162_OTA_FW2_ERR_FORMAT    firmware_format != 2.
 *  @retval     CI13162_OTA_FW2_ERR_CHIP      Unexpected chip name.
 *  @retval     CI13162_OTA_FW2_ERR_NV        Invalid NV region.
 *  @retval     CI13162_OTA_FW2_ERR_ALIGN     NV not 4 KiB aligned.
 *  @retval     CI13162_OTA_FW2_ERR_OVERLAP   Aligned image overlaps NV.
 *  @retval     CI13162_OTA_FW2_ERR_SIZE      content_length exceeds max.
 */
int32_t ci13162_ota_fw2_validate(const uint8_t *image,
                                 size_t available_length,
                                 uint32_t content_length,
                                 uint32_t firmware_max_size,
                                 ci13162_ota_fw2_layout_t *layout);

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
/** @brief Host-callable regression tests for FW2 validation. */
int ci13162_ota_fw2_self_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CI13162_OTA_FW2_H */
