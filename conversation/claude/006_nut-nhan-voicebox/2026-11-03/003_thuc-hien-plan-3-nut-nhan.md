# Thực hiện plan 3 nút nhấn VOICEBOX bằng subagent

## Hỏi

Người dùng duyệt plan `002_plan-3-nut-nhan-voicebox.md`: "duyệt, triển khai plan theo subagent".

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định (ruling khi chạy)

- **R1:** dùng bản sao `run-tests.sh` riêng của plan này, cùng cờ gcc.
- **R2:** test `button_post_rules` tự đặt handle queue tĩnh về NULL, để kết quả không phụ thuộc thứ tự chạy test.
- **R3 (người dùng chọn "bản commit"):**
  - Trước khi chạy plan, `hardware.h` và `m_button.c` có sẵn chỗ sửa dở của người dùng (`MAX_NUM_BUTTON` 6→3, bỏ phần riêng của VOICEBOX). Task 4 thay các dòng đó bằng code mới:
  - `MAX_NUM_BUTTON` trở lại 6 cho các thiết bị khác;
  - VOICEBOX quét `BTN_VOICEBOX_COUNT` = 3 nút, mức nghỉ 1.
- **R4:** khi hàng đợi đầy (4 hành động), lần nhấn bị bỏ, không beep.
- **R5:** sửa sau review cuối. Nút 2 chỉ đổi giọng khi đã thấy sự kiện nhấn trước đó.
  - Nếu chỉ có sự kiện thả mà không có nhấn (ví dụ lúc khởi động IO20 đọc 0) thì không làm gì.
  - Giữ để cấu hình vẫn như cũ.
- **R6:** giữ workspace `.superpowers/sdd/002_plan-3-nut-nhan-voicebox/`, vì chưa commit.
- Tôi đã sửa lại nhận định trong `001_...`: nút GPIO1 không bị ngược mức. Bản commit có đặt mức nghỉ 1; chỉ bản người dùng đang sửa dở mới bỏ dòng đó.

## Đã làm

- **Task 1, driver:**
  - cmd 9 chỉ nhận 3 phần tử;
  - nhớ cờ muted từ cmd 9 và 13;
  - thêm `aiio_ci13162_uart_get_volume_state()` và `aiio_ci13162_uart_send_mute()`;
  - callback âm lượng có thêm `muted`.
- **Task 2:** thêm `app_volume_button_step(bool)` và `app_voice_button_next()`.
  - Chưa biết trạng thái thì hỏi chip trước.
  - Dùng chung cơ chế chờ chip với 225/230.
  - Báo App 225/230 với `u` 0, `act_id` 1.
- **Task 3:** module `app_button` gồm máy trạng thái và hàng đợi 4 phần tử, cùng `app_cwsl_is_learning()`.
- **Task 4:**
  - IO21/20/0 có pull-up, mức nghỉ 1; bỏ GPIO1;
  - nhánh VOICEBOX trong `m_button.c`;
  - `process_data.c` gọi init và poll;
  - Test 10 của voicebox runner;
  - cập nhật `uart_wb2_ci13162.md`.
- **Review:**
  - 4 task đều Approved;
  - review cuối (Opus): sẵn sàng, cần sửa 1 Important (R5);
  - lượt sửa cuối F1–F4, re-review xác nhận đã xử lý hết.
- **Kết quả trên cây cuối** (Claude tự chạy lại):
  - runner: uart 20, cwsl 11 (`failed=0`), scene 17, voicebox 10, voice 46;
  - `make` exit 0, không có warning ở các file đã sửa;
  - `switchv3.bin` 929.676 B.

## Không làm

- Không sửa thư viện `huytv_button`, nên không có chống dội ngoài việc thư viện quét mỗi 50 ms.
- Không đụng 2 dòng comment người dùng đang tự thêm trong `aiio_ci13162_uart.c` (`/* cmd scene */`, `/* problem here */`).

## Chưa kiểm

- Chưa kiểm trên board. Làm theo checklist cuối plan, thêm 3 mục:
  - bật nguồn mà không chạm nút: không có welcome, không có 230;
  - nhấn đúp nhanh nút 1;
  - nhấn nút 1 một lần mà tiếp điểm dội: không được tăng 2 mức.
- Chưa commit.
