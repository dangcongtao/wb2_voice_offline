# Log nguyên nhân 220 trả result:0

## Hỏi

Khi nhận lệnh MQTT 220 mà trả `"result":0`, log nguyên nhân dạng `[ERROR]: <nguyên nhân lỗi>, in[<tên hàm>]`.

## Branch

- /home/taodc/wb2/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Quyết định

- Chỉ log cho 220. Macro `APP_CWSL_LEARN_ERROR(mqtt_cmd, fmt, ...)` trong `app_cwsl.c` chỉ in khi `mqtt_cmd == 220`. 221/222/223 dùng chung hàm parse nhưng không in.
- Log tại chỗ phát hiện lỗi, `in[...]` lấy từ `__func__`, nên chỉ đúng hàm (`app_cwsl_parse_common`, `app_cwsl_parse_mutating`, `app_cwsl_start_mutating`).
- Text log bằng tiếng Anh ASCII, giống các log `os_trace` khác, có kèm giá trị sai (cmd_id, slot, respond_id).
- Phạm vi chỉ gồm bản tin từ chối `result:0`. Final event (timeout 52, mã lỗi chip 3x/4x) không có field `result` nên không log.

## Đã làm

- `app_cwsl.c`:
  - `app_cwsl_parse_common`: tách điều kiện gộp thành từng nhánh: js_root NULL, có field `action`, field loại thiết bị thiếu/khác 220, `u` sai, `act_id` sai.
  - `app_cwsl_parse_mutating`: tách `cmd_id`/`slot` thiếu. Phân biệt respond_id thiếu/sai kiểu với respond_id ngoài tập hợp lệ. Log cmd_id=255, ngoài 1..70, 2..6 (wake factory), 56..70 (scene, dùng 227), 54/55, voicebox chỉ cho 1, slot sai cho wake (0..4) và lệnh (0..14). Đổi ternary sang if, kết quả trả về giữ nguyên.
  - `app_cwsl_start_mutating`: OTA đang chạy, volume/voice_id đang chờ chip, phiên khác đang bận (in trạng thái mutating/query/resp_set/scene), gửi UART LEARN lỗi.
- Test host `test_app_cwsl_runner.c` (thêm include `middle/gpio`, `middle/ble_slave`): 13/13 pass, `-Wall -Wextra -Wformat=2` không có warning ở `app_cwsl.c`. Log ra đúng dạng, ví dụ `[ERROR]: cmd_id=99 out of range 1..70, in[app_cwsl_parse_mutating]`. Số lần in khớp chỉ case 220 của validation test.
- Build firmware `make -j16` exit 0, `app_cwsl.o` build lại, không có warning mới. `switchv3.bin` 932.188 B.

## Không làm

- Không log cho 221/222/223 và final event lỗi của 220.
- Không sửa spec `CWSL_MQTT_SPEC.md`, không thêm assert cho text log trong test.
- Không đụng thay đổi sẵn có `FIRMWARE_VERSION` trong `app_config.h`.

## Chưa kiểm

- Chưa flash, chưa test trên thiết bị thật.
- Chưa commit.
