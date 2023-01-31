#include "my_ble.h"
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include "gatt.h"
#include "conn.h"
#include "conn_internal.h"
#include "hci_core.h"

static char ble_name[64];
// #define NAME_DEVICE "HUN167291413297wswitch2vN1"

/*自定义UUID*/
/* service UUID: 0x6e400001-0xb5a3-0xf393-0xe0a9-0xe50e24dcca9e */
/* rx chracteristic: 0x6e400002-0xb5a3-0xf393-0xe0a9-0xe50e24dcca9e */
/* notify chracteristic: 0x6e400003-0xb5a3-0xf393-0xe0a9-0xe50e24dcca9e */
#define MY_UUID1_USER_SER    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e))
#define MY_UUID1_USER_TXD    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)) /* phone tx */
#define MY_UUID1_USER_RXD    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)) /* phone rx */

#define MY_UUID2_USER_SER    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x10190d0c, 0x0b0a, 0x0908, 0x0706, 0x050403020100))
#define MY_UUID2_USER_TRD    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x102B0d0c, 0x0b0a, 0x0908, 0x0706, 0x050403020100))			
my_ble_recv_data_cb_t static_ble_recv_data_cb = NULL;
my_ble_send_data_cb_t static_ble_send_data_cb = NULL;
my_ble_connet_status_cb_t static_ble_connet_status_cb = NULL;
static struct bt_gatt_exchange_params exchange_params;

static void ccc_cfg_changed(const struct bt_gatt_attr *attr,u16_t value);
static void connected(struct bt_conn *conn, u8_t err);
static void disconnected(struct bt_conn *conn, u8_t reason);
static void le_param_updated(struct bt_conn *conn, u16_t interval,u16_t latency, u16_t timeout);

static void bt_ready_cb(int err);
static ssize_t read_data(struct bt_conn *conn,const struct bt_gatt_attr *attr, void *buf,u16_t len, u16_t offset);
static ssize_t write_data(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,u16_t len, u16_t offset, u8_t flags);

/*ble 属性表*/
static struct bt_gatt_attr ble_Attributes[]=
{
    /*自定义主服务*/
	BT_GATT_PRIMARY_SERVICE(MY_UUID1_USER_SER),
    BT_GATT_CHARACTERISTIC(MY_UUID1_USER_TXD, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE, NULL, write_data, NULL),
    BT_GATT_CHARACTERISTIC(MY_UUID1_USER_RXD, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, read_data,NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    // BT_GATT_PRIMARY_SERVICE(MY_UUID2_USER_SER),
    // BT_GATT_CHARACTERISTIC(MY_UUID2_USER_TRD, BT_GATT_CHRC_NOTIFY|BT_GATT_CHRC_WRITE|BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_READ|BT_GATT_PERM_WRITE,read_data,write_data,NULL),
    // BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static void ccc_cfg_changed(const struct bt_gatt_attr *attr,u16_t value)
{
    printf("ccc change %d", value);
}

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
    pconn=conn;
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	if (err) 
    {
		printf("Failed to connect to %s (%u)", addr, err);
		return;
	}
	printf("Connected: %s \r\n", addr);
    if(static_ble_connet_status_cb != NULL)
    {
        static_ble_connet_status_cb(MY_BLE_DEVICE_CONNECT, addr);
    }
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
    pconn=conn;
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));  
	printf("Disconnected: %s (reason %u)", addr, reason);
    if(static_ble_connet_status_cb != NULL)
    {
        static_ble_connet_status_cb(MY_BLE_DEVICE_DISCONNECT, addr);
    }
}


static void le_param_updated(struct bt_conn *conn, u16_t interval,u16_t latency, u16_t timeout)
{
   printf("LE conn param updated: int 0x%04x lat %d to %d", interval, latency, timeout);
}

/*广播包数据*/
static struct bt_data adv_data[2] = 
{
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, "123456", 6),
};

