# Lệnh UART giữa WB2 và CI13162 (theo code hiện tại)

Liệt kê từ code firmware WB2 trên branch `voiceboxv1/taodc/dev_261102`. Đây là những gì
WB2 **thật sự gửi và chấp nhận**, không phải spec phía chip. Spec phía chip ở
[commands.md](commands.md). Chỗ hai bên lệch nhau ghi ở mục 7.

Nguồn chính (dưới `smarthome_devices/components/`):

| File | Nội dung |
|---|---|
| `hardware/aiio_ci13162_uart.c` | Driver UART, parser JSON, gửi lệnh, dispatch |
| `hardware/aiio_ci13162_config.h` | Chân, baud, timeout |
| `application/app_cwsl.c` | Học / xóa / dừng / hỏi mẫu giọng nói (CWSL) |
| `application/app_volume.c` | Âm lượng loa |
| `hardware/buzz.c` | Beep |
| `middle/ota/m_ci13162_ota.c`, `ci13162_ota_protocol.h` | OTA nhị phân |

## 1. Kết nối và cấu hình

| Thuộc tính | Giá trị |
|---|---|
| UART WB2 | UART1 (driver tự mở thiết bị hosal UART1) |
| TX WB2 → RX chip | **GPIO4** |
| RX WB2 ← TX chip | **GPIO3** |
| Nguồn chip | **GPIO17**, mức cao = bật |
| Cấu hình | 115200, 8N1, không flow control |
| Log debug WB2 | UART0, TX GPIO16, RX GPIO7, 115200 (macro SDK `CUSTOM_LOG_IO` trong `Makefile`), tách riêng khỏi UART chip |

Lúc boot, `aiio_ci13162_uart_hold_tx_idle()` giữ GPIO4 mức cao ngay đầu `main()`, trước khi init UART1, để byte rác của Boot2 không lọt sang chip.

## 2. Frame JSON và luật parser phía WB2

```text
{"cmd":<id>,"data":[<v0>,<v1>,...]}\r\n
{"cmd":<id>,"data":[<v0>],"cmd_id":<id>,"cwsl":<0..14|255>}\r\n   # lệnh thoại
```

Luật nhận (chip → WB2), theo `aiio_ci13162_process_byte()` và `aiio_ci13162_parse_line()`:

- Mỗi frame phải kết thúc bằng đúng `\r\n`. Gặp `\n` đứng một mình, `\r` theo sau không phải `\n`, hoặc byte `0x00` thì bỏ cả dòng.
- Dòng dài tối đa **63 ký tự** (không tính `\r\n`). Dòng dài hơn bị bỏ, không báo lỗi.
- Chỉ chấp nhận 4 key: `cmd`, `data`, `cmd_id`, `cwsl`. Bắt buộc có `cmd` và `data`.
- Lệnh thoại (`cmd` 1, 2, 7, 12, 13) **bắt buộc** có `cmd_id` và `cwsl`. Lệnh khác **không được** có hai key này.
- `data` tối đa 4 phần tử. Các lệnh 0, 1, 2, 7, 12, 13, 14 chỉ được 1 phần tử. Giá trị là uint8, riêng phần tử đầu của `cmd:3` được đọc là uint32.
- `cwsl`: `0..14` (nói mẫu đã học) hoặc `255` (nói câu gốc).
- Frame bị từ chối được đẩy lên MQTT dưới dạng `{"<dev_type>":299,"raw":"<dòng>","reason":"<lý do>"}`. Lý do có thể là `INVALID_FORMAT`, `NUMBER_OVERFLOW`, `UNKNOWN_COMMAND` hoặc `INVALID_DATA`.

## 3. WB2 → CI13162

