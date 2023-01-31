#ifndef _my_BLE_H_
#define _my_BLE_H_

#include <stdint.h>
#include "conn.h"
#include "hci_driver.h"
#include "ble_lib_api.h"
#include "bluetooth.h"
#include "gatt.h"
#include "uuid.h"
enum {
    MY_BLE_DEVICE_CONNECT = 0,//连接状态
    MY_BLE_DEVICE_DISCONNECT, //断开状态
};
unsigned char bleuart_connect_status;
struct bt_conn *pconn;
static struct bt_gatt_attr *notify_attrs;
typedef void (*my_ble_recv_data_cb_t)(struct bt_conn *conn, const char *buf, u16_t len);
typedef void (*my_ble_send_data_cb_t)(struct bt_conn *conn, const char *buf, u16_t len);
typedef void (*my_ble_connet_status_cb_t)(uint8_t status, char *addr);
void my_bleuart_send(char *buf, u16_t len);
void my_ble_notify(struct bt_conn *conn, char *buf, u16_t len);
void my_ble_start_advertise();
void my_ble_init(void);
void my_ble_deinit(void);
void my_ble_recv_register(my_ble_recv_data_cb_t ble_reve_cb);
void my_ble_connect_register(my_ble_connet_status_cb_t ble_connet_cb);
void my_ble_set_name(char* name);
/*自定义UUID*/
/* service UUID: 0x6e400001-0xb5a3-0xf393-0xe0a9-0xe50e24dcca9e */
/* rx chracteristic: 0x6e400002-0xb5a3-0xf393-0xe0a9-0xe50e24dcca9e */
/* notify chracteristic: 0x6e400003-0xb5a3-0xf393-0xe0a9-0xe50e24dcca9e */

// #define MY_UUID1_USER_SER    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e))
// #define MY_UUID1_USER_TXD    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)) /* phone tx */
// #define MY_UUID1_USER_RXD    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)) /* phone rx */

// #define MY_UUID2_USER_SER    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x10190d0c, 0x0b0a, 0x0908, 0x0706, 0x050403020100))
// #define MY_UUID2_USER_TRD    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x102B0d0c, 0x0b0a, 0x0908, 0x0706, 0x050403020100))

// static struct bt_gatt_attr ble_Attributes[]=
// {
// 	BT_GATT_PRIMARY_SERVICE(MY_UUID1_USER_SER),
//     BT_GATT_CHARACTERISTIC(MY_UUID1_USER_TXD, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE, NULL, write_data, NULL),
//     BT_GATT_CHARACTERISTIC(MY_UUID1_USER_RXD, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, read_data,NULL, NULL),
//     BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
// };

#endif