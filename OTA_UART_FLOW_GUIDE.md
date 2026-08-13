# Hướng dẫn OTA qua UART cho CI13LC / CI13162

Tài liệu này mô tả luồng cập nhật firmware qua UART dành cho hai đối tượng:

- Người phát triển **host OTA** (ví dụ MCU Wi-Fi) cần điều khiển việc cập nhật một CI13162.
- Người vận hành cần nạp một file firmware từ máy tính bằng Python updater.

Mục tiêu là làm rõ các trạng thái, các baud rate, frame UART và các điều kiện để chuyển từ firmware đang chạy sang bộ nạp trong MaskROM. OTA ở đây là cập nhật firmware qua UART; nó không phải giao thức UART ứng dụng thông thường của sản phẩm.

> **Cảnh báo:** Các pha ERASE và WRITE làm thay đổi flash. Chỉ gửi chúng khi đã kiểm tra đúng chip, đúng firmware container và nguồn cấp ổn định. Một update bị ngắt giữa chừng có thể khiến sản phẩm không khởi động được cho đến khi nạp lại hoàn chỉnh.

## 1. Mô hình tổng quát

```text
Host Wi-Fi / PC
      │ UART0
      ▼
CI13162 application ──(yêu cầu reset)──► MaskROM / boot stage
                                             │
                                             │ handshake @115200
                                             ▼
                                      tải updater vào SRAM @460800
                                             │
                                             ▼
                                      updater ghi flash @921600
```

Firmware ứng dụng bình thường không tự hiểu frame OTA `A1/83`. Chip chỉ nhận handshake OTA trong boot stage sau reset. Vì vậy host luôn phải thực hiện theo thứ tự:

1. Bắt đầu phát handshake ở 115200 baud.
2. Reset chip để nó đi vào boot stage trong khi handshake vẫn đang được phát.
3. Chỉ tiếp tục khi nhận ACK handshake.

Nếu chỉ phát handshake khi firmware ứng dụng đang chạy mà không reset, không nên kỳ vọng chip sẽ vào OTA trừ khi ứng dụng đã tự cài một cơ chế chuyển OTA riêng.

## 2. Điều kiện phần cứng và reset

### 2.1 UART0 và PG_EN (PA4)

CI13162 dùng PA4 (`PG_EN`) làm chân strap khi cấp nguồn/reset. PA4 có pull-up nội; boot stage chỉ vào luồng update khi điều kiện strap và tín hiệu upgrade trên UART0 phù hợp. Board có thể thêm pull-down PA4 để ưu tiên boot nhanh; khi đó cần xác minh riêng cách đưa chip vào update mode.

Sau boot, firmware có thể tái sử dụng PA4 như GPIO. Điều đó không thay đổi vai trò strap của nó tại thời điểm reset. Người thiết kế host OTA cần kiểm tra schematic thực tế để bảo đảm PA4 không bị giữ ở mức khiến boot stage bỏ qua update.

### 2.2 Ba cách reset

| Cách | Độ tin cậy | Ghi chú |
|---|---:|---|
| Kéo chân reset hoặc power-cycle | Cao nhất | Host phát handshake trước, sau đó kéo reset. Phù hợp khi MCU Wi-Fi điều khiển transistor/reset line. |
| Lệnh reset do firmware ứng dụng tự định nghĩa | Cao | Ứng dụng nhận một lệnh UART riêng, ACK ngay, rồi gọi reset hệ thống. Đây là cách khuyến nghị khi không có reset GPIO từ host. |
| Một frame reset OTA của SDK/dòng chip khác | Chưa xác nhận chung | Không coi là portable giữa các chip/bootloader. Chỉ dùng sau khi đã bắt được ACK và thử nghiệm trên đúng phần cứng. |

Với software reset, host gửi lệnh reset **của ứng dụng**, rồi lập tức spam handshake. Không chờ log boot hay chờ ứng dụng gửi lại một lệnh khác.

## 3. Khung frame OTA

