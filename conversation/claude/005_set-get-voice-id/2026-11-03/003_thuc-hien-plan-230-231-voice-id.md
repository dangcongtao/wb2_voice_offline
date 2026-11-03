# Thực hiện plan MQTT 230/231 set/get voice_id bằng subagent

## Hỏi

- Người dùng duyệt plan `002_plan-them-230-231-voice-id.md`: "duyệt, triển khai plan".
- Người dùng không chọn cách chạy; Claude chạy bằng subagent, như cách người dùng chọn ở plan 227 trước.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định (ruling khi chạy)

- **R1:** dùng bản sao `run-tests.sh` trong workspace của plan này. Lệnh gcc giống hệt bản cũ, có thêm `test_app_voice_runner`.
- **R2:** test "không có version thì không hỏi giọng" kiểm thêm số lần TX không đổi. Assert trong plan tự pass khi buffer rỗng.
- **R3:** giữ việc cập nhật bản sao format 100 trong Test 7 của voicebox runner. Không runner nào compile `process_data.c`, nên chỗ này chỉ kiểm được bằng build và trên board.
- **R4 (cần người dùng xác nhận):** 229 không bị chặn khi 230/231 đang chạy, vì 229 chỉ đọc flash của WB2.
- **R5:** `voice_id` trong bản tin lỗi của 230 là số App gửi nếu là số nguyên 0..255; ngoài khoảng đó trả 0.
- **R6:** giữ `app_voice_read_number` là một bản riêng trong module mới.
  - Review Task 2 nêu Important (chép code), do plan yêu cầu.
  - Repo đã có 3 bản tương tự trong scene, cwsl và volume; gộp chung nằm ngoài plan.
- **R7 (cần người dùng quyết):** giữ timeout chờ chip trả cmd 17 là 2 s, vì người dùng đã chọn.
  - Review cuối nêu Important: `commands.md` ghi chip phát welcome (vài giây) rồi mới trả `[1]`.
  - Nếu đúng vậy, mọi lệnh 230 thành công vẫn trả code 52, dù voice_id WB2 nhớ vẫn đúng.
- **R8:** bản tin 105 gửi qua BLE còn khoảng 6 byte trước giới hạn 256 B. Hiện vẫn đủ nên không sửa.
- **R9:** giữ workspace SDD `.superpowers/sdd/002_plan-them-230-231-voice-id/`, vì chưa có commit.

## Đã làm

- **Task 1, driver UART:**
  - cmd 17/18: hàm gửi, kiểm frame, callback;
  - WB2 nhớ voice_id: cập nhật từ mọi frame cmd 18, và từ cmd 17 `[1]` với giá trị đã gửi (kể cả trả muộn);
  - hỏi cmd 18 sau khi nhận version: thử lại sau 1 s, tối đa 3 lần, tạm dừng khi đang học lệnh hoặc OTA;
  - thêm Test 18 và 19.
- **Task 2, module mới `app_voice.c/.h` (230/231):**
  - mã 64/69/67/70/52, chờ 2 s, rồi 2 s bỏ frame đến muộn, tách riêng cho set và get;
  - chặn hai chiều với âm lượng;
  - runner mới `test_app_voice_runner.c`.
- **Task 3, nối vào hệ thống:**
  - 6 chỗ chặn trong `app_cwsl.c`;
  - trong `process_data.c`: thêm 230/231 vào bảng lệnh, gọi init/poll, chặn OTA 129/134, thêm `"voice_id"` vào 100/105;
  - Test 10 của cwsl runner;
  - cập nhật `uart_wb2_ci13162.md`.
- **Review:**
  - 3 task đều Approved;
  - review cuối (Opus): sẵn sàng, cần đo thời gian chip trả cmd 17 trên board;
  - lượt sửa cuối: header, tài liệu, test 231 hết giờ và OTA; re-review xác nhận đã xử lý hết.
- **Kết quả trên cây cuối** (Claude tự chạy lại):
  - runner: uart 19, cwsl 10 (`failed=0`), scene 17, voicebox 9, voice 14, đều pass;
  - `make` exit 0, chỉ còn warning cũ `aiio_ci13162_print_hex`;
  - `switchv3.bin` 928.204 B, có chuỗi `voice_id`.

## Không làm

- Không sửa `hnn_voiceboxv1.xlsx`. Người dùng cần thêm `act_id` vào mục 17 và thêm 231.
- Không đổi timeout 2 s (R7).
- Không gộp các hàm đọc số JSON giữa các module (R6).

## Chưa kiểm

- Chưa kiểm trên board (theo checklist cuối plan):
  - đo thời gian từ lúc gửi cmd 17 tới lúc nhận `[1]`;
  - log hỏi cmd 18 lúc khởi động;
  - `voice_id` trong 105;
  - 230 với voice_id 2 và 5;
  - 231;
  - 230 trong lúc đang học lệnh.
- Chưa commit.
