# Thực hiện plan MQTT 232 set respond_id bằng subagent

## Hỏi

Người dùng duyệt plan `002_plan-232-set-respond-id.md`: "triển khai đi". Đã chọn chạy bằng subagent từ trước.

## Branch

- /home/taodc/wb2/rada/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Quyết định (ruling khi chạy)

- **Format code test:** cũng áp quy định định dạng C (80 cột, `{` xuống dòng) cho các dòng test mới, vì CLAUDE.md nói áp dụng cho code mới kể cả trong file còn kiểu cũ.
  - Nếu sai: hai file runner có thêm thay đổi chỉ về format.
- **Log "busy, dropping late cmd 15 reply of last edit":** giữ nguyên. Log này đã commit cùng phần log 227, và vẫn đúng nghĩa cho cả 232.
- **Phần parse 227 và 232 đang lặp nhau (khoảng 80 dòng):** không gộp thành hàm chung, vì nằm ngoài plan.
- **Khoảng 2 s bỏ frame muộn sau timeout cmd 15:**
  - Khoảng này không tính là bận với các lệnh khác; chỉ 232 và 227 sửa nhận 67.
  - Giữ như hành vi hiện có của 227 sửa. Frame muộn vẫn bị bỏ.
- **`APP_SCENE_CMD_RESP_SET` trong `app_scene.h` lệch cột 1 khoảng trắng** so với nhóm 227/228/229. Để nguyên, vì quy trình không cho lượt sửa thứ hai và lỗi này chỉ là thẩm mỹ.
- **Workspace `.superpowers/sdd/002_plan-232-set-respond-id/` được giữ lại,** vì code chưa commit.

## Đã làm

- **Điều kiện bắt đầu:** người dùng đã commit phần log 227 (e7a67c33).
- **Tạo lại công cụ snapshot** cây làm việc thành commit tạm, bỏ qua `conversation/` và `.superpowers/`, không đụng branch hay index.
- **Task 1 (`app_cwsl.c/.h`, `test_app_cwsl_runner.c`):**
  - thêm `app_cwsl_start_resp_set()`: kiểm OTA/bận, gửi cmd 15, bắt đầu chờ;
  - `app_cwsl_start_scene_resp_set()` chỉ còn kiểm dải 56..70 rồi gọi hàm chung;
  - sửa Test 6 theo luật `cmd_id` ≤ 103 (case 220 và 221 với `cmd_id` 7, thêm case 104);
  - mock OTA bật/tắt được; thêm Test 14.
  - Review: Approved.
- **Task 2 (`app_scene.c/.h`, `process_data.c`, `test_app_scene_runner.c`, `docs/voice/uart_wb2_ci13162.md`):**
  - handler `app_scene_resp_set()`: parse, kiểm theo đúng thứ tự đã chốt, log `[ERROR]`, publish 232;
  - `app_scene_on_resp_set_result()` trả 227 hay 232 theo `s_app_scene_pending_mqtt`;
  - đăng ký `{app_scene_resp_set, 232}`;
  - thêm Test 18 (a)–(i); sửa 7 chỗ trong tài liệu UART.
  - Review: Approved.
- **Review toàn bộ (Opus):** "With fixes". Không có lỗi Critical/Important, chỉ có 5 Minor.
- **Lượt sửa cuối:**
  - ngắt dòng trong `app_scene.h`, thêm Doxygen Request/Reply;
  - thêm case (j): 232 bị từ chối không làm hỏng 227 sửa đang chờ;
  - sửa comment/tài liệu còn ghi "chỉ 227";
  - ngắt dòng test cho đủ 80 cột.
  - Re-review: các mục đều đã xử lý, trừ một `#define` lệch cột.
- **Xác minh cuối (Claude tự chạy lại):**
  - runner: uart 21, cwsl 14 (`validation total=61 failed=0`), scene 18, voicebox 10, voice 46;
  - dòng code mới không vượt 80 cột;
  - `make` OK, không có warning ở `app_scene`/`app_cwsl`; `switchv3.bin` 935.108 B.

## Không làm

- Không đổi 231 (get voice_id).
- Không thêm đọc respond_id (cmd 16), không cho nhiều cặp trong một request.
- Không sửa `hnn_voiceboxv1.xlsx` và `CWSL_MQTT_SPEC.md`.
- Không mở rộng bộ validation `BUILD_TEST` trong `app_scene.c`.

## Chưa kiểm

- Chưa thử trên board: làm theo mục "Kiểm trên board" cuối plan.
- Chưa commit.
