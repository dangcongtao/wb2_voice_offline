# Lệnh MQTT nào cần chip CI13162 trả lời qua UART

## Hỏi

Những lệnh MQTT nào có luồng phải chờ frame trả lời UART từ CI13162.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Đã làm

Đọc code trong working tree, gồm cả phần 227 `respond_id` chưa commit: bảng `array_handler` (`process_data.c:81-121`), `app_cwsl.c`, `app_scene.c`, `app_volume.c`, `aiio_ci13162_config.h`.

Các lệnh phải chờ chip trả lời:

| MQTT | UART gửi | Chờ frame | Timeout |
|---|---|---|---|
| 220 | `cmd 4 [1,cmd_id,slot]` | `cmd 5` | 120 s, hết giờ thì gửi STOP và trả code 52 |
| 221 | `cmd 4 [2,cmd_id,slot]` | `cmd 5` | 120 s |
| 222 | `cmd 4 [4]` (chỉ khi đang học) | `cmd 5` | theo phiên đang chạy |
| 223 | `cmd 4 [6,cmd_id]` | `cmd 6` | 2 s |
| 225 | `cmd 8 [1..7]` | `cmd 9` | 2 s |
| 226 | `cmd 11 []` | `cmd 9` | 2 s |
| 227 thêm | QUERY → LEARN 4 phần tử → xóa slot cũ | `cmd 6` → `cmd 5` → `cmd 5` | 2 s / 120 s / 2 s mỗi slot |
| 227 sửa | `cmd 15 [cmd_id,respond_id]` | `cmd 15 [1]/[0]` | 2 s |
| 228 | `cmd 4 [2,cmd_id,255]`; 255 thì lặp 56..70 | `cmd 5` mỗi lần | 2 s mỗi lần |

Không chờ chip: 105 (đọc cache), 123, 129, 134 (OTA nhị phân, trả MQTT trước khi reboot), 224 (chip chủ động gửi), 229 (đọc flash). 230 (đổi giọng) chưa có handler; theo `commands.md`, nếu làm thì cần `cmd 17` và chờ chip trả `cmd 17 [1]/[0]`.

## Phát hiện

- `uart_wb2_ci13162.md` mục 3 ghi DELETE `[2,255,255]` được gửi "khi reset nhà máy". Code hiện tại không còn chỗ nào gọi `app_cwsl_factory_clear()`. MQTT 123 (`app_remove_device`) cố ý giữ mẫu đã học. Câu đó trong tài liệu đã cũ.
- 228 coi code 51 (`DELETE_FAIL`) là thành công (`app_scene.c:419`, `:451`).

## Không làm

- Không sửa tài liệu, không sửa code.

## Chưa kiểm

- Chưa kiểm trên board.