Mọi frame OTA dùng cấu trúc dưới đây:

```text
A5 0F | length[2] | type | command | sequence | payload[length] | crc16[2] | FF
```

| Trường | Mô tả |
|---|---|
| `A5 0F` | Header, theo thứ tự byte trên UART. |
| `length[2]` | Độ dài payload, little-endian. |
| `type` | `A0` command, `A1` request, `A2` ACK/response, `A3` notify/progress. |
| `command` | Mã lệnh của pha hiện tại. |
| `sequence` | Số thứ tự; các capture tham chiếu dùng `00`. |
| `crc16[2]` | CRC16-CCITT, poly `0x1021`, init `0x0000`, gửi little-endian. |
| `FF` | Tail. |

Frame CRC chỉ được tính trên:

```text
type | command | sequence | payload
```

Nó không bao gồm header, trường length, CRC hay tail.

## 4. Handshake tại 115200 baud

Host bắt đầu ở **115200, 8N1** và gửi lặp frame sau mỗi khoảng 20–35 ms:

```text
A5 0F 00 00 A1 83 00 67 C4 FF
```

Ý nghĩa: `A1/83` là request handshake của CI13LC/CI13162. Khi boot stage bắt được frame, chip trả ACK `A2/83`, ví dụ:

```text
A5 0F 00 00 A2 83 00 37 9D FF
```

Host phải đặt timeout hữu hạn, ví dụ 30 giây. Không có ACK nghĩa là ít nhất một trong các điều kiện sau chưa đúng:

- Chip chưa reset vào boot stage.
- UART0 RX/TX hoặc GND nối sai.
- PA4/PG_EN strap không cho phép update ở boot.
- Baud hoặc frame CRC sai.
- Host đang nói với firmware ứng dụng thay vì boot stage.

Sau ACK, host phát các command điều khiển ở 115200 và chờ `A2/<command>` cho từng command. Không đổi baud local trước khi ACK lệnh đổi baud đã về.

## 5. Bản đồ baud và state machine

```text
S0  APP / idle
 │  host spam A1/83 @115200, sau đó reset chip
 ▼
S1  MaskROM control @115200
 │  CHANGE_BAUD(460800), nhận ACK ở 115200
 ▼
S2  Tải agent vào SRAM @460800
 │  verify/jump agent
 ▼
S3  Agent control @115200
 │  GET_INFO, CHANGE_BAUD(921600), nhận ACK ở 115200
 ▼
S4  Agent erase/write/verify flash @921600
 │  COMPLETE
 ▼
S5  Reset hoặc boot firmware mới
```

| Pha | Baud | Chủ thể chạy trên chip | Công việc |
|---|---:|---|---|
| S1 | 115200 | MaskROM / boot stage | Handshake và điều khiển khởi đầu. |
| S2 | 460800 | MaskROM / boot stage | Nhận updater agent vào SRAM. |
| S3 | 115200 | Updater agent | Kiểm tra sẵn sàng, chọn format firmware, đọc partition table. |
| S4 | 921600 | Updater agent | Erase, ghi, verify các block flash. |

Điểm dễ nhầm: updater agent được tải ở **460800**, nhưng sau khi agent chạy, control quay lại **115200** trước khi tăng tiếp lên **921600** để ghi firmware.

## 6. Cơ chế đổi baud

Lệnh đổi baud là `A0/0B`, payload là baud 32-bit little-endian.

| Mục tiêu | Payload | Frame tham chiếu |
|---:|---|---|
| 460800 | `00 08 07 00` | `A5 0F 04 00 A0 0B 00 00 08 07 00 9B 36 FF` |
| 921600 | `00 10 0E 00` | `A5 0F 04 00 A0 0B 00 00 10 0E 00 C1 66 FF` |

Quy tắc bắt buộc:

```text
1. Host gửi CHANGE_BAUD tại baud hiện tại.
2. Chip trả ACK A2/0B tại baud hiện tại.
3. Host đổi UART sang baud mới.
4. Host gửi TEST_BAUD A0/0C và chờ ACK.
```

