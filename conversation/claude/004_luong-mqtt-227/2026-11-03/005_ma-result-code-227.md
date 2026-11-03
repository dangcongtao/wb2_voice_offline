# Các result/code WB2 trả cho MQTT 227

## Hỏi

Bản tin 227 WB2 gửi lên có những `result` và `code` nào, mỗi giá trị nghĩa là gì?

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Đã làm

Đọc `app_scene.c` (`app_scene_add`, `app_scene_on_learn_result`, `app_scene_on_resp_set_result`) và `app_cwsl.c`:
- `app_cwsl_start_scene_learn`, `app_cwsl_start_scene_resp_set`;
- `app_cwsl_on_status`, `app_cwsl_on_info`, `app_cwsl_stop`, `app_cwsl_poll`, `app_cwsl_on_resp_set`.

**Kết quả:**
- `result:1`:
  - thêm mới: sau khi chip trả 32. Nếu xóa mẫu cũ thất bại thì vẫn trả 1;
  - sửa: chip trả cmd 15 `[1]`.
- Lỗi WB2 trả ngay, trước khi gửi UART:
  - 64: request sai;
  - 65: `cmd_id` ngoài 56..70 hoặc chưa kích hoạt;
  - 67: đang bận;
  - 50: đủ 15 ngữ cảnh;
  - 69: đang OTA;
  - 70: gửi UART lỗi.
- Lỗi trả sau:
  - 52: hỏi slot quá 2 s, học quá 120 s, hoặc sửa quá 2 s;
  - 34: bị hủy bằng 222;
  - 70: gửi LEARN lỗi, hoặc lưu mask vào flash lỗi;
  - mã cmd5 nhóm 3x/4x của chip được chuyển nguyên lên App;
  - sửa mà chip trả `[0]`: 64.
- Cùng một số có thể đến từ WB2 hoặc từ chip (50, 52, 64, 65, 67). App không phân biệt được nguồn.
- Các mã chip 16..18 (yêu cầu nói lại) không được gửi lên App.

## Chưa kiểm

- Không chạy gì; chỉ đọc code.
