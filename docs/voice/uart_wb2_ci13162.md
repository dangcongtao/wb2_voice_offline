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
| `application/app_voice.c` | MQTT 230/231 đặt/đọc giọng qua cmd 17/18 |
| `application/app_button.c` | Nút nhấn VOICEBOX (âm lượng, đổi giọng) |
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
| Log debug WB2 | UART0, TX GPIO16, 115200; SDK mở UART0 chỉ TX (`hosal_uart_init_only_tx`), RX không dùng (GPIO7 chỉ xuất hiện trong macro `CUSTOM_LOG_RX_IO`). Cấu hình bằng các macro `CUSTOM_LOG_IO` / `CUSTOM_LOG_TX_IO` / `CUSTOM_LOG_RX_IO` / `CUSTOM_LOG_RX_BAUD` trong `Makefile`, tách riêng khỏi UART chip |

Lúc boot, `aiio_ci13162_uart_hold_tx_idle()` giữ GPIO4 mức cao ngay đầu `main()`, trước khi init UART1, để byte rác của Boot2 không lọt sang chip.

UART chip chỉ được khởi tạo trên VOICEBOX, sau khi `main()` đọc device ID. `hw_uart` (FT62F28, cũng dùng UART1 GPIO4/GPIO3) không được khởi tạo hay polling trên VOICEBOX.

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
- `data` tối đa 4 phần tử. Các lệnh 0, 1, 2, 7, 12, 13, 14, 15 chỉ được 1 phần tử (cmd 15: giá trị 0 hoặc 1). Giá trị là uint8, riêng phần tử đầu của `cmd:3` được đọc là uint32.
- `cwsl`: `0..14` (nói mẫu đã học) hoặc `255` (nói câu gốc).
- Frame bị từ chối được đẩy lên MQTT dưới dạng `{"<dev_type>":299,"raw":"<dòng>","reason":"<lý do>"}`. Lý do có thể là `INVALID_FORMAT`, `NUMBER_OVERFLOW`, `UNKNOWN_COMMAND` hoặc `INVALID_DATA`.

## 3. WB2 → CI13162

