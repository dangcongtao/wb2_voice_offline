#include "m_ble.h"
#include "my_ble.h"
#include "event_driven.h"
#include "security.h"

#include "hci_core.h"
#include "hardware.h"
#include <bl_sec.h>
#include <string.h>

#define HUN_BLE_SERVICE_UUID      0xFBFA
#define VOICEBOX_SEND_STATE_CMD   0x9A
#define VOICEBOX_BEACON_ADV_MS    3000U

static uint8_t s_hun_beacon_tid = 0;
static uint8_t s_is_beacon_advertising = 0;
static uint32_t s_last_time_beacon_ble = 0;

void m_ble_send_voicebox_state(uint8_t cmd_id)
{
    if (!ble_is_inited())
    {
        os_trace("m_ble_send_voicebox_state %u, ble not inited\r\n", cmd_id);
        return;
    }

    if (dev_is_doing_config())
    {
        os_trace("m_ble_send_voicebox_state %u, doing config\r\n", cmd_id);
        return;
    }

    if (s_hun_beacon_tid == 0)
    {
        s_hun_beacon_tid = (uint8_t)(((uint32_t)bl_rand() ^ sys_get_tick_ms()) % 255U) + 1U;
    }
    else
    {
        s_hun_beacon_tid++;
        if (s_hun_beacon_tid == 0)
        {
            s_hun_beacon_tid = 1;
        }
    }

    os_trace("m_ble_send_voicebox_state cmd_id: %u, tid: %u, time: %lu\r\n",
             cmd_id, s_hun_beacon_tid, sys_get_tick_ms());

    hunonic_voicebox_msg_t hnn_msg;
    memset(&hnn_msg, 0, sizeof(hnn_msg));
    hnn_msg.service_uuid[0] = 0xFA;
    hnn_msg.service_uuid[1] = 0xFB;
    hnn_msg.cmd = VOICEBOX_SEND_STATE_CMD;
    hnn_msg.tid = s_hun_beacon_tid;
    hnn_msg.content.cmd_id = cmd_id;

    m_ble_get_mac_addr(hnn_msg.content.src_mac);

    hnn_msg.checksum = calculate_checksum((uint8_t *)&hnn_msg.content, sizeof(encode_data_t));
    encrypt_data((uint8_t *)&hnn_msg.content, sizeof(encode_data_t), hnn_msg.tid);

    if (my_ble_update_data((uint8_t *)&hnn_msg, sizeof(hnn_msg)))
    {
        s_last_time_beacon_ble = sys_get_tick_ms();
        s_is_beacon_advertising = 1;
    }
}

void m_ble_beacon_loop(void)
{
    if (!ble_is_inited())
    {
        s_is_beacon_advertising = 0;
        s_last_time_beacon_ble = 0;
        return;
    }

    if (s_is_beacon_advertising &&
        (uint32_t)(sys_get_tick_ms() - s_last_time_beacon_ble) >= VOICEBOX_BEACON_ADV_MS)
    {
        s_is_beacon_advertising = 0;
        s_last_time_beacon_ble = 0;
        if (!dev_is_doing_config())
        {
            bt_le_adv_stop();
            if (!bleuart_connect_status)
            {
                my_ble_start_advertise();
            }
            os_trace("m_ble_beacon_loop: beacon timeout, restore default adv\r\n");
        }
    }
}
