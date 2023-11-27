#include "m_beacon.h"
#include "bluetooth.h"
#include "hardware.h"
#include "hci_core.h"
#include "m_database.h"

// static ble_scan_info_t master_scan_tbl[MASTER_SCAN_MAX];
// static int master_current_scan;
static uint8_t scan_le_addr[6];
m_ble_t s_ble_obj;
m_ble_info_t obj_ble;

u8_t data_adv[30];
uint8_t data_adv_len = 0;

static struct bt_data adv_data[1] =
    {
        BT_DATA(BT_DATA_SVC_DATA16, "1", 1),
};

// void ble_reverse_byte(uint8_t *arr, uint32_t size)
// {
//     uint8_t i, tmp;

//     for (i = 0; i < size / 2; i++)
//     {
//         tmp = arr[i];
//         arr[i] = arr[size - 1 - i];
//         arr[size - 1 - i] = tmp;
//     }
// }

void master_proc_rcv_request_link(uint8_t *data, uint8_t len);
void slave_proc_response_request_link(uint8_t *data, uint8_t len);
void master_proc_rcv_verify_link(uint8_t *data, uint8_t len);

static int is_existed_in_db(uint8_t* mac)
{
    uint8_t i;
    for (i = 0; i < obj_ble.num_devices_link; i++)
    {
        if (!memcmp(obj_ble.list_child[i].mac, mac, 6)) {
            return i;
        }
    }
    return -1;
}

static uint8_t is_tid_valid(int index, uint8_t tid)
{
    if (index == -1) {
        return 0;
    }
    if (s_ble_obj.arr_tid_child[index] != tid)
    {
        s_ble_obj.arr_tid_child[index] = tid;
        return 1;
    }
    return 0;
}

static bool scan_data_cb(struct bt_data *data, void *user_data)
{
    // ble_scan_info_t *info = user_data;
    

    switch (data->type)
    {
    case BT_DATA_SVC_DATA16:
    {
        if (*((uint16_t *)&(data->data[0])) == HUN_BLE_SERVICE_UUID)
        {
            // os_trace("scan_data_cb 16-bit service:\r\n");
            
            switch (data->data[2])
            {
            case WAIT_CONFIG_REC:
                master_proc_rcv_request_link(data->data, data->data_len);
                break;

            case RESPONSE_REQUEST_LINK:
                slave_proc_response_request_link(data->data, data->data_len);
                break;

            case VERIFY_LINK:
                master_proc_rcv_verify_link(data->data, data->data_len);
                break;

            case SEND_SS_STATE_CBCT:{
                if (is_tid_valid(is_existed_in_db((uint8_t*)(&data->data[3])), data->data[9])) {
                    os_trace("bat den\r\n");
                } else {
                    return true;
                }
            } break;
            
            default:
                // os_trace("cmd not support: %u\r\n", data->data[2]);
                // break;
                return true;
            }
            // uint8_t i;
            // for (i = 0; i < data->data_len; i++)
            // {
            //     os_trace("%02X ", data->data[i]);
            // }
            // os_trace("\r\n");
            return false;
        }
    }
        return true;
    default:
        return true;
    }
}

static void scan_device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t evtype,
                              struct net_buf_simple *buf)
{
    bt_data_parse(buf, scan_data_cb, NULL);
}

int ble_master_scan(void)
{

    memset(&s_ble_obj, 0, sizeof(m_ble_t));
    s_ble_obj.tid = rand() & 0xFF;

    int ret;
    struct bt_le_scan_param scan_param;
    scan_param.type = BT_LE_SCAN_TYPE_PASSIVE;
    scan_param.filter_dup = BT_LE_SCAN_FILTER_DUPLICATE;
    scan_param.interval = 160; // 100 ms
    scan_param.window = 48;

    // master_current_scan = 0;
    memset(scan_le_addr, 0, sizeof(scan_le_addr));
    // memset(master_scan_tbl, 0, sizeof(master_scan_tbl));

    ret = bt_le_scan_start(&scan_param, scan_device_found);
    if (ret != 0)
    {
        printf("[BLE][SCAN] ret:%d \r\n", ret);
        return -1;
    }

    return 0;
}



void my_ble_update_data(void)
{
    //
    bt_le_adv_stop();
    struct bt_le_adv_param param;
    param.id = BT_ID_DEFAULT;
    param.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
    param.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;
    param.options = (BT_LE_ADV_OPT_NONE);

    //

    int err = 0;
    memset(&adv_data[0], 0, sizeof(struct bt_data));
    adv_data[0].type = BT_DATA_SVC_DATA16;
    adv_data[0].data = (u8_t *)data_adv;
    adv_data[0].data_len = data_adv_len;
    // err = bt_le_adv_update_data(adv_data, ARRAY_SIZE(adv_data), NULL, 0); // rsp_data, ARRAY_SIZE(rsp_data));
    
    //
    bt_le_adv_start(&param, adv_data, ARRAY_SIZE(adv_data),NULL,0);
    //

    if (err)
    {
        os_trace("Failed to update data adv (err %d)", err);
    }
    else
    {
        os_trace("beacon data success: ");
        for (uint8_t i = 0; i < data_adv_len; i++)
        {
            os_trace("%02X ", data_adv[i]);
        }
        os_trace("\r\n");
    }
}