void my_ble_start_advertise(void)
{
    struct bt_le_adv_param param;
	int err = 0;
    param.id = BT_ID_DEFAULT;
    param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
    param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
    param.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_ONE_TIME);
    /*Get mode, 0:General discoverable,  1:non discoverable, 2:limit discoverable*/
    err = bt_le_adv_start(&param, adv_data, ARRAY_SIZE(adv_data),NULL,0);//rsp_data, ARRAY_SIZE(rsp_data));
    if(err)
    {
       printf("Failed to start advertising (err %d)", err);
    }
    else
    {
        printf("Advertising started");
    }
}

static void bt_ready_cb(int err)
{
    if (err != 0) 
    {
        return;
    }
    printf("bt_ready");
}

static ssize_t read_data(struct bt_conn *conn,const struct bt_gatt_attr *attr, void *buf,u16_t len, u16_t offset)
{
    if(static_ble_send_data_cb != NULL)
    {
        static_ble_send_data_cb(conn, buf, len);
    }
    return len;
}

static ssize_t write_data(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,u16_t len, u16_t offset, u8_t flags)
{
    if(static_ble_recv_data_cb != NULL)
    {
        static_ble_recv_data_cb(conn, buf, len);
    }
    return len;
}

struct bt_gatt_service BLE_GATT_SER = BT_GATT_SERVICE(ble_Attributes);
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
};


/*BLE发送函数*/
void my_ble_notify(struct bt_conn *conn, char *buf, u16_t len)
{
    bt_gatt_notify(conn, notify_attrs, buf, len);
}

static void ble_stack_start(void)
{
    // Initialize BLE controller
    ble_controller_init(configMAX_PRIORITIES - 1);
    // Initialize BLE Host stack
    hci_driver_init();
    bt_enable(bt_ready_cb);
}

/*BLE初始化函数*/
void my_ble_init(void)
{                                                        
    printf("my_ble_init\r\n");

    ble_stack_start();
    bt_gatt_service_register((struct bt_gatt_service *)&BLE_GATT_SER); //注册GATT服务     
    notify_attrs = &ble_Attributes[4];   //对应notify 
    bt_conn_cb_register(&conn_callbacks);
    
    my_ble_start_advertise();//开启广播

    printf("BLE naem: %s\r\n", ble_name);
    bt_set_name(ble_name); //设置蓝牙名称  
}
/*注册蓝牙接收回调*/
void my_ble_recv_register(my_ble_recv_data_cb_t ble_recv_cb)
{
    static_ble_recv_data_cb = ble_recv_cb;
}
/*注册蓝牙连接回调*/
void my_ble_connect_register(my_ble_connet_status_cb_t ble_connet_cb)
{
    static_ble_connet_status_cb = ble_connet_cb;
}

static void ble_disconnect_all(struct bt_conn *conn, void *data)
{
    if (conn->state == BT_CONN_CONNECTED)
    {
        printf("[BLE] disconn id:%d \r\n", conn->id);
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

void my_ble_deinit(void)
{
    printf("my_ble_deinit\r\n");

    bt_conn_foreach(BT_CONN_TYPE_ALL, ble_disconnect_all, NULL);

    /* wait for all connection to be disconnected */
    int disconn_cnt = 0;
    while (le_check_valid_conn() && disconn_cnt++ < 10) {
        printf("[BLE] wait for ble_disconnect_all\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    bt_gatt_service_unregister((struct bt_gatt_service *)&BLE_GATT_SER);
    
    bt_le_adv_stop();

    int err = bt_disable();
    if (err)
    {
        printf("Fail to disable bt, there is existed scan/adv/conn event\r\n");
    }
    else
    {
        printf("Disable bt successfully\r\n");
    }
}

void my_ble_set_name(char* name)
{
    // bt_le_adv_stop();
    // my_ble_start_advertise();
    // bt_set_name(name);
    int len = strlen(name) > sizeof(ble_name) ? sizeof(ble_name): strlen(name);
    memset(ble_name, 0, sizeof(ble_name));
    memcpy(ble_name, name, len);
}