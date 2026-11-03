# Liệt kê lệnh UART giữa WB2 và CI13162

## Hỏi

Dựa vào code hiện tại, liệt kê các lệnh giao tiếp giữa WB2 và chip giọng nói CI13162 ra một file `.md`. Trước đó người dùng hỏi UART nối chip giọng nói dùng IO nào: TX là GPIO16, RX là GPIO7, nguồn chip là GPIO17.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định

- Đặt file ở `docs/voice/uart_wb2_ci13162.md`, cạnh `commands.md` (spec phía chip).
- Chỉ ghi những gì code WB2 thật sự gửi và nhận. Chỗ lệch so với `commands.md` gom vào một bảng riêng (mục 7).

## Đã làm

- Đọc `aiio_ci13162_uart.c/.h`, `aiio_ci13162_config.h`, `app_cwsl.c`, `app_volume.c`, `buzz.c`, `m_ci13162_ota.c`, `ci13162_ota_protocol.h/.c`.
- Viết file gồm: kết nối và cấu hình, luật parser, bảng WB2 → chip, bảng chip → WB2, mã trạng thái CWSL, giao thức OTA nhị phân, bảng lệch với spec.
- Lệch đáng chú ý nhất: parser chỉ nhận `CMD_SCENE` với `data` 1..10 và `cmd_id` = 55 + data. Vì vậy lệnh nhà 71..103 và ngữ cảnh 11..15 trong `commands.md` (model 2026-11-01) bị từ chối, sinh MQTT 299. `app_cwsl_notify_voice_cmd()` cũng bỏ `cmd_id` > 65.
- Các lệch khác: không có mute/unmute (`cmd:8` `[8]`/`[9]`); bỏ qua byte `muted` của `cmd:9`; LEARN không gửi `value`; chưa cài `cmd` 15..18 và 201..204.

## Không làm

- Không sửa firmware để khớp với spec mới.
- Không sửa `commands.md`.

## Chưa kiểm

- Chưa đối chiếu bằng log UART thật trên board.
- Chưa commit file tài liệu và file nhật ký.
