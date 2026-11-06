# Plan: macro chọn hardware VOICEBOX cũ (V1) / mới (V2)

## Hỏi

- Hardware cũ dùng UART0 nối chip giọng nói và 1 nút nhấn để cấu hình + đổi giọng.
- Vì dùng UART0 nên luôn phải tắt log, không cần log qua GPIO.
- Dùng macro để chọn hardware cũ hoặc mới.

## Branch

/home/taodc/wb2/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Quyết định (người dùng chốt)

- Chân V1 giống commit 38ee6ae6: chip UART0 TX GPIO16 / RX GPIO7, nút GPIO1,
  LED GPIO2, nguồn chip GPIO17.
- Nút V1 chạy như nút giữa (function) hiện tại: giữ 5 s rồi thả để smart
  config, giữ 10 s rồi thả để AP config, nhấn nhanh để đổi giọng. Không có nút
  âm lượng.
- Nút V1: nhấn = 0, có pull-up (mức nghỉ 1).
- Macro đặt trong `app_config.h`, mặc định là hardware mới. Chọn V1 thì luôn tắt
  log, không phụ thuộc DEBUG_ENABLE.
- Người dùng duyệt plan dưới đây.

## Plan

### Macro (`app_config.h`)

```c
#define VOICEBOX_HW_V1 1  /* cũ: chip UART0 TX16/RX7, 1 nút GPIO1 */
#define VOICEBOX_HW_V2 2  /* mới: chip UART1 TX4/RX3, 3 nút 21/20/0, log UART0 */
#define VOICEBOX_HW VOICEBOX_HW_V2
```

- V1 ép `DEBUG_ENABLE` = 0. Khi đó `os_trace` không in, `main()` gọi
  `bl_sys_logall_disable()`, và `printf`/blog của SDK bị chặn.
- Không đưa lại `gpio_log.c`. `HARDWARE_VERSION` giữ nguyên (không có chỗ dùng).

### UART chip (`aiio_ci13162_config.h`, `aiio_ci13162_uart.c`)

V1 làm lại đúng như driver UART0 cũ (trước commit e43f22cd):
- dùng lại `uart_stdio` của SDK;
- `hold_tx_idle` kéo GPIO16 lên mức cao rồi `UART_Disable(UART0)`;
- sau khi init thì cấu hình lại GPIO7 làm UART0_RXD (`HBN_*`, `GLB_UART_Fun_Sel`);
- init lỗi thì trả UART0 về chế độ chỉ TX;
- OTA chip không cần bật/tắt log, vì log luôn tắt.

V2 giữ nguyên. Cờ `CUSTOM_LOG_IO` (16/7) trong Makefile khớp với V1, không đổi.

### Nút (`hardware.h`, `button.c`, `app_button.h`, `m_button.c`)

- V1 có 1 nút ở GPIO1, index 0, nhấn = 0, có pull-up, chạy như nút giữa.
- Không có tăng/giảm âm lượng bằng nút. Âm lượng qua MQTT và giọng nói vẫn chạy.
- LED GPIO2 và nguồn GPIO17 giữ nguyên.

### Test và kiểm

- Host test cho V2 (test hiện có) và V1 (chân, số nút, index, log bị ép tắt).
- Build firmware cả hai bản.
- Cập nhật `docs/voice/uart_wb2_ci13162.md`.

### Rủi ro

V1: 3 dòng `puts()` lúc boot của SDK vẫn ra UART0 TX16, vào chip. Lỗi này có từ
trước với hardware cũ. Muốn sửa phải sửa SDK, nên không đụng.
