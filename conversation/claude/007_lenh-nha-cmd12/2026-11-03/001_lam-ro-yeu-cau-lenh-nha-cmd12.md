# Làm rõ yêu cầu: WB2 xử lý lệnh nhà 71..103 qua `cmd 12`

## Hỏi

WB2 bắn `{"voiceboxv1":299,"raw":"{"cmd":12,"data":[102],"cmd_id":102,"cwsl":255}","reason":"INVALID_DATA"}` khi người dùng nói "BẬT-ĐÈN". Lên plan sửa. Không đoán; cần thông tin thì hỏi.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Hiện trạng (đọc code HEAD `6fe9bffa`)

- Chip gửi lệnh nhà đúng spec (`commands.md` mục 2.1): `{"cmd":12,"data":[<cmd_id>],"cmd_id":<cmd_id>,"cwsl":255}`, `cmd_id` 71..103.
- `aiio_ci13162_validate_message()` (`aiio_ci13162_uart.c:843-854`) chỉ nhận `data` 1..15 với `cmd_id = 55 + data`. Lệnh nhà → `INVALID_DATA` → MQTT 299. Không có LED, BLE ADV hay 224.
- `app_cwsl_notify_voice_cmd()` (`app_cwsl.c:1451`) bỏ `cmd_id` > 70 (`APP_CWSL_CMD_ID_MAX`).
- `m_ble_send_voicebox_state(uint8_t cmd_id)` nhận được 71..103, không cần sửa.

## Quyết định (người dùng trả lời qua bảng hỏi, duyệt phương án trong chat)

1. Lệnh nhà hợp lệ: WB2 làm cả 3 việc như ngữ cảnh: nháy LED 200 ms, phát BLE ADV với `cmd_id`, bắn MQTT 224.
2. MQTT 224 cùng format ngữ cảnh: `{"voiceboxv1":224,"cmd_id":102,"cwsl":255}`. `cwsl` là giá trị chip gửi.
3. Phạm vi: chỉ đường nhận `cmd 12`. Không mở rộng 220/221/223 và frame `cmd 6` cho 71..103.
4. Frame `cmd 12` sai (`cmd_id` ngoài 71..103, hoặc `data[0]` khác `cmd_id`): giữ cơ chế hiện tại, `INVALID_DATA` → MQTT 299.
5. Lệnh nhà không kiểm "đã kích hoạt" như ngữ cảnh.

Lựa chọn của Claude (đã nêu trong phương án, người dùng duyệt):

- Không nâng `APP_CWSL_CMD_ID_MAX` (giữ 70) vì hằng này dùng chung cho 220/221/223 và `cmd 6`. Thêm hằng riêng 71..103.
- Sửa `docs/voice/uart_wb2_ci13162.md` mục 4 và mục 7. Chạy 4 runner và build firmware. Không commit.

## Chưa kiểm

- Chưa sửa code.
