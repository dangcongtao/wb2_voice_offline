# So sánh cmd4 (WB2 → chip) giữa code và commands.md

## Hỏi

cmd4 (WB2 gửi xuống chip) trong code và trong `docs/voice/commands.md` có gì khác nhau không.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Đã làm

So `commands.md` mục 4.1, 4.2, 4.5, 4.6 với code:
- `aiio_ci13162_uart_send_cwsl()` (`hardware/aiio_ci13162_uart.c:2183`);
- mọi chỗ gọi hàm này trong `application/app_cwsl.c`;
- các hàm parse MQTT: `app_cwsl_parse_mutating`, `app_cwsl_parse_query`.

**Giống nhau**
- Dạng frame `{"cmd":4,"data":[...]}\r\n`.
- 4 action: 1 LEARN, 2 DELETE, 4 STOP, 6 QUERY.
- Slot: wake `0..4`, command `0..14`, `255`.
- `[2,255,255]` và `[6,255]`.

**Khác nhau**
1. LEARN không gửi được phần tử thứ 4 `value`. Action 1/2 bắt buộc đúng 2 byte data.
2. Dải `cmd_id` WB2 cho gửi hẹp hơn chip:
   - Chip nhận: wake 1, âm lượng 48..55, ngữ cảnh 56..70, lệnh nhà 71..103.
   - VOICEBOX chỉ gửi:
     - 220/221/223: `cmd_id` 1, hoặc 255;
     - 227/228: `cmd_id` 56..65.
   - `APP_CWSL_CMD_ID_MAX = 65`. Parser cmd6 cũng loại `cmd_id` > 65.
3. Commands.md nói MCU "chỉ nên giữ một CWSL request đang chờ". Code cho phép QUERY 223 trong lúc LEARNING; chỉ chặn ở QUERY_OLD. Commands.md mục 4.5 nói chip vẫn trả lời QUERY khi đang học.
4. `app_cwsl_factory_clear()` (gửi `[4]` rồi `[2,255,255]`) không có ai gọi. Lệnh xóa thiết bị (123) không xóa mẫu giọng trên chip.

## Không làm

- Không sửa code hay tài liệu.

## Chưa kiểm

- Chưa kiểm trên board phản hồi của chip với frame có `value`.
