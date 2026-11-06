# Phân tích: vẫn thấy "log" trên UART0 ở bản V1

## Hỏi

Người dùng báo vẫn thấy log trên UART0.

## Branch

/home/taodc/wb2/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Đã làm

- `app_config.h` đang để `VOICEBOX_HW_V1`. `build_out/switchv3.bin` build lúc
  17:28. ELF không có chuỗi debug (`[CI13162][TX]`: 0).
- File picocom mới nhất `sdr_2026-11-05_17:29:24.log` có 6 dòng
  `{"cmd":3,"data":[]}`, dòng đầu có thêm byte 0xFF ở trước. Không có dòng log
  chữ nào.
- `{"cmd":3,"data":[]}` là frame WB2 hỏi version chip, không phải log. Trên V1,
  UART0 TX16 chính là dây sang chip, nên đầu USB-serial gắn vào UART0 sẽ thấy
  mọi frame WB2 gửi chip.
- Bộ hỏi version lúc boot gửi tối đa 3 lần
  (`AIIO_CI13162_VERSION_MAX_ATTEMPTS`), lần đầu sau 3 s, cách nhau 1 s. Ngoài
  ra không có chỗ nào khác gọi `aiio_ci13162_uart_request_version()`.
  Vì vậy 6 dòng nghĩa là chip không trả lời version, và có thể WB2 đã boot 2
  lần trong lúc ghi log. Đây là suy luận, chưa kiểm.
- Giả thuyết: đầu USB-serial dùng để flash vẫn nối vào GPIO7 (RX của WB2).
  Khi đó TX của adapter tranh dây với TX của chip, WB2 không nhận được trả lời.

## Quyết định

- Người dùng xác nhận chỉ thấy frame `{"cmd":...}`, không có dòng log chữ nào.
  Như vậy log đã tắt đúng trên V1.
- Người dùng xác nhận adapter nối cả TX và RX trong lúc chạy. Giả thuyết tranh
  dây GPIO7 vì vậy càng có khả năng.
- Hướng dẫn: sau khi flash thì rút dây TX của adapter khỏi GPIO7. Chỉ dùng RX
  của adapter để nghe GPIO16 (WB2 → chip), và nếu cần thì thêm RX thứ hai để
  nghe GPIO7 (chip → WB2).
- Cách kiểm khi không có log: chip trả version thì mỗi lần boot chỉ thấy 1 lần
  `{"cmd":3,"data":[]}`, rồi tới `{"cmd":18,"data":[]}` (hỏi giọng). Thấy
  `cmd 3` lặp 3 lần thì là chip không trả lời.

## Không làm

Không sửa code.

## Chưa kiểm

- Chưa đo lại sau khi rút TX adapter khỏi GPIO7.
- Chưa rõ byte 0xFF ở đầu là do lúc remux chân hay do picocom mở cổng giữa chừng.
