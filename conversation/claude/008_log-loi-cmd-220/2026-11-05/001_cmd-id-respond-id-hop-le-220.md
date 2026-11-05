# cmd_id và respond_id hợp lệ của cmd 220

## Hỏi

cmd 220 hỗ trợ những cmd_id nào, respond_id nào.

## Branch

/home/taodc/wb2/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Đã làm

Đọc `app_cwsl_parse_mutating()` (`smarthome_devices/components/application/app_cwsl.c`)
và `app_scene_respond_id_is_valid()` (`app_scene.h`).

- respond_id (bắt buộc): 0..3, 11..19, 40..47, 50..68, 70, 71. Thiếu, sai
  kiểu hoặc ngoài tập này thì code 64.
- cmd_id học được: 1, 7..53. Sai thì trả:
  - 255: code 65
  - 0, > 70: code 65
  - 2..6: code 64
  - 56..70 (scene, dùng 227): code 64
  - 54, 55 (khoá theo policy): code 65
- slot: cmd_id 1 nhận 0..4 hoặc 255; cmd khác nhận 0..14 hoặc 255. Sai thì code 66.
- Thứ tự kiểm: field chung, cmd_id/slot đọc được, respond_id, rồi mới xét
  khoảng cmd_id/slot.

## Phát hiện

Commit 6168b039 ("cho phép cmd_id tới 103") đổi nhánh `device_type == VOICEBOX`
thành chặn `cmd_id > 103`. Nhưng trước đó đã có check
`cmd_id > APP_CWSL_CMD_ID_MAX` (= 70, `app_cwsl.h:16`), nên 71..103 vẫn bị
từ chối với code 65. Nhánh `> 103` không bao giờ chạy tới. Test
`test_app_cwsl_runner.c` (cmd_id 71 → 65) đang khớp với hành vi này.

## Không làm

Không sửa code, chưa rõ 71..103 có nên học qua 220 không.

## Chưa kiểm

Chưa chạy test, chưa build.
