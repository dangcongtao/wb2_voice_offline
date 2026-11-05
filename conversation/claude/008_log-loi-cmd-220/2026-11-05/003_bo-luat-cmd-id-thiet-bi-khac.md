# Bỏ luật cmd_id của thiết bị khác trong CWSL

## Hỏi

- Hỏi vì sao còn dòng `if ((cmd_id >= 2U) && (cmd_id <= 6U))` trong
  `app_cwsl_check_mutating_cmd_id()`.
- Người dùng chốt: code này chỉ chạy cho VOICEBOX, không chạy cho thiết bị khác.

## Branch

/home/taodc/wb2/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Quyết định

- Thay quyết định "chỉ áp cho VOICEBOX, thiết bị khác giữ luật cũ" ở file 002.
  Giờ CWSL dùng một luật duy nhất, không xét `device_type`.
- 220/221: nhận 1..6, 48..55, 71..103. 56..70 trả 64. Ngoài tập trả 65.
- 223: nhận 1..6, 48..103 và 255.

## Đã làm

- `app_cwsl.c`:
  - `app_cwsl_check_mutating_cmd_id()`: bỏ nhánh non-VOICEBOX (khoảng 1..70,
    chặn 2..6, chặn 54/55).
  - `app_cwsl_parse_query()`: bỏ nhánh `device_type`.
  - Viết lại bảng `app_cwsl_run_validation_tests()` (chỉ có khi `BUILD_TEST`)
    theo luật mới, 73 case. Trước đó bảng theo luật quạt, sau khi đổi luật
    fail 32/61.
- `app_cwsl.h`: sửa comment `APP_CWSL_CMD_ID_MAX`, vì macro giờ chỉ còn dùng
  cho MQTT 224.
- `test_app_cwsl_runner.c`:
  - Test 2-4 và 10-13: đổi cmd_id 7 thành 72.
  - Test 2: case 54/55 bị chặn đổi thành 47/8, case 71 thành 104.
- `docs/voice/uart_wb2_ci13162.md`: bỏ dòng luật thiết bị khác.
- Kết quả:
  - Host test: cwsl 13/13, bảng tự chạy 73/73 không fail. uart 21/21,
    voicebox 10/10, voice 46/46.
  - `make -j16`: OK.

## Không làm

- `app_cwsl_notify_voice_cmd()` vẫn còn hai chỗ xét `device_type`
  (`m_fan_need_buzz` cho non-VOICEBOX, LED/BLE cho VOICEBOX). Ngoài phạm vi
  kiểm cmd_id nên chưa đụng.
- Test runner vẫn dùng key `fanoem` (device_type 0). Validation không còn
  phụ thuộc device_type nên không đổi.

## Chưa kiểm

Chưa flash, chưa test trên chip thật. Chưa commit.
