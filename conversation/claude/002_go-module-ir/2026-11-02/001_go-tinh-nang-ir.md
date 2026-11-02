# Gỡ module IR khỏi firmware

## Hỏi

Xóa module `components/middle/ir`.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định

- Chỉ xóa thư mục thì build hỏng, vì `app_ir.c`, `process_data.c`, `main.c`, database và OTA đều đang dùng module này. Người dùng chọn **gỡ hẳn tính năng IR**: các thiết bị quạt build từ cây code này cũng mất IR.
- Giữ nguyên số bit `BIT_FLAG_SYNC_FAN` là 4. Bit 3 (trước là AC) để trống và có comment ghi chú.

## Đã làm

- Xóa `components/middle/ir/` (89 file) và `components/application/app_ir.c/.h`.
- Build: gỡ `ir` khỏi `middle/bouffalo.mk` (include dir và 41 file nguồn), gỡ `app_ir.c` khỏi `application/bouffalo.mk`, gỡ `middle/ir` khỏi `Makefile`.
- `switchv3/main.c`: bỏ include `IRrecv.h`, `_tolerance = kTolerance`, `m_ir_init()`.
- `process_data.c`: bỏ include IR và `app_ir.h`, bỏ các handler MQTT `cmd` 1/2/4/15/16, bỏ `app_ir_send_response_from_remote_brand_check_loop()`.
- Database: bỏ trường `_ac_info` trong `m_device_t`, bỏ `BIT_FLAG_SYNC_AC`, key `ac_info` và phần đọc/ghi AC trong `flash_api.c`.
- OTA: bỏ `m_ir_deinit()` và include ở `m_ota.c` và `m_ci13162_ota.c` (bỏ cả `#undef IR_RX`).
- `event_apconfig.c`, `event_device_active.c` (file CRLF): bỏ include và các dòng comment `m_ir_*`.
- `hardware.h`: bỏ `IR_RX`, `IR_RX_POS`, `IR_RX_DC`, `IR_RX_DC_POS`, `IRpin`. README: bỏ đoạn ghi chú IR.
- Build sạch (`make clean && make -j16`) chạy được. `switchv3.bin` giảm từ 1.047.170 xuống 925.228 byte. Không có cảnh báo mới ở các file đã sửa; mất cảnh báo `IR_RX redefined`.
- 4 test runner trên host (voicebox, cwsl, scene, uart_protocol) build bằng gcc và đều pass 100%.

## Không làm

- Không xóa key `ac_info` cũ trong easyflash của thiết bị đã chạy firmware có IR (key sẽ nằm thừa, không ai đọc).
- Không đụng `smarthome_devices/.gitignore` và `script.sh` (đã có thay đổi từ trước).

## Chưa kiểm

- Chưa flash và chạy thử trên board.
- Chưa commit.