void my_ble_reset_data(void)
{
    memset(data_adv, 0, sizeof(data_adv));
    data_adv_len = 0;
    my_ble_update_data();
}

void adv_send_request_link(void)
{
    memset(data_adv, 0, sizeof(data_adv));

    data_adv[0] = HUN_BLE_SERVICE_UUID & 0xFF;
    data_adv[1] = HUN_BLE_SERVICE_UUID >> 8;
    data_adv[2] = WAIT_CONFIG_REC;
    bt_addr_le_t adv_addr;
    bt_get_local_public_address(&adv_addr);
    memcpy(&data_adv[3], adv_addr.a.val, 6);
    data_adv[9] = ++s_ble_obj.tid;
    data_adv[10] = DEVICE_NEW;
    data_adv_len = 11;

    my_ble_update_data();
}

void master_accept_request_link(void)
{
    if (s_ble_obj.step != WAIT_CONFIG) {
        os_trace("master_accept_request_link: step incorrect %u\r\n", s_ble_obj.step);
        return;
    }
    memset(data_adv, 0, sizeof(data_adv));

    data_adv[0] = HUN_BLE_SERVICE_UUID & 0xFF;
    data_adv[1] = HUN_BLE_SERVICE_UUID >> 8;
    data_adv[2] = RESPONSE_REQUEST_LINK;
    bt_addr_le_t adv_addr;
    bt_get_local_public_address(&adv_addr);
    memcpy(&data_adv[3], adv_addr.a.val, 6);
    data_adv[9] = ++s_ble_obj.tid;
    memcpy(&data_adv[10], s_ble_obj.mac, 6);
    data_adv_len = 16;

    s_ble_obj.step = WAIT_VERIFY_ACCEPT_LINK;

    my_ble_update_data();

    vTaskDelay(pdMS_TO_TICKS(1000));

    my_ble_reset_data();
}

void slave_response_request_link(void)
{
    memset(data_adv, 0, sizeof(data_adv));

    data_adv[0] = HUN_BLE_SERVICE_UUID & 0xFF;
    data_adv[1] = HUN_BLE_SERVICE_UUID >> 8;
    data_adv[2] = VERIFY_LINK;
    bt_addr_le_t adv_addr;
    bt_get_local_public_address(&adv_addr);
    memcpy(&data_adv[3], adv_addr.a.val, 6);
    data_adv[9] = ++s_ble_obj.tid;
    memcpy(&data_adv[10], s_ble_obj.mac, 6);
    data_adv_len = 16;

    //
    // memcpy(s_ble_obj.mac, m_param->mac_addr, 6);
        os_trace("VERIFY_LINK: Verify success\r\n");
        // slave_response_request_link();
        // ble_adv_data_t m_adv_data1;
        // m_adv_data1.cmd = VERIFY_LINK;
        // memcpy(m_adv_data1.data.mac, s_ble_obj.mac, sizeof(m_adv_data1.data.mac));
        // m_adv_data1.len = 17;
    //

    my_ble_update_data();

    vTaskDelay(pdMS_TO_TICKS(1000));

    my_ble_reset_data();
}

void master_proc_rcv_request_link(uint8_t *data, uint8_t len)
{
    if (s_ble_obj.step > WAIT_CONFIG) {
        return;
    }
    if (s_ble_obj.is_run_config == 1)
    {
        if (!memcmp(s_ble_obj.mac, &data[3], 6) && xTaskGetTickCount() - s_ble_obj.last_time_running_config >= pdMS_TO_TICKS(3000)) {
            os_trace("recv repeated request link\r\n");
            s_ble_obj.step = WAIT_CONFIG;
            s_ble_obj.last_time_running_config = xTaskGetTickCount();
            // TODO: send to mqtt: has a device need to pair
        }
    }
    else if (s_ble_obj.is_run_config == 0)
    {
        os_trace("recv first request link\r\n");
        memcpy(s_ble_obj.mac, &data[3], 6);
        if (len > 10)
        {
            s_ble_obj.type_device_config = data[10];
        }
        else
        {
            s_ble_obj.type_device_config = DEVICE_TYPE_CTLK;
        }
        s_ble_obj.is_run_config = 1;
        s_ble_obj.last_time_running_config = xTaskGetTickCount();
        s_ble_obj.step = WAIT_CONFIG;
        // TODO: send to mqtt: has a device need to pair
    }
}