| `cmd` | Frame | Hàm gửi | Khi nào gửi |
|---:|---|---|---|
| 0 | `{"cmd":0,"data":[]}` | `aiio_ci13162_uart_ota_send_app_reset()` | Chỉ trong OTA, khi reset mode là `SOFT_ONLY`, hoặc `POWER_FALLBACK` mà sau 5 s chưa handshake được. Mode hiện tại là `POWER_ONLY`, nên **không gửi**: chip được reset bằng cách ngắt nguồn GPIO17 trong 1000 ms |
| 3 | `{"cmd":3,"data":[]}` | `aiio_ci13162_uart_request_version()` | Tự động: 3 s sau `aiio_ci13162_uart_init()`, thử lại mỗi 1 s, tối đa 3 lần, dừng khi có phản hồi hợp lệ. Tạm dừng khi đang có thao tác CWSL hoặc OTA |
| 4 | `{"cmd":4,"data":[1,<cmd_id>,<slot>]}` | `aiio_ci13162_uart_send_cwsl()` | LEARN. MQTT 220 gửi đúng slot yêu cầu. MQTT 227 (thêm ngữ cảnh) gửi slot `255`. Code **không** gửi phần tử thứ 4 (`value`) |
| 4 | `{"cmd":4,"data":[2,<cmd_id>,<slot>]}` | như trên | DELETE một mẫu (MQTT 221, hoặc xóa slot cũ sau khi học lại ngữ cảnh) |
| 4 | `{"cmd":4,"data":[2,<cmd_id>,255]}` | như trên | DELETE mọi mẫu của `cmd_id` (MQTT 221, MQTT 228 xóa ngữ cảnh) |
| 4 | `{"cmd":4,"data":[2,255,255]}` | như trên | DELETE toàn bộ (MQTT 221 với `cmd_id`=`slot`=255, hoặc khi reset nhà máy) |
| 4 | `{"cmd":4,"data":[4]}` | như trên | STOP (MQTT 222, hoặc sau 120 s học mà không có kết quả) |
| 4 | `{"cmd":4,"data":[6,<cmd_id>]}` | như trên | QUERY bitmap slot của `cmd_id` (MQTT 223, hoặc trước khi học ngữ cảnh) |
| 4 | `{"cmd":4,"data":[6,255]}` | như trên | QUERY sức chứa (MQTT 223 với `cmd_id` 255) |
| 8 | `{"cmd":8,"data":[1..7]}` | `aiio_ci13162_uart_send_volume()` | MQTT 225 (đặt âm lượng). Giá trị ngoài `1..7` bị chặn, không gửi |
| 11 | `{"cmd":11,"data":[]}` | `aiio_ci13162_uart_query_volume()` | MQTT 226 (hỏi âm lượng) |
| 200 | `{"cmd":200,"data":[0]}` | `aiio_ci13162_uart_send_buzz_type(0)` | Beep ngắn (`hw_buzz_set_type()` trên thiết bị có chip giọng nói) |
| 200 | `{"cmd":200,"data":[1]}` | `aiio_ci13162_uart_send_buzz_type(1)` | Beep dài, hoặc nâng beep ngắn đang kêu thành beep dài |

Mọi lệnh gửi đều bị chặn (trả `-1`) khi OTA đang giữ UART1. Các lệnh TX đi qua một mutex chung, chờ tối đa 50 ms.

Ràng buộc phía WB2 trước khi gửi `cmd:4` (`app_cwsl_parse_mutating()`, `app_cwsl_parse_query()`):

- `cmd_id` hợp lệ là `1..65`, hoặc `255` (không dùng cho LEARN).
- Bị từ chối: `cmd_id` `2..6`; `54`, `55` (tắt/bật tiếng); `56..65` qua MQTT 220/221 (ngữ cảnh phải đi qua MQTT 227/228).
- Slot: wake (`cmd_id` 1) là `0..4`; lệnh khác là `0..14`; `255` nghĩa là tự chọn.
- `VOICEBOX`: MQTT 220..223 chỉ nhận `cmd_id` 1 hoặc 255.
- Chỉ chạy một thao tác CWSL tại một thời điểm. Đang chờ phản hồi âm lượng hoặc đang xử lý ngữ cảnh thì trả `BUSY` (67); đang OTA thì trả `OTA_HOLD` (69).

## 4. CI13162 → WB2

| `cmd` | `data` hợp lệ | `cmd_id` bắt buộc | Xử lý trên WB2 |
|---:|---|---|---|
| 0 | `[]` | — | **Reset chính WB2** (`bl_sys_reset_por()`) |
| 1 TURN | `[1]` / `[0]` | `[1]`→7/8, `[0]`→9/10 | Chỉ báo MQTT 224 (nhánh quạt không còn áp dụng) |
| 2 MODE | `[1..6]` | 1→11/12, 2→13/14, 3→15/16, 4→17/18, 5→19..21, 6→22..24 | Chỉ báo MQTT 224 (nhánh quạt không còn áp dụng) |
| 3 VERSION | `[<uint32 > 0>]` | — | Lưu cache (`aiio_ci13162_uart_get_voice_version()`) |
| 5 CWSL_STATUS | `[code, cmd_id, slot]` | — | `app_cwsl_on_status()` |
| 6 CWSL_INFO | `[cmd_id(1..65), bm_lo, bm_hi]` hoặc `[255, wake, ncmd, nfree]` | — | `app_cwsl_on_info()` → MQTT 223 |
| 7 TIMER | `[0..12]` | `[0]`→47; `[n]`→`23+2(n-1)` hoặc `+1` | Chỉ báo MQTT 224 (nhánh quạt không còn áp dụng) |
| 9 VOLUME_INFO | `[level(0..7), result(0/1)]` hoặc thêm phần tử thứ 3 | — | Lưu cache `level`, gọi callback `app_volume`. Phần tử thứ 3 nhận nhưng **bỏ qua** |
| 10 ASR_UNHANDLED | `[cmd_id]` | — | Chỉ ghi log |
| 12 SCENE | `[1..10]` | `55 + data` (56..65) | Báo MQTT 224 (chỉ khi ngữ cảnh đó đang được cấu hình) |
| 13 VOLUME_CHANGE | `[0..7]` | 48..55. 54 → `[0]`; 50/51 → `[1]`; 52/53 → `[7]`; 48/49/55 → `[1..7]` | Lưu cache, báo MQTT 224, `app_volume_report_voice()` |
| 14 STATE | `[0]` | — | Nhận rồi bỏ |
| 200 BUZZ | `[]`, `[0]`, `[1]` | — | Nhận rồi bỏ |
| 4, 8, 11, khác | — | — | `UNKNOWN_COMMAND` → MQTT 299 |