| `cmd` | Frame | Hàm gửi | Khi nào gửi |
|---:|---|---|---|
| 0 | `{"cmd":0,"data":[]}` | `aiio_ci13162_uart_ota_send_app_reset()` | Chỉ trong OTA, khi reset mode là `SOFT_ONLY`, hoặc `POWER_FALLBACK` mà sau 5 s chưa handshake được. Mode hiện tại là `POWER_ONLY`, nên **không gửi**: chip được reset bằng cách ngắt nguồn GPIO17 trong 1000 ms |
| 3 | `{"cmd":3,"data":[]}` | `aiio_ci13162_uart_request_version()` | Tự động: 3 s sau `aiio_ci13162_uart_init()`, thử lại mỗi 1 s, tối đa 3 lần, dừng khi có phản hồi hợp lệ. Tạm dừng khi đang có thao tác CWSL (kể cả đang chờ trả lời cmd 15) hoặc OTA |
| 4 | `{"cmd":4,"data":[1,<cmd_id>,<slot>,<respond_id>]}` | `aiio_ci13162_uart_send_cwsl()` | LEARN, luôn 4 phần tử. MQTT 220 gửi đúng slot yêu cầu kèm `respond_id` (bắt buộc trong request 220). MQTT 227 (thêm ngữ cảnh) gửi slot `255` kèm `respond_id` (bắt buộc trong request 227) |
| 4 | `{"cmd":4,"data":[2,<cmd_id>,<slot>]}` | như trên | DELETE một mẫu (MQTT 221, hoặc xóa slot cũ sau khi học lại ngữ cảnh) |
| 4 | `{"cmd":4,"data":[2,<cmd_id>,255]}` | như trên | DELETE mọi mẫu của `cmd_id` (MQTT 221, MQTT 228 xóa ngữ cảnh) |
| 4 | `{"cmd":4,"data":[2,255,255]}` | như trên | DELETE toàn bộ (MQTT 221 với `cmd_id`=`slot`=255, hoặc khi reset nhà máy) |
| 4 | `{"cmd":4,"data":[4]}` | như trên | STOP (MQTT 222, hoặc sau 120 s học mà không có kết quả) |
| 4 | `{"cmd":4,"data":[6,<cmd_id>]}` | như trên | QUERY bitmap slot của `cmd_id` (MQTT 223, hoặc trước khi học ngữ cảnh) |
| 4 | `{"cmd":4,"data":[6,255]}` | như trên | QUERY sức chứa (MQTT 223 với `cmd_id` 255) |
| 15 | `{"cmd":15,"data":[<cmd_id>,<respond_id>]}` | `aiio_ci13162_uart_send_resp_set()` | RESP_SET. **Chỉ dùng cho MQTT 227 sửa câu phản hồi** (`cmd_id` 56..70 đã kích hoạt). Chờ chip trả `cmd:15` tối đa 2 s |
| 8 | `{"cmd":8,"data":[1..7]}` | `aiio_ci13162_uart_send_volume()` | MQTT 225 (đặt âm lượng) và nút nhấn tăng/giảm âm lượng. `aiio_ci13162_uart_send_volume()` chặn giá trị ngoài `1..7`, không gửi |
| 8 | `{"cmd":8,"data":[8]}` | `aiio_ci13162_uart_send_mute()` | Nút giảm âm lượng gửi khi đang tắt tiếng (giữ tắt tiếng) |
| 11 | `{"cmd":11,"data":[]}` | `aiio_ci13162_uart_query_volume()` | MQTT 226 (hỏi âm lượng), và nút nhấn âm lượng khi WB2 chưa biết mức / tắt tiếng |
| 17 | `{"cmd":17,"data":[1..4]}` | `aiio_ci13162_uart_send_voice_set()` | MQTT 230 (đặt giọng) và nút 2 nhấn nhanh (giọng kế tiếp). Chờ chip trả `cmd:17` tối đa 2 s. `voice_id` ngoài `1..4` bị chặn, không gửi |
| 18 | `{"cmd":18,"data":[]}` | `aiio_ci13162_uart_query_voice()` | MQTT 231 (hỏi giọng), nút 2 nhấn nhanh khi chưa biết giọng, và lần hỏi lúc boot. MQTT 231 chờ chip trả `cmd:18` tối đa 2 s. Lần hỏi lúc boot không có thời gian chờ riêng cho từng yêu cầu: chỉ chạy sau khi nhận version, còn cache giọng bằng 0 thì thử lại tối đa 3 lần, cách 1 s; tạm dừng khi đang có thao tác CWSL hoặc OTA (giống dòng cmd 3) |
| 200 | `{"cmd":200,"data":[0]}` | `aiio_ci13162_uart_send_buzz_type(0)` | Beep ngắn (`hw_buzz_set_type()` trên thiết bị có chip giọng nói) |
| 200 | `{"cmd":200,"data":[1]}` | `aiio_ci13162_uart_send_buzz_type(1)` | Beep dài, hoặc nâng beep ngắn đang kêu thành beep dài |

Mọi lệnh gửi đều bị chặn (trả `-1`) khi OTA đang giữ UART1. Các lệnh TX đi qua một mutex chung, chờ tối đa 50 ms.

Ràng buộc phía WB2 trước khi gửi `cmd:4` (`app_cwsl_parse_mutating()`, `app_cwsl_parse_query()`):

- `cmd_id` hợp lệ là `1..70`, hoặc `255` (không dùng cho LEARN).
- Bị từ chối: `cmd_id` `2..6`; `54`, `55` (tắt/bật tiếng); `56..70` qua MQTT 220/221 (ngữ cảnh phải đi qua MQTT 227/228).
- Slot: wake (`cmd_id` 1) là `0..4`; lệnh khác là `0..14`; `255` nghĩa là tự chọn.
- `VOICEBOX`: MQTT 220..223 chỉ nhận `cmd_id` 1 hoặc 255.
- Chỉ chạy một thao tác CWSL tại một thời điểm. Đang chờ phản hồi âm lượng, đang xử lý ngữ cảnh, hoặc MQTT 230/231 đang chờ / còn trong khoảng chờ frame muộn thì trả `BUSY` (67); đang OTA thì trả `OTA_HOLD` (69).

## 4. CI13162 → WB2

| `cmd` | `data` hợp lệ | `cmd_id` bắt buộc | Xử lý trên WB2 |
|---:|---|---|---|
| 0 | `[]` | — | **Reset chính WB2** (`bl_sys_reset_por()`) |
| 1 TURN | `[1]` / `[0]` | `[1]`→7/8, `[0]`→9/10 | Chỉ báo MQTT 224 (nhánh quạt không còn áp dụng) |
| 2 MODE | `[1..6]` | 1→11/12, 2→13/14, 3→15/16, 4→17/18, 5→19..21, 6→22..24 | Chỉ báo MQTT 224 (nhánh quạt không còn áp dụng) |
| 3 VERSION | `[<uint32 > 0>]` | — | Lưu cache (`aiio_ci13162_uart_get_voice_version()`) |
| 5 CWSL_STATUS | `[code, cmd_id, slot]` | — | `app_cwsl_on_status()` |
| 6 CWSL_INFO | `[cmd_id(1..70), bm_lo, bm_hi]` hoặc `[255, wake, ncmd, nfree]` | — | `app_cwsl_on_info()` → MQTT 223 |
| 7 TIMER | `[0..12]` | `[0]`→47; `[n]`→`23+2(n-1)` hoặc `+1` | Chỉ báo MQTT 224 (nhánh quạt không còn áp dụng) |
| 9 VOLUME_INFO | `[level(0..7), result(0/1), muted(0/1)]` (đúng 3 phần tử; khác đi là `INVALID_DATA` → MQTT 299) | — | Lưu cache `level` và `muted`, gọi callback `app_volume` |
| 10 ASR_UNHANDLED | `[cmd_id]` | — | Chỉ ghi log |
| 12 SCENE | `[1..15]` (ngữ cảnh) hoặc `[71..103]` (lệnh nhà) | Ngữ cảnh: `55 + data` (56..70). Lệnh nhà: `= data` (71..103) | `app_cwsl_notify_voice_cmd()`: nháy LED 200 ms, BLE ADV, MQTT 224. Ngữ cảnh chưa được cấu hình thì bỏ hẳn (không LED, BLE, 224); lệnh nhà luôn xử lý. Cặp `data`/`cmd_id` khác → `INVALID_DATA` (MQTT 299) |
| 13 VOLUME_CHANGE | `[0..7]` | 48..55. 54 → `[0]`; 50/51 → `[1]`; 52/53 → `[7]`; 48/49/55 → `[1..7]` | Lưu cache, báo MQTT 224, `app_volume_report_voice()` |
| 14 STATE | `[0]` | — | Nhận rồi bỏ |
| 15 RESP_SET | `[0]` / `[1]` (đúng 1 phần tử, không có `cmd_id`/`cwsl`) | — | `app_cwsl_on_resp_set()` (trong `app_cwsl.c`). `[1]` → MQTT 227 thành công, `[0]` → 227 code 64. Nhận khi không có lệnh sửa nào đang chờ thì bỏ qua |
| 17 VOICE_SET | `[0]` / `[1]` | — | Cả `[0]` và `[1]` đều chuyển cho `app_voice`. `[1]` chỉ cập nhật cache `voice_id` khi WB2 thật sự đã gửi một giá trị; `[0]` → MQTT 230 code 64 |
| 18 VOICE_GET | `[1..4]` | — | Lưu cache (`aiio_ci13162_uart_get_voice_id()`) và báo `app_voice` (MQTT 231) |
| 200 BUZZ | `[]`, `[0]`, `[1]` | — | Nhận rồi bỏ |
| 4, 8, 11, 16, khác | — | — | `UNKNOWN_COMMAND` → MQTT 299 |

### Nút nhấn VOICEBOX

Ba nút trên VOICEBOX (nhấn = mức 0, mức nghỉ = 1, pull-up trong). Nhấn không beep. Xử lý trong `application/app_button.c`, chân khai báo ở `hardware/hardware.h`.

| Nút | Chân | Thao tác | Hành động | Báo App |
|---|---|---|---|---|
| 1 | IO21 | Nhấn | Tăng âm lượng 1 mức (1..7); đang tắt tiếng thì `cmd 8 [1]` | 225 (`act_id` 1) |
| 2 | IO20 | Nhấn nhanh rồi thả | Đổi sang giọng kế tiếp (`v % 4 + 1`) qua `cmd 17` | 230 (`act_id` 1) |
| 2 | IO20 | Giữ 5 s, thả tay | Đèn smart config khi đủ 5 s; thả tay thì vào smart config | Không |
| 2 | IO20 | Giữ 10 s, thả tay | Đèn AP config khi đủ 10 s; thả tay thì vào AP config | Không |
| 3 | IO0 | Nhấn | Giảm âm lượng 1 mức (1..7); đang tắt tiếng thì `cmd 8 [8]` (giữ tắt tiếng) | 225 (`act_id` 1) |

