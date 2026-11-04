# Bỏ macro log lỗi, gọi os_trace trực tiếp

## Hỏi

Người dùng hỏi sao không viết thẳng `os_trace("[ERROR]: ...")` thay cho macro `APP_CWSL_LEARN_ERROR` ở bước 001. Sau khi nghe giải thích, người dùng chọn cách 1: bỏ macro và chấp nhận 221/222/223 cũng in log ở các hàm dùng chung.

## Branch

- /home/taodc/wb2/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Quyết định

- Bỏ macro `APP_CWSL_LEARN_ERROR` và điều kiện chỉ log cho 220. Mọi chỗ log gọi `os_trace("[ERROR]: <nguyên nhân>, in[%s]\r\n", ..., __func__)`.
- Hệ quả: lỗi phát hiện trong `app_cwsl_parse_common` (220/221/222/223), `app_cwsl_parse_mutating` và `app_cwsl_start_mutating` (220/221) đều in log. Thay cho quyết định "chỉ log 220" của file 001.
- Log gửi UART lỗi đổi từ "UART send LEARN" thành `UART send cmd %lu to CI13162 failed`, vì nhánh này giờ in cả cho 221.

## Đã làm

- `app_cwsl.c`: thay 21 chỗ gọi macro bằng `os_trace` trực tiếp, xóa định nghĩa macro, ngắt các dòng quá dài. Nội dung nguyên nhân và hành vi trả về giữ như file 001.
- Test host `test_app_cwsl_runner.c` (`-Wall -Wextra -Wformat=2`): 13/13 pass, không có warning ở `app_cwsl.c`. Số dòng log tăng, ví dụ `slot=15` từ 8 lên 13, có thêm `act_id` từ case 223. Đúng như dự kiến khi 221/223 cũng log.
- Build firmware `make -j16` exit 0, `app_cwsl.o` build lại. `switchv3.bin` 932.028 B.

## Không làm

- Không thêm log cho các nhánh từ chối riêng của 221 (`cmd_id=255` với `slot≠255`), 222 (`app_cwsl_stop`), 223 (`app_cwsl_parse_query`, `app_cwsl_query`).
- Không log final event lỗi (timeout, mã lỗi chip).

## Chưa kiểm

- Chưa flash, chưa test trên thiết bị thật.
- Chưa commit.