Frame thoại chỉ được xử lý trên `VOICEBOX` (chỉ thiết bị này dùng chip giọng nói; `fanproacv2` không còn). Nhánh điều khiển quạt cho cmd 1/2/7 vẫn còn trong code nhưng không áp dụng cho thiết bị nào trong firmware này.

MQTT 224 do `app_cwsl_notify_voice_cmd(cmd_id, cwsl)` gửi. Hàm này bỏ qua `cmd_id` bằng 0 hoặc lớn hơn 65, và gộp `cmd_id` `1..6` thành `1`.

## 5. Mã trạng thái CWSL (`cmd:5`, `data[0]`)

| Mã | Tên trong code | Nguồn |
|---:|---|---|
| 32 (0x20) | `LEARN_OK` | chip |
| 33 (0x21) | `DELETE_OK` | chip |
| 34 (0x22) | `EXIT_OK` | chip |
| 52 (0x34) | `TIMEOUT` | chip, hoặc WB2 tự sinh khi hết hạn chờ |
| 64 (0x40) | `BAD_REQUEST` | chip / WB2 |
| 65 (0x41) | `BAD_COMMAND_ID` | chip / WB2 |
| 66 (0x42) | `BAD_SLOT` | chip / WB2 |
| 67 (0x43) | `BUSY` | chip / WB2 (lúc xóa slot cũ sau khi học lại ngữ cảnh, nếu chip trả BUSY thì WB2 gửi lại tối đa 2 lần) |
| 68 (0x44) | `NOT_IN_LEARN_MODE` | chip |
| 69 | `OTA_HOLD` | chỉ WB2, báo qua MQTT, không đi trên UART |
| 70 | `TX_FAIL` | chỉ WB2, báo qua MQTT, không đi trên UART |

Code xếp loại theo `code >> 4` và chỉ chốt phiên ở lớp `2` (thành công), `3` (thất bại), `4` (bị từ chối). Mã lớp `0` và `1` (tiến trình, ví dụ `0x10..0x12`) bị bỏ qua, phiên vẫn chờ tiếp.

Timeout phía WB2: thao tác học/xóa chờ 120 s (hết hạn thì gửi STOP), query chờ 2 s, âm lượng chờ 2 s.

## 6. OTA nhị phân (MQTT 134)

Khi OTA chạy, `aiio_ci13162_uart_ota_acquire()` chiếm UART1: ngừng parser JSON, chuyển byte nhận sang stream OTA. Log UART0 không bị tắt và không đổi baud. Khi xong, `aiio_ci13162_uart_ota_release()` trả baud UART1 về 115200 và bật lại giao thức JSON.

Frame (`ci13162_ota_protocol.c`):

```text
A5 0F | len (2 byte, LE, độ dài payload) | type | cmd | seq | payload | CRC16 (2 byte, LE) | FF
```

CRC16-CCITT, đa thức `0x1021`, giá trị đầu `0`, tính trên `type, cmd, seq` và payload.

| `type` | Ý nghĩa |
|---|---|
| `0xA0` | CMD |
| `0xA1` | REQ |
| `0xA2` | ACK |
| `0xA3` | NOTIFY |

