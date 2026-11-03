# Thực hiện plan: WB2 nhận lệnh nhà 71..103 qua `cmd 12`

## Hỏi

Chạy plan `002_plan-lenh-nha-cmd12.md` theo cách native (Claude tự làm trong phiên, cuối có 1 reviewer).

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102` (HEAD `6fe9bffa`)

## Quyết định

- Làm thẳng trên working tree, không tạo worktree, không commit. Các file test có thay đổi chưa commit của việc khác, nên phải giữ nguyên.
- Reviewer chỉ báo lỗi Minor. Riêng câu tài liệu mình viết mới (`uart_wb2_ci13162.md` dòng 97, 122) nói ngữ cảnh chưa cấu hình vẫn nháy LED/BLE, không khớp code. Plan yêu cầu mọi câu phải khớp code, nên nâng lên Important và sửa.

## Đã làm

- Task 1, UART:
  - `app_cwsl.h` thêm `APP_CWSL_HOME_CMD_ID_MIN 71U` và `APP_CWSL_HOME_CMD_ID_MAX 103U`;
  - `aiio_ci13162_uart.c` sửa `case AIIO_CI13162_CMD_SCENE` cho nhận dạng lệnh nhà (`data` 71..103, `cmd_id == data`); bỏ comment `/* problem here */`;
  - Test 21 trong `test_uart_protocol_runner.c`. RED ở ca 71, GREEN sau khi sửa.
- Task 2, CWSL:
  - `app_cwsl_notify_voice_cmd()` cho 71..103 đi tiếp;
  - `test_app_cwsl_runner.c`: mock scene điều khiển được (`s_mock_scene_active`), ca 71 ở Test 5 đổi thành có publish, thêm Test 12. RED ở Test 5, GREEN sau khi sửa.
- Task 3: sửa `uart_wb2_ci13162.md` mục 4 (dòng `12 SCENE`, câu về MQTT 224) và bỏ dòng `CMD_SCENE (12)` ở mục 7.
- Kết quả:
  - 4 runner: uart `ALL 21`, cwsl `ALL 12` (`validation total=52 failed=0`), scene `ALL 17`, voicebox `ALL 10`;
  - `make` exit 0, `switchv3.bin` 929.740 B;
  - warning duy nhất trong 2 file C đã sửa là `aiio_ci13162_print_hex` không dùng, có từ trước ở HEAD.
- Review (opus): không có Critical/Important, kết luận sẵn sàng. Reviewer chạy mutation, 9/10 mutation làm test fail.

## Không làm

- Hoãn 3 lỗi Minor:
  - chưa có test kiểm `m_fan_need_buzz()` không bị gọi cho lệnh nhà (chỉ ảnh hưởng thiết bị không phải VOICEBOX);
  - dải 71..103 được kiểm ở 2 chỗ, có thể gom thành 1 helper;
  - Test 12.2 chưa assert số lần LED/BLE.
- Bảng self-test parser có sẵn (`aiio_ci13162_run_parser_tests`) lỗi 36/91 dòng ở cả HEAD lẫn sau khi sửa. Không runner nào assert kết quả bảng này. Không thuộc plan.
- Không mở rộng 220/221/223, `cmd 6`; không sửa `commands.md`, `hnn_voiceboxv1.xlsx`.

## Chưa kiểm

- Chưa commit.
- Chưa kiểm trên board:
  - nói "BẬT-ĐÈN": log `[CI13162] voicebox cmd=0x0C cmd_id=102`, không có 299, có `{"voiceboxv1":224,"cmd_id":102,"cwsl":255}`, LED nháy, log `m_ble_send_voicebox_state cmd_id: 102`;
  - nói "KÍCH-HOẠT-NGỮ-CẢNH-MỘT" khi chưa thêm ngữ cảnh 56: không có 224;
  - lệnh âm lượng chạy như cũ.
