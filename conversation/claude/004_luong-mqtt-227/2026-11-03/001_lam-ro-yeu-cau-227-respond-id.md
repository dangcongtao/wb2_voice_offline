# Làm rõ yêu cầu: 227 format mới (cmd_id 255 thêm / khác sửa, respond_id)

## Hỏi

Người dùng sửa lại lệnh 227:

```
{"voiceboxv1":227, "u":55519, "cmd_id":255, "act_id":0, "respond_id":1}
```

- `cmd_id`: 255 là thêm mới, giá trị khác là sửa.
- `respond_id`: id câu phản hồi muốn gán cho `cmd_id`.

Yêu cầu: lên plan. Không đoán; khi cần xác nhận thì hỏi lại.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định (người dùng trả lời qua bảng hỏi)

1. **Sửa** (`cmd_id` 56..70) **chỉ đổi `respond_id`**, không học lại giọng.
   - WB2 gửi UART `{"cmd":15,"data":[<cmd_id>,<respond_id>]}`.
   - Chờ chip trả `{"cmd":15,"data":[1]}` (đã áp) hoặc `[0]` (từ chối).
2. `respond_id` **bắt buộc**. Thiếu thì trả 64.
3. **Bỏ key `value`**, dùng `respond_id`.
4. Trả **65** khi sửa ngữ cảnh chưa kích hoạt, hoặc `cmd_id` ngoài 56..70 (và khác 255).
5. Bản tin 227 trả về, cả thành công lẫn lỗi, **kèm `cmd_id` và `respond_id`**:
   - Thành công: `{"voiceboxv1":227,"cmd_id":56,"respond_id":47,"u":55519,"act_id":0,"result":1}`
   - Lỗi: `{"voiceboxv1":227,"cmd_id":56,"respond_id":47,"u":55519,"act_id":0,"result":0,"code":64}`
   - `cmd_id` trong bản tin là số trong request. Khi thêm mới mà chưa cấp được thì là 255. Đã cấp thì là số đã cấp.
   - Thiếu hoặc sai kiểu thì ghi 0.
6. Mã lỗi khi sửa:
   - chip trả `[0]` → 64;
   - chip không trả lời trong 2 s → 52;
   - gửi UART lỗi → 70.
7. Sửa khi đang bận xử lý như thêm mới:
   - bận → 67, đang OTA → 69, không gửi xuống chip;
   - trong lúc chờ chip trả lời lệnh sửa, các lệnh 220..223/225/226/227/228 khác cũng nhận 67.
8. Danh sách `respond_id` hợp lệ giữ như key `value` trước đây: `0, 1, 2, 3, 11..19, 40..47, 50..68, 70, 71`. Sai thì WB2 chặn trước, trả 64. Áp dụng cho cả thêm mới và sửa.
9. Thiếu key `cmd_id` thì trả 64.

## Đã làm

- Hỏi 3 lượt (10 câu), ghi lại các câu trả lời ở trên.
- Đọc code hiện tại:
  - `app_scene_add` đang có phần người dùng sửa dở (08:03 ngày 2026-11-03);
  - `system_typedef.h` đã có thêm `APP_SCENE_KEY_RESPOND_ID "respond_id"` và `APP_SCENE_KEY_CMD_ID "cmd_id"`;
  - WB2 chưa hỗ trợ UART cmd 15.
- Phần sửa dở có 2 lỗi:
  - truyền `uint8_t *` vào `app_scene_read_number`, trong khi hàm này ghi `uint32_t`, nên ghi đè stack;
  - `cmd_id` bị kiểm bằng tập `respond_id`.
- `test_app_scene_runner.c` định nghĩa `CJSON_KEYWORD_H__` nên bỏ qua `system_typedef.h`, vì vậy runner này hiện không build được.
- Máy đã khởi động lại lúc 07:36. Scratchpad cũ mất, gồm cả script snapshot cho SDD.

## Chưa kiểm

- Chưa sửa code. Chưa build.