Đổi UART host ngay sau khi gửi `A0/0B` sẽ làm mất ACK và phá vỡ session.

## 7. Tải updater agent vào SRAM

Sau khi đã chuyển sang 460800:

1. Gửi `A0/0C` (`TEST_BAUD`) và chờ ACK.
2. Gửi `A0/06` (`BLOCK_INFO`) cho agent:

   ```text
   address = 0x1FF80000
   size    = 0x00005000       # 20 KiB
   crc16   = CRC16(agent bytes)
   ```

3. Chip chủ động yêu cầu từng chunk bằng `A1/08`:

   ```text
   payload request = offset[4] | size[4]
   ```

4. Với mỗi request, host trả `A2/08`:

   ```text
   payload response = offset[4] | agent_data[size]
   ```

5. Khi chip gửi `A1/09` (`BLOCK_WRITE_DONE`), gửi `A0/8A` để verify/jump agent; chờ ACK thành công.

Agent phải được pad thành đúng `0x5000` byte bằng `0xFF` trước khi tính CRC và chia chunk. Chunk chuẩn là `0x1000` byte; agent chiếm năm chunk.

> Không stream agent hoặc firmware một chiều. Boot stage/updater là phía kéo dữ liệu: host chỉ trả dữ liệu cho từng `A1/08` mà nó yêu cầu.

## 8. Đọc thông tin firmware và partition table

Khi agent đã chạy, host đặt UART trở lại 115200 rồi gửi:

1. `A0/05` — `CHECK_READY`.
2. `A0/12`, payload `02` — chọn firmware format V2.
3. `A0/04` — `GET_INFO`.

Response `A2/04` chứa thông tin chip và partition table hiện có. Với update theo partition, host phải so sánh từng partition trong image mới với table trên chip theo các trường:

```text
version | address | size | crc
```

Chỉ partition có ít nhất một trường khác mới cần erase/write/verify. Sau đó host gửi `CHANGE_BAUD(921600)` theo quy tắc ở phần 6.

## 9. Ghi firmware ở 921600 baud

Mỗi block flash được xử lý tuần tự:

```text
A0/0C  TEST_BAUD
A0/06  BLOCK_INFO(base, size, crc-field)
A0/07  ERASE(base, size)
loop:
  A1/08 từ chip: offset, size
  A2/08 từ host: offset, data
A1/09  BLOCK_WRITE_DONE từ chip
A0/0A  VERIFY(base, size, CRC16(real block data))
```

Sau tất cả block, host gửi `A0/0E` (`COMPLETE`) và chờ ACK.

Payload trả cho `A1/08` luôn có dạng:

```text
offset[4] | data[size]
```

CRC block được tính trên dữ liệu thật của block, **không** tính bốn byte offset có trong từng frame `A2/08`.

Firmware image phải được pad `0xFF` tới bội số 4096 byte trước khi ghi. Nếu file firmware không phải container firmware format V2 có partition table, đừng đưa trực tiếp file `user_code.bin` vào updater; hãy đóng gói full firmware trước.

## 10. Pseudocode cho host Wi-Fi

```text
open_uart(115200)
start_background_handshake_loop(A1/83, period=20ms)
request_application_reset_or_toggle_reset_pin()

wait until receive A2/83 or timeout
stop_background_handshake_loop()

send_and_wait_ack(A0/85)  # readiness sequence theo boot stage
send_and_wait_ack(A0/47, payload=00)
send_and_wait_ack(A0/84)
change_baud_transaction(460800)

send_and_wait_ack(A0/0C, payload=10 00 00 00)
send_and_wait_ack(A0/06, agent_block_info)
while receive frame:
    if frame == A1/08:
        send A2/08 with requested agent bytes
    if frame == A1/09:
        break
send_and_wait_ack(A0/8A)

set_uart_baud(115200)
send_and_wait_ack(A0/05)
send_and_wait_ack(A0/12, payload=02)
partition_table = send_and_wait_ack(A0/04)
plan = compare_image_partitions(partition_table)
change_baud_transaction(921600)

for each changed flash block:
    test_baud()
    send BLOCK_INFO
    send ERASE
    answer every A1/08 request with A2/08
    wait A1/09
    send VERIFY and require success

send COMPLETE and require ACK
reset_chip_or_wait_for_normal_boot()
```