void slave_proc_response_request_link(uint8_t *data, uint8_t len)
{
    
    if (s_ble_obj.is_run_config == 2 && s_ble_obj.step == SEND_REQUEST_LINK) {
        bt_addr_le_t adv_addr;
        bt_get_local_public_address(&adv_addr);
        if (!memcmp(&data[10], adv_addr.a.val, 6)) {
            memcpy(s_ble_obj.mac, &data[3], 6);
            s_ble_obj.step = RESPOND_TO_REQUEST_LINK;
        }
    }    
}

void master_proc_rcv_verify_link(uint8_t *data, uint8_t len)
{
    if (s_ble_obj.is_run_config == 1 && s_ble_obj.step == WAIT_VERIFY_ACCEPT_LINK) {
        bt_addr_le_t adv_addr;
        bt_get_local_public_address(&adv_addr);
        if (!memcmp(&data[10], adv_addr.a.val, 6) && !memcmp(&data[3], s_ble_obj.mac, 6)) {
            s_ble_obj.step = MASTER_LINK_SUCCESS;
        }
    }
}

void ble_go_request_pair(void)
{
    s_ble_obj.step = SEND_REQUEST_LINK;
}

static void check_config_timeout(void)
{
	if (s_ble_obj.is_run_config == 1 && xTaskGetTickCount() - s_ble_obj.last_time_running_config >= pdMS_TO_TICKS(20000))
	{
		s_ble_obj.is_run_config = 0;
		os_trace("check_config_timeout: TIMEOUT CONFIG\r\n");
		// led_timeout_config();
		s_ble_obj.step = NOTHING;
		// m_db_backup_database();
		// ble_data_init();
	}
}

static uint8_t ble_db_add_child(uint8_t* mac, uint8_t type)
{
    if (-1 != is_existed_in_db(mac)) {
        os_trace("device existed\r\n");
        return 0;
    }
    memcpy(obj_ble.list_child[obj_ble.num_devices_link].mac, mac, 6);
    obj_ble.list_child[obj_ble.num_devices_link].data.holder = 0;
    obj_ble.list_child[obj_ble.num_devices_link].type = type;
    if (type == DEVICE_TYPE_CBCT) {
        obj_ble.time_delay_light_off[obj_ble.num_devices_link] = DEFAULT_DELAY_LIGHT_OFF_CBCT;
    }
    obj_ble.num_devices_link++;
    // sync database
    m_db_get_instance()->_request_sync_db_ble();
    return 1;
}

void ble_config_loop(void)
{

    switch (s_ble_obj.step)
    {
    case WAIT_VERIFY_ACCEPT_LINK:
    case WAIT_CONFIG: // step_2: -> master
    {
        check_config_timeout();
    }
    break;

    case SEND_REQUEST_LINK: // step_1: slave ->
    {

        if (s_ble_obj.is_run_config == 0)
        {
            os_trace("SEND_REQUEST_LINK: 0\r\n");
            s_ble_obj.is_run_config = 2;
            // s_ble_obj.last_time_start_config = sys_get_tick_ms();
            // adv_wait_config();
            adv_send_request_link();
        }
        else if (s_ble_obj.is_run_config == 2)
        {
            os_trace("SEND_REQUEST_LINK: 1\r\n");
            adv_send_request_link();
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
    break;

    case RESPOND_TO_REQUEST_LINK: // step_3: -> master
    {
        os_trace("RESPOND_TO_REQUEST_LINK: success\r\n");
        
        slave_response_request_link();
        s_ble_obj.step = SLAVE_LINK_SUCCESS;
    }
    break;

    case MASTER_LINK_SUCCESS:
    {
        os_trace("MASTER_LINK_SUCCESS, type: %u\r\n", s_ble_obj.type_device_config);
        ble_db_add_child(s_ble_obj.mac, s_ble_obj.type_device_config);

        s_ble_obj.is_run_config = 0;
        memset(s_ble_obj.mac, 0, 6);

        s_ble_obj.step = NOTHING;
    }
    break;

    case SLAVE_LINK_SUCCESS:
    {
        os_trace("SLAVE_LINK_SUCCESS, type: %u\r\n", s_ble_obj.type_device_config);
        ble_db_add_child(s_ble_obj.mac, s_ble_obj.type_device_config);

        s_ble_obj.is_run_config = 0;
        memset(s_ble_obj.mac, 0, 6);
        s_ble_obj.step = NOTHING;
    }
    break;

    default:
        break;
    }
}

void m_ble_get_mac_addr(uint8_t *my_mac)
{
    bt_addr_le_t adv_addr;
    bt_get_local_public_address(&adv_addr);
    memset(my_mac, 0, 6);
    memcpy(my_mac, adv_addr.a.val, 6);
}

m_ble_info_t* m_ble_get_info_reference(void)
{
    return &obj_ble;
}