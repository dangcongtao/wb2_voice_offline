# Mở cmd_id 1..6, 48..103 cho 220/221/223 trên VOICEBOX

## Hỏi

Cmd 220 cần hỗ trợ toàn bộ cmd_id đang có: 1..6 và 48..103.

## Branch

/home/taodc/wb2/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Quyết định

- Chỉ áp cho VOICEBOX. Thiết bị khác giữ luật cũ (1, 7..53; chặn 2..6, 54, 55).
- 220/221 vẫn chặn 56..70 (code 64), ngữ cảnh phải đi qua 227/228.
- Mở cùng tập cho 221 DELETE và 223 QUERY. 223 cho cả 56..70 vì chỉ đọc.
- VOICEBOX: 0, 7..47, 104..254 trả code 65. 54, 55 được mở.
- Slot giữ luật cũ: cmd_id 1 là 0..4, còn lại (kể cả 2..6) là 0..14, hoặc 255.

## Đã làm

- `app_scene.h`: chuyển `app_scene_resp_cmd_id_exists()` từ `app_scene.c` lên
  header, đổi tên thành `app_scene_cmd_id_exists()` (static inline). 232 và
  CWSL dùng chung.
- `app_cwsl.c`:
  - Tách phần kiểm cmd_id của LEARN/DELETE ra `app_cwsl_check_mutating_cmd_id()`.
    Thêm nhánh VOICEBOX. Bỏ nhánh `> 103` không bao giờ chạy tới (vì check
    `> APP_CWSL_CMD_ID_MAX` = 70 chạy trước). Lỗi này do commit 6168b039.
  - `app_cwsl_parse_query()`: VOICEBOX nhận `app_scene_cmd_id_exists()` hoặc 255.
  - Sửa warning `%lu` với `uint32_t` (respond_id), ngắt các dòng dài hơn 80 cột
    trong `app_cwsl_parse_mutating()`.
- `aiio_ci13162_uart.c`: CWSL_INFO nhận `data[0]` 1..103 (trước là 1..70), để
  223 hỏi được slot của 71..103. Cập nhật vector parse.
- Test:
  - `test_app_cwsl_runner.c`: thêm Test 6b (220/221/223 với các id hợp lệ và
    không hợp lệ, cùng luật slot).
  - `test_uart_protocol_runner.c`: CWSL_INFO 71/103 OK, 104 INVALID_DATA.
- `docs/voice/uart_wb2_ci13162.md`: cập nhật ràng buộc cmd_id, bỏ dòng lệch
  CWSL_INFO ở mục 7.
- Kết quả:
  - Host test (`-Wall -Wextra -Wformat=2`): cwsl 13/13, uart 21/21, voicebox
    10/10, voice 46/46 đều pass. Trước khi sửa, cwsl abort ở dòng 493 (VOICEBOX
    cmd_id 7 phải trả 65).
  - Build firmware SDK (`make -j16`): OK.

## Không làm

- `test_app_scene_runner.c` không link được (thiếu mock
  `app_cwsl_start_resp_set`). Lỗi có từ trước, không sửa.
- Bảng vector tự chạy `aiio_ci13162_run_parser_tests()` đã fail 36 case ở
  HEAD, vì parser gán `version = data[0]` còn vector mong 0. Bảng không assert
  nên không ai thấy. Không sửa. Hai vector mới 71/103 cũng lệch theo cách này.
- `CWSL_MQTT_SPEC.md` mục 9 vẫn mô tả bản quạt cũ, không sửa.
- Log scene của 221 vẫn ghi "use cmd 227" (đúng ra là 228), giữ nguyên.

## Chưa kiểm

- Chưa flash, chưa test trên chip thật. Chưa rõ chip xử lý ra sao khi học
  cmd_id 2..6 (wake mặc định) hoặc 54/55.
- Chưa commit.