Trong implementation thật, parser UART phải chịu được frame bị chia nhỏ giữa nhiều lần đọc, nhiều frame trong một lần đọc, notify `A3/11` xen giữa và timeout. Mọi command thay đổi trạng thái chỉ được gửi sau ACK của command trước đó.

## 11. Cách dùng Python updater

Trước khi nạp, cài Python 3 và pyserial:

```powershell
python -m pip install pyserial
```

Kiểm tra image mà không mở cổng serial:

```powershell
python tool_update_diff.py --dry-run
python tool_update_all.py --dry-run
```

Nạp theo partition thay đổi, giữ dữ liệu NV:

```powershell
python tool_update_diff.py COM5 D:\firmware\Firmware_V2.0.0.bin
```

Nạp toàn bộ image nhưng giữ NV:

```powershell
python tool_update_all.py COM5 D:\firmware\Firmware_V2.0.0.bin --keep-nv
```

Nạp toàn bộ image và xóa NV:

```powershell
python tool_update_all.py COM5 D:\firmware\Firmware_V2.0.0.bin
```

Khi updater in thông báo đang spam handshake, reset hoặc power-cycle chip. Không reset trước khi chạy updater quá lâu vì cửa sổ boot có thể đã qua.

### Chọn chế độ nạp

| Lệnh | Khi dùng | NV |
|---|---|---|
| `tool_update_diff.py` | Cập nhật lặp lại, chỉ thay code/voice/model/command file. Khuyến nghị mặc định. | Giữ nguyên. |
| `tool_update_all.py --keep-nv` | Cần ghi lại toàn bộ firmware nhưng vẫn giữ cấu hình/dữ liệu người dùng. | Giữ nguyên. |
| `tool_update_all.py` | Lần nạp đầu, recovery flash bị hỏng, hoặc chủ đích xóa sạch cấu hình. | Bị xóa. |

## 12. Xử lý lỗi và checklist

| Triệu chứng | Kiểm tra trước tiên |
|---|---|
| Không có `A2/83` | Reset timing, PA4/PG_EN, UART0 TX/RX/GND, 115200 8N1, CRC frame. |
| Mất kết nối ngay sau đổi baud | Host đã đổi baud trước ACK, payload baud sai endian, hoặc UART không ổn định ở baud cao. |
| Timeout chờ `A1/08` | `BLOCK_INFO`/CRC/size/address sai, agent không được pad đúng 0x5000, hoặc không qua `TEST_BAUD`. |
| Verify fail | CRC block tính cả offset, image chưa pad 4096, mất chunk hay nguồn không ổn định. |
| Boot không lên sau update | Nạp lại full firmware, kiểm tra đúng chip/format, đảm bảo partition table không bị ghi sai. |

Khi một pha thất bại sau ERASE, cách phục hồi an toàn nhất là reset chip và chạy lại flow từ handshake; không giả định rằng host và chip còn đồng bộ state sau timeout.

## 13. Yêu cầu an toàn cho sản phẩm

- Chỉ cho phép một host tin cậy truy cập UART update; giao thức ở mức transport không tự chứng minh image là hợp lệ về nguồn gốc.
- Cấp nguồn ổn định trong suốt erase/write; không dùng reset ngẫu nhiên để hủy giữa block.
- Kiểm tra chip ID và partition table sau `GET_INFO` trước khi ghi.
- Lưu log theo transaction: baud, command, ACK, offset, size và CRC để có thể tái hiện lỗi.
- Nếu sản phẩm không có đường recovery vật lý, cần có chính sách chống cúp nguồn và recovery riêng trước khi phát hành OTA ngoài thực tế.