# Kiểm tra log qua GPIO thay UART0

## Hỏi

UART0 của WB2 đã dùng để nối chip giọng nói. Code đã có log qua GPIO thay cho UART0 chưa?

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Đã làm

- Có sẵn: `components/hardware/gpio_log.c` là UART mềm (bit-bang) trên GPIO14, 9600 8N1 (`TIME_BIT_US 104`), không có RX. File đã nằm trong `hardware/bouffalo.mk`.
- Bật bằng `CONFIG_HUNONIC_LOG_GPIO:=1` trong `proj_config.mk` (đang bật). Cờ này sinh `-DPROJECT_HUNONIC_LOG_GPIO`, khiến SDK gọi `sw_uart_init()` trong `bfl_main.c` và `sw_uart_send_byte()` trong `printf`/`vprint`/`bl_putchar` của `evb/src/debug.c`. Hook này nằm trong SDK đã sửa (`~/wb2/Ai-Thinker-WB2`, branch `dungnt98/main`, commit `20265b52 add log gpio`).
- Hiện `DEBUG_ENABLE 0`: `os_trace` không in, và `main()` gọi `bl_sys_logall_disable()` nên không có log. Muốn xem log thì đặt `DEBUG_ENABLE 1` (khi đó `LED_STT` = 0xFF; VOICEBOX dùng LED GPIO2 nên không đụng GPIO14).
- Phát hiện: `puts()` trong `debug.c` **chưa** được hook, vẫn gọi `bl_uart_data_send(0, c)`, tức là ra UART0 phần cứng. Các chỗ gọi: 3 dòng boot trong `bfl_main.c` (chạy trước `main()` và trước `aiio_ci13162_uart_hold_tx_idle()`), và một số nhánh dump/lỗi của SDK (bloop dump, `bl_mtd`, `looprt`, `dhcp_server_raw`). Code app không gọi `puts`.
- Mỗi byte log, `sw_uart_send_byte()` tắt ngắt khoảng 1,04 ms. Chỉ ảnh hưởng bản debug.
- Lỗi IDE `#include errors` ở `process_data.c` là do `.vscode/c_cpp_properties.json` chỉ include `${workspaceFolder}/**`, không có header SDK. Không liên quan việc gỡ IR; build vẫn chạy.

## Không làm

- Chưa sửa `puts()` trong SDK (repo khác), chờ người dùng quyết.

## Chưa kiểm

- Chưa đo bằng logic analyzer xem 3 dòng boot của `puts()` có thật sự ra GPIO16 tới chip giọng nói không.
- Chưa chạy bản `DEBUG_ENABLE 1` trên board để xem log GPIO14.
