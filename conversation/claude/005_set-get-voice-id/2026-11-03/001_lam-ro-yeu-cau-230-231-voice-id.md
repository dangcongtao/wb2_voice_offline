# Làm rõ yêu cầu MQTT 230 (set voice_id) / 231 (get voice_id)

## Hỏi

- Thêm xử lý cho `{"voiceboxv1":230, "u":55519, "voice_id":1}` để đặt giọng cho thiết bị. Có 4 giọng, voice_id 1..4, như trong `docs/voice/response_text.txt`.
- Phía CI13162 đã có lệnh UART set/get giọng chưa?
- Get giọng: `{"voiceboxv1":231, "u":55519}`.
- Lên plan cho set/get voice_id.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định (người dùng chốt qua câu hỏi)

- **Lệnh UART phía chip** (`docs/voice/commands.md` mục 3.2) — đây là cái có sẵn, không phải quyết định mới:
  - set: `{"cmd":17,"data":[v]}`, v 1..4; chip trả `[1]` (đã đổi) hoặc `[0]` (từ chối);
  - get: `{"cmd":18,"data":[]}`; chip trả `[v]`.
- **act_id bắt buộc**, giống 225: thiếu hoặc sai thì trả lỗi. Bản tin thực tế:
  - 230: `{"voiceboxv1":230,"u":..,"voice_id":1..4,"act_id":..}`
  - 231: `{"voiceboxv1":231,"u":..,"act_id":..}`
- **Bản tin lỗi** có `"result":0,"code":N`, giống 227.
- **Bảng mã** (người dùng đồng ý):
  - 64: thiếu/sai `u`, `act_id`, `voice_id` (ngoài 1..4), hoặc chip trả `[0]`. WB2 chặn voice_id sai, không gửi xuống chip;
  - 67: đang bận (học lệnh, ngữ cảnh, âm lượng, hoặc 230/231 khác);
  - 69: đang OTA;
  - 70: gửi UART lỗi;
  - 52: chip không trả lời trong 2 s.
- **voice_id trong bản tin lỗi:** 230 trả lại số App gửi; 231 không có key `voice_id`.
- **Thời gian chờ** chip trả cmd 17/18: 2 s, như âm lượng.
- **Chặn hai chiều**, như 225/226:
  - 230/231 bị từ chối khi đang học lệnh/ngữ cảnh, đang đặt/đọc âm lượng, hoặc đang OTA;
  - trong lúc 230/231 chờ chip, các lệnh UART khác và OTA cũng bị từ chối.
- **231 hỏi chip mỗi lần** bằng cmd 18, giống 226.
- **Bản tin trạng thái 100 và 105 thêm `"voice_id"`.** Chưa biết giọng thì trả 0.
- **Hỏi giọng lúc khởi động:**
  - sau khi nhận được version của chip, WB2 gửi cmd 18;
  - không có trả lời thì thử lại sau 1 s, tối đa 3 lần;
  - chip không trả version thì không hỏi, voice_id giữ 0.
- **Giá trị voice_id WB2 nhớ** được cập nhật từ:
  - mọi frame cmd 18 hợp lệ, kể cả frame đến muộn;
  - cmd 17 `[1]` của lệnh 230, kể cả khi chip trả muộn sau lúc App đã nhận 52.

## Đã làm

- Đọc `commands.md` mục 3.2, `response_text.txt`, `hnn_voiceboxv1.xlsx`.
  - xlsx mục 17 có 230, response mẫu `{"voiceboxv1":230,"u":55519,"voice_id":1,"result":1}`;
  - xlsx chưa có 231, và mục 17 không có `act_id`.
- Đọc code mẫu:
  - `app_volume.c`: 225/226 có pending, timeout, khoảng chờ frame muộn, chặn với CWSL và OTA;
  - các điểm chặn bận trong `app_cwsl.c` và `process_data.c` (129, 134);
  - parser, dispatch, hỏi version trong `aiio_ci13162_uart.c`.

## Không làm

- Không sửa `hnn_voiceboxv1.xlsx`, vì người dùng tự quản file này. Cần bổ sung `act_id` vào mục 17 và thêm 231.

## Chưa kiểm

- Chip trả `{"cmd":17,"data":[1]}` trước hay sau khi phát xong welcome. `commands.md` không ghi thời gian. Nếu chip trả sau hơn 2 s, App nhận 52 dù giọng đã đổi.