Chưa biết mức / tắt tiếng thì hỏi chip trước (`cmd 11`), chưa biết giọng thì hỏi `cmd 18`, có trả lời rồi mới gửi lệnh đặt. Nút 1 và 3 giữ lâu không làm gì. Đang bận (có 220..231, lần nhấn trước còn chờ chip, hoặc OTA) thì bỏ qua lần nhấn và gửi `cmd 200 [1]` (beep dài); riêng khi đang học lệnh (`app_cwsl_is_learning()`) thì im lặng.

Frame thoại chỉ được xử lý trên `VOICEBOX` (chỉ thiết bị này dùng chip giọng nói; `fanproacv2` không còn). Nhánh điều khiển quạt cho cmd 1/2/7 vẫn còn trong code nhưng không áp dụng cho thiết bị nào trong firmware này.

MQTT 224 do `app_cwsl_notify_voice_cmd(cmd_id, cwsl)` gửi. Hàm này bỏ qua `cmd_id` bằng 0, bỏ `cmd_id` lớn hơn 70 trừ dải lệnh nhà `71..103`, và gộp `cmd_id` `1..6` thành `1`. Ngữ cảnh `56..70` chưa được cấu hình thì dừng luôn, không LED, BLE hay 224. Các `cmd_id` còn lại ≥ 7 trên VOICEBOX được nháy LED 200 ms và phát BLE ADV (`m_ble_send_voicebox_state()`) trước khi gửi 224.

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

Timeout phía WB2: thao tác học/xóa chờ 120 s (hết hạn thì gửi STOP), query chờ 2 s, âm lượng chờ 2 s, giọng (MQTT 230/231) chờ chip 2 s và khoảng chờ frame muộn 2 s.

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
| `CMD_CWSL_INFO` (6) | `cmd_id` theo `cmd_info` (tới 103) | `data[0]` chỉ nhận `1..70` hoặc `255` | Không hỏi được slot của lệnh nhà mới |
| Lệnh quạt 1, 2, 7, 14 | Đã bỏ | Vẫn parse; chỉ báo MQTT 224, nhánh quạt không áp dụng | Code thừa, vô hại |
| `CMD_VOLUME` (8) | `[8]` tắt tiếng, `[9]` bật tiếng | Gửi `1..7`; `[8]` chỉ do nút giảm gửi khi đang tắt tiếng; không gửi `[9]` | WB2 không bật tiếng bằng `[9]`; bật lại bằng nút tăng (`[1]`) |
| `CMD_VOLUME_INFO` (9) | `[level, result, muted]` | Chỉ nhận đúng 3 phần tử, nhớ `muted` | Không có |
| `CMD_CWSL` LEARN | Có dạng 4 phần tử, kèm `value` câu trả lời | MQTT 220 và 227 đều gửi 4 phần tử (kèm `respond_id`) | Không có |
| `CMD_RESP_SET` (15) | Có | **Đã cài**, chỉ cho MQTT 227 sửa câu phản hồi | Không có đường khác đặt câu phản hồi |
| `CMD_RESP_GET` (16) | Có | Chưa cài | Chip trả về thì WB2 báo `UNKNOWN_COMMAND` (MQTT 299) |
| `CMD_VOICE_SET/GET` (17/18) | Có | **Đã cài**, cho MQTT 230/231 và lần hỏi lúc boot | Không có |
| 201..204 (bản debug của chip) | Có | Chưa cài | Nếu chip chạy bản debug, frame 201 bị báo MQTT 299. Frame 202 có key `log` cũng bị từ chối, hoặc bị bỏ vì dài quá 63 ký tự |
| `CMD_RESET` (0) chiều chip → MCU | Không mô tả | WB2 tự reset khi nhận | Cần xác nhận chip có gửi frame này không |
| `CMD_BEEP` (200) chiều chip → MCU | Một chiều | WB2 nhận rồi bỏ | Không ảnh hưởng |