| `cmd` | Tên | Chiều / cách dùng |
|---|---|---|
| `0x83` | HANDSHAKE | WB2 gửi REQ mỗi 20 ms cho tới khi chip trả ACK (tối đa 30 s) |
| `0x85` | (maskrom control) | WB2 gửi CMD 7 lần, cách nhau 15 ms, rồi chờ ACK |
| `0x0B` | CHANGE_BAUD | WB2 → chip, chờ ACK rồi đổi baud phía WB2 (460800 khi nạp agent, 921600 khi ghi firmware) |
| `0x0C` | TEST_BAUD | WB2 → chip, payload `0x10` (u32 LE) |
| `0x06` | BLOCK_INFO | WB2 → chip, khai báo vùng sắp ghi |
| `0x07` | ERASE | WB2 → chip |
| `0x08` | WRITE | Chip gửi REQ xin dữ liệu, WB2 trả phần dữ liệu tương ứng |
| `0x09` | BLOCK_DONE | Chip gửi REQ báo xong một khối |
| `0x0A` | VERIFY | WB2 → chip |
| `0x8A` | VERIFY_AGENT | WB2 → chip, sau khi nạp agent |
| `0x05` | CHECK_READY | WB2 → chip |
| `0x12` | SET_FW_FMT | WB2 → chip |
| `0x04` | GET_INFO | WB2 → chip, chờ ACK tối đa 35 s |
| `0x0E` | COMPLETE | WB2 → chip, bước cuối |
| `0x11` | PROGRESS | Chip gửi NOTIFY bất kỳ lúc nào |

Trình tự trong `ci13162_ota_task()`:

1. Tải agent qua HTTP.
2. Chiếm UART1.
3. Vào maskrom: baud 115200, ngắt nguồn chip qua GPIO17, gửi HANDSHAKE liên tục.
4. Gửi `0x85`, rồi đổi baud UART1 lên 460800.
5. Nạp agent (TEST_BAUD, BLOCK_INFO, phục vụ WRITE/BLOCK_DONE, VERIFY_AGENT).
6. Khởi tạo updater (CHECK_READY, SET_FW_FMT, GET_INFO).
7. Ghi firmware ở 921600 (BLOCK_INFO, ERASE, phục vụ WRITE/BLOCK_DONE, VERIFY).
8. Xóa vùng NV nếu được yêu cầu.
9. Gửi COMPLETE, trả UART1.

## 7. Chỗ code WB2 lệch với `commands.md` (spec phía chip)

| Mục | `commands.md` | Code WB2 hiện tại | Hậu quả |
|---|---|---|---|
| `CMD_SCENE` (12) | Ngữ cảnh `[1..15]` (`cmd_id` 56..70), lệnh nhà `[71..103]` với `data` = `cmd_id` | Chỉ nhận `data` `1..10` và `cmd_id` = `55 + data` | Ngữ cảnh 11..15 và **toàn bộ lệnh nhà 71..103 bị từ chối** (`INVALID_DATA` → MQTT 299). Ngay cả khi parser cho qua, `app_cwsl_notify_voice_cmd()` vẫn bỏ `cmd_id` > 65 |
| `CMD_CWSL_INFO` (6) | `cmd_id` theo `cmd_info` (tới 103) | `data[0]` chỉ nhận `1..65` hoặc `255` | Không hỏi được slot của lệnh nhà mới |
| Lệnh quạt 1, 2, 7, 14 | Đã bỏ | Vẫn parse; chỉ báo MQTT 224, nhánh quạt không áp dụng | Code thừa, vô hại |
| `CMD_VOLUME` (8) | `[8]` tắt tiếng, `[9]` bật tiếng | Chỉ gửi `1..7` | WB2 không tắt/bật tiếng được |
| `CMD_VOLUME_INFO` (9) | `[level, result, muted]` | Bỏ qua `muted` | Cache chỉ có mức, không biết đang tắt tiếng |
| `CMD_CWSL` LEARN | Có dạng 4 phần tử, kèm `value` câu trả lời | Chỉ gửi 3 phần tử | Không đặt được câu trả lời khi học |
| `CMD_RESP_SET/GET` (15/16), `CMD_VOICE_SET/GET` (17/18) | Có | Chưa cài | Chip trả về thì WB2 báo `UNKNOWN_COMMAND` (MQTT 299) |
| 201..204 (bản debug của chip) | Có | Chưa cài | Nếu chip chạy bản debug, frame 201 bị báo MQTT 299. Frame 202 có key `log` cũng bị từ chối, hoặc bị bỏ vì dài quá 63 ký tự |
| `CMD_RESET` (0) chiều chip → MCU | Không mô tả | WB2 tự reset khi nhận | Cần xác nhận chip có gửi frame này không |
| `CMD_BEEP` (200) chiều chip → MCU | Một chiều | WB2 nhận rồi bỏ | Không ảnh hưởng |
