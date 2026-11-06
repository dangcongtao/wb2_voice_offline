# Thực hiện macro chọn hardware VOICEBOX V1/V2

## Hỏi

Làm theo plan `001_plan-macro-chon-hardware.md` (đã duyệt).

## Branch

/home/taodc/wb2/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Quyết định

- Tên macro: `VOICEBOX_HW`, giá trị `VOICEBOX_HW_V1` (cũ) / `VOICEBOX_HW_V2`
  (mới, mặc định). Đặt trong `app_config.h`, có `#ifndef` để build bằng
  `-DVOICEBOX_HW=1`. Giá trị khác V1/V2 thì báo `#error`.
- Các header dùng macro (`hardware.h`, `aiio_ci13162_config.h`,
  `app_button.h`) báo `#error` nếu thiếu `VOICEBOX_HW`. Lý do: các test host
  chặn `app_config.h`, và `#if` với macro chưa định nghĩa sẽ âm thầm chọn V1.
- Driver UART giữ tên biến `s_aiio_ci13162_uart`. Riêng V1 thì
  `#define s_aiio_ci13162_uart uart_stdio`, để không phải sửa 26 dòng dùng
  biến này.

## Đã làm

- `app_config.h`: thêm macro. V1 ép `DEBUG_ENABLE 0`, nên `main()` gọi
  `bl_sys_logall_disable()` và `os_trace` không in.
- `aiio_ci13162_config.h`: include `app_config.h`. V1 dùng UART0 16/7, V2 dùng
  UART1 4/3.
- `aiio_ci13162_uart.c`, chỉ cho V1, làm lại theo driver UART0 cũ (trước
  e43f22cd):
  - dùng `uart_stdio`;
  - `hold_tx_idle` thêm `UART_Disable(UART0)`;
  - sau init thì cấu hình GPIO7 làm UART0_RXD (HBN + `GLB_UART_Fun_Sel`);
  - init lỗi thì gọi `aiio_ci13162_restore_tx_only()`.
  V2 không đổi. Sửa comment trong `.h` cho đúng cả hai bản.
- `hardware.h`: V1 có 1 nút GPIO1, `BTN_VOICEBOX_COUNT 1`. Thêm
  `BTN_VOICEBOX_PINS`. `button.c` tra chân theo bảng này thay cho `switch`.
- `app_button.h`: V1 có function = index 0, volume = 0xFF (không có nút).
- `m_ci13162_ota.c`: log ghi `UART%u` theo `AIIO_CI13162_UART_ID`.
- Test:
  - 4 runner định nghĩa `VOICEBOX_HW`, mặc định V2.
  - Nhánh V1: `test_uart_protocol_runner` Test 1b (UART0 16/7, GPIO7,
    `UART_Disable`, OTA không bật/tắt log).
  - `test_voicebox_runner`: chân và nút GPIO1.
  - `test_app_voice_runner`: nút đơn ở index 0. Test nút function dùng
    `APP_BUTTON_INDEX_FUNCTION`.
- `docs/voice/uart_wb2_ci13162.md`: bảng chọn hardware, khác biệt của V1, nút V1.
- Kết quả:
  - Host test V2: cwsl 13/13, uart 21/21, voicebox 10/10, voice 46/46.
  - Host test V1: cwsl 13/13, uart 21/21, voicebox 10/10, voice 45/45.
    Đã xác nhận nhánh V1 có chạy.
  - Firmware `make -j16`: V1 OK, ELF không còn chuỗi debug (0, so với 9 ở V2).
    V2 OK. Không có warning mới ở file đã sửa.
  - Mặc định đã trả về V2. `build_out` hiện là bản V2.
  - `uart_stdio` nằm ở `.data` (objdump), ghi được.

## Không làm

- Không sửa SDK: `puts()` lúc boot vẫn ra UART0 TX16 trên V1.
- Không đưa lại `gpio_log.c`.
- `test_app_scene_runner.c` vẫn không link được (thiếu mock
  `app_cwsl_start_resp_set`). Lỗi có từ trước.
- Logic âm lượng/giọng qua MQTT và giọng nói giữ nguyên trên V1.

## Chưa kiểm

- Chưa flash, chưa chạy trên board V1 hay V2.
- Chưa commit.
