# Làm rõ yêu cầu: MQTT set respond_id cho mọi cmd_id

## Hỏi

- Người dùng yêu cầu: "thêm cmd 231: `{"voiceboxv1":231, "u":55519, cmd_id":48, "respond_id":1, "act_id":0}`".
- Mục tiêu: set respond_id cho bất kỳ cmd_id nào (1 → 103).

## Branch

- /home/taodc/wb2/rada/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Quyết định (người dùng chốt)

- **Số lệnh:** dùng **232**.
  - 231 đang là get voice_id (tính năng 005) và giữ nguyên.
  - Request: `{"voiceboxv1":232,"u":55519,"cmd_id":48,"respond_id":1,"act_id":0}`.
- **Field bắt buộc:** `u`, `cmd_id`, `respond_id`, `act_id` (0..255), giống 227/230. Thiếu hoặc sai kiểu thì trả 64.
  - Có field `action` cũng trả 64, giống 227 (Claude đề xuất, người dùng duyệt).
- **`cmd_id`:** chỉ nhận id có trên chip, theo model 62 lệnh.
  - Nhận 1..6 (wake), 48..55 (âm lượng), 71..103 (lệnh nhà).
  - 56..70 (ngữ cảnh) chỉ nhận khi đã kích hoạt, giống 227 sửa. Chưa kích hoạt thì trả 65.
  - 0, 7..47, >103 trả 65, không gửi UART.
- **`respond_id`:** dùng chung tập lọc của 227: 0..3, 11..19, 40..47, 50..68, 70, 71. Ngoài tập này trả 64.
- **Cách chạy:**
  - WB2 gửi `{"cmd":15,"data":[cmd_id,respond_id]}` (1 cặp), dùng chung cơ chế chờ chip với 227 sửa.
  - Chờ chip tối đa 2 s:
    - chip trả `[1]`: `result:1`;
    - chip trả `[0]`: code 64;
    - không trả lời: code 52. Sau đó 2 s tiếp theo bỏ frame cmd 15 đến muộn.
- **Bận, chặn hai chiều như 227 sửa:**
  - 232 trả 67 khi đang có 220..223, 227, 228, 225, 230, lần nhấn nút còn chờ chip, hoặc đang bỏ frame cmd 15 đến muộn.
  - 232 trả 69 khi đang OTA.
  - Trong lúc 232 chờ chip:
    - 220..223, 227, 228, 230, 231 nhận 67; 225/226 nhận `result:0`;
    - nút nhấn beep dài;
    - OTA 129/134 bị chặn.
- **Bản tin trả lời App:**
  - Thành công: `{"voiceboxv1":232,"u":55519,"cmd_id":48,"respond_id":1,"act_id":0,"result":1}`.
  - Lỗi: cùng các field, thay `"result":1` bằng `"result":0,"code":N`. Vẫn echo `cmd_id`/`respond_id` App gửi; field đọc không được thì là 0.
  - Mã lỗi:
    - 64: request sai, hoặc chip trả `[0]`;
    - 65: `cmd_id` sai hoặc ngữ cảnh chưa kích hoạt;
    - 67: bận;
    - 69: OTA;
    - 70: gửi UART lỗi;
    - 52: chip không trả lời trong 2 s.
- **Log:** `[ERROR]: <nguyên nhân>, in[<hàm>]` ở từng chỗ lỗi, như 220/227 (Claude đề xuất, người dùng duyệt).
- **Tài liệu:** sửa `docs/voice/uart_wb2_ci13162.md`, vì tài liệu đang ghi cmd 15 "chỉ dùng cho 227" (Claude đề xuất, người dùng duyệt).
- **Test cũ `test_app_cwsl_runner.c` Test 6:**
  - Hai case VOICEBOX 220/221 với `cmd_id` 7 đang đòi bị từ chối 65. Sau commit 6168b039, `cmd_id` 7 được nhận.
  - Sửa hai case đó theo luật mới: 7 được nhận và gửi UART. Thêm case `cmd_id` 104 bị từ chối 65.
- **Thứ tự làm:**
  - Người dùng commit phần log lỗi 227 (việc 008/003) trước, rồi mới sửa code cho 232.
  - Viết plan, người dùng duyệt, rồi chạy bằng subagent.
- **Repo:** chỉ `rada/ai-wb2-2026`.

## Đã làm

Chỉ đọc code và tài liệu:
- `docs/voice/commands.md`, mục 3:
  - cmd 15 set được câu trả lời cho mọi lệnh, value 0 = ngẫu nhiên, 1 = mặc định, 2..99 = response id;
  - tối đa 12 cặp; sai một cặp thì trả `[0]`;
  - chip lưu NV.
  - Mục 2.1: model có 62 lệnh. Các `cmd_id` 7..47 không tồn tại.
- `app_cwsl.c`:
  - cmd 15 hiện chỉ đi qua `app_cwsl_start_scene_resp_set` (56..70), state `s_app_cwsl_resp_set`, kết quả đi vào `app_scene_on_resp_set_result`;
  - `app_cwsl_is_pending()` có tính `resp_set.active`, nên volume/voice/nút/OTA tự bị chặn.
- `app_scene.c`:
  - `app_scene_respond_id_is_valid` (static) chứa tập lọc;
  - 229 không trả respond_id, nên WB2 không cần lưu.
- `process_data.c`: 230/231 đã dùng; 232 còn trống.

## Không làm

- Không đổi 231.
- Không cho nhiều cặp `cmd_id`/`respond_id` trong một request.
- Không thêm lệnh MQTT đọc respond_id (cmd 16).
- WB2 không lưu respond_id.
- Không đụng `ai-wb2-baremetal`.

## Chưa kiểm

- Chưa viết code; chưa có plan.
- Chưa kiểm tra chip trả gì cho `cmd_id` không tồn tại. WB2 chặn trước nên không gửi xuống chip.
