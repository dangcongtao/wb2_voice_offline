# Nguyên nhân MQTT 299 với frame `cmd 12` `data [102]`

## Hỏi

WB2 gửi lên MQTT `{"voiceboxv1":299,"raw":"{"cmd":12,"data":[102],"cmd_id":102,"cwsl":255}","reason":"INVALID_DATA"}`. Vì sao?

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Đã làm

- Chip nhận câu `BẬT-ĐÈN` (`cmd_id` 102, lệnh nhà) và gửi frame đúng spec chip (`commands.md` mục 2.1): lệnh nhà 71..103 gửi `cmd 12` với `data[0] = cmd_id`.
- WB2 `aiio_ci13162_validate_message()` (`aiio_ci13162_uart.c:840-850`) chỉ nhận `cmd 12` dạng ngữ cảnh:
  - `data[0]` phải trong 1..15 (`APP_SCENE_COUNT_MAX`);
  - `cmd_id` phải bằng `55 + data[0]`.
- 102 > 15 nên frame bị trả `INVALID_DATA`. Driver không chạy lệnh mà đẩy dòng gốc lên MQTT 299.
- Dù frame có qua được, `app_cwsl_notify_voice_cmd()` (`app_cwsl.c:1444`) vẫn bỏ `cmd_id` > 70 (`APP_CWSL_CMD_ID_MAX`), nên không có MQTT 224.
- Lỗi lệch này đã ghi sẵn ở `docs/voice/uart_wb2_ci13162.md` mục 7.

## Không làm

- Không sửa code.

## Chưa kiểm

- Chưa kiểm trên board.
