# Debug: 227 thêm mới trả code 52

## Hỏi

Lệnh nhận: `{"voiceboxv1":227, "u":55519, "cmd_id":255, "act_id":0, "respond_id":1}`
Lệnh gửi đi: `{"voiceboxv1":227,"cmd_id":56,"respond_id":1,"u":55519,"act_id":0,"result":0,"code":52}`

Nguyên nhân là gì?

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Đã làm

- **`cmd_id` 56 trong bản tin** cho thấy WB2 đã cấp ngữ cảnh 56 và đã bắt đầu phiên học. Lỗi xảy ra sau bước đó.
- **Các đường ra 52 khi thêm mới** (`app_cwsl.c`):
  - QUERY_OLD quá 2 s: chip không trả `cmd:6` cho `{"cmd":4,"data":[6,56]}`;
  - LEARNING quá 120 s;
  - chip trả cmd5 `0x34`: chip hết 30 s không có tiến triển.
- **Đọc 3 log serial** `smarthome_devices/sdr_2026-11-03_09:19:36.log`, `..._09:35:01.log`, `..._09:35:58.log`:
  - Firmware build với `DEBUG_ENABLE 1`; có dòng `[SCENE] changing respond_id...` in qua `os_trace`.
  - Ở chế độ này, mọi dòng chip gửi lên đều được in `[CI13162][ASCII] ...`, và bản tin version được in `[CI13162] version=...`.
  - Cả 3 log **không có dòng nào nhận được từ chip**: không có version (WB2 hỏi lúc 3, 4, 5 s sau khi init UART), không có trả lời cho hai lệnh 225, không có `cmd:6`.
  - Dòng duy nhất liên quan là `[CI13162] UART1 TX GPIO4 RX GPIO3, 115200 8N1 ready`.
  - Các log dừng ngay sau `_cmd_number: 227`, nên không thấy lúc WB2 gửi 52.
- **Loại trừ xung đột GPIO17** (chân nguồn chip): `hw_buzz_init` bỏ qua khi thiết bị có chip giọng nói; `hw_temp_config` không chạy trên VOICEBOX.

## Kết luận tạm (chưa xác nhận trên board)

- Rất có thể WB2 không nhận được byte nào từ chip trên UART1 (RX GPIO3). Vì vậy bước hỏi slot cũ (QUERY_OLD) hết 2 s và trả 52.
- Chưa phân biệt được các khả năng sau:
  - chip không nhận được lệnh từ WB2 (TX GPIO4);
  - chip có trả lời nhưng WB2 không nhận (RX GPIO3);
  - đấu dây TX/RX ngược;
  - chip chưa chạy.
- Lưu ý: ngày 2026-11-02, người dùng đổi `aiio_ci13162_config.h` sang TX=3/RX=4 lúc 17:24, rồi đổi lại TX=4/RX=3 lúc 17:54.

- **Theo yêu cầu, thêm log dữ liệu TX/RX của UART giao tiếp CI13162** (`components/hardware/aiio_ci13162_uart.c`):
  - cờ mới `AIIO_CI13162_UART_TRACE_ENABLE`, mặc định bằng `DEBUG_ENABLE`;
  - mọi frame gửi qua `aiio_ci13162_uart_tx()` in `[CI13162][TX] <n> B: <ascii>` và `[CI13162][TX] HEX: ...`, kèm `sent=<số byte> rx_irq=<số lần ngắt RX> rx_bytes=<tổng byte nhận>`;
  - mọi chunk byte thô mà task UART nhận được in `[CI13162][RX] <n> B: ...` và `HEX: ...`;
  - không log frame nhị phân OTA (`aiio_ci13162_uart_ota_write`).
  - Kết quả: 4 runner đều pass (uart 17, cwsl 9 `failed=0`, scene 17, voicebox 9); `make` exit 0, không có warning ở file này; `switchv3.bin` 926.492 B, có chuỗi trace.

- **Người dùng đo bằng logic analyzer:** có frame UART 115200 trên chân, nhưng code không log bản tin nhận.
- **Log `sdr_2026-11-03_10:38:57.log` (có trace TX):**
  - WB2 gửi đủ `{"cmd":3}` (3 lần) và `{"cmd":8,"data":[5]}`, `[4]`, với `sent` bằng đúng độ dài frame;
  - mọi dòng đều có `rx_irq=0 rx_bytes=0`, tức ngắt RX của UART1 chưa từng xảy ra.
  - Kết luận: lỗi nằm trước ngoại vi UART1 (định tuyến chân/tín hiệu, hoặc ngắt), không nằm ở parser hay baud.
- **Đọc SDK:**
  - `hosal_uart_init` (`bl602_hal/hosal_uart.c`) có map tín hiệu `pin % 8` (GPIO3 → SIG3 = UART1_RXD, GPIO4 → SIG4 = UART1_TXD) và có đăng ký, bật ngắt UART1.
  - Trong project chỉ có driver CI13162 init UART1 (`hw_uart_init` không chạy trên VOICEBOX). VFS UART tắt (`CONFIG_SYS_VFS_UART_ENABLE:=0`).
  - Log UART0 init bằng `hosal_uart_init_only_tx`, hàm này chỉ map SIG0 = UART0_TXD và không map RX (dòng map RX bị comment).
  - **Nghi vấn:**
    - Nếu thanh ghi `GLB_UART_SIG_SEL_0` giữ giá trị mặc định (giả định 0x76543210, chưa xác nhận) cho các signal không ai đặt lại, thì SIG7 vẫn là UART1_RXD. Khi đó UART1_RXD được chọn ở cả SIG3 (GPIO3) lẫn SIG7 (GPIO7, chân RX log), và UART1 có thể đang lấy RX từ SIG7.
    - Chưa xác nhận giá trị thanh ghi trên board, cũng chưa xác nhận phần cứng xử lý trường hợp trùng như thế nào.
- **Thêm log chẩn đoán** `[CI13162][DIAG]` (in sau khi init và sau mỗi lần TX), gồm:
  - `sig_sel` (giá trị `GLB_UART_SIG_SEL_0`);
  - function và mức logic của GPIO RX, function của GPIO TX;
  - `rx_fifo` (`UART_GetRxFifoCount(1)`), `rx_irq`, `rx_bytes`.
- **Thêm mock** `GLB_GPIO_Get_Fun`, `GLB_GPIO_Read`, `BL_RD_REG`, `UART_GetRxFifoCount` vào `test_mocks/` để runner build được.
- **Kết quả:**
  - 4 runner đều pass;
  - `make` exit 0, không có warning ở `aiio_ci13162_uart.c`;
  - `switchv3.bin` 926.804 B (10:43), có chuỗi DIAG.

- **Log `sdr_2026-11-03_10:44:24.log` (có DIAG):** mọi dòng đều là `sig_sel=0x76567212 rx_gpio3 fun=7 lvl=0 tx_gpio4 fun=7 rx_fifo=0 rx_irq=0 rx_bytes=0`.
  - `sig_sel` 0x76567212 có SIG3 = 7 và SIG7 = 7, tức **UART1_RXD bị chọn ở 2 tín hiệu**. Giả định giá trị reset 0x76543210 là đúng.
  - `rx_fifo=0`: không có byte nào vào được FIFO của UART1, nên lỗi không nằm ở ngắt.
  - `lvl=0` ở mọi lần đọc, cả lúc init (khi đó chip chưa được cấp nguồn, vì `power_init` chạy sau) lẫn ngay sau mỗi lần TX. UART lúc nghỉ phải ở mức 1, và GPIO3 đang bật pull-up.
- **Nguồn gốc lỗi phía code** (so với commit cũ `3c0796f4`, `aiio_ci13162_uart.c:1811`):
  - Driver UART0 cũ có gọi `GLB_UART_Fun_Sel(GLB_UART_SIG_7, GLB_UART_SIG_FUN_UART0_RXD)`, kèm cấu hình pad GPIO7.
  - Plan chuyển sang UART1 (task 2 của plan 002 hôm 2026-11-02) đã bỏ khối GPIO7 đó, nên SIG7 giữ giá trị reset UART1_RXD. Đây là lỗi hồi quy do plan đó gây ra.
- **Đã sửa:**
  - Sau `hosal_uart_init`, nếu `RX_PIN % 8 != 7` thì gọi `GLB_UART_Fun_Sel(GLB_UART_SIG_7, GLB_UART_SIG_FUN_UART0_RXD)`.
  - Test: viết test trước (fail ở `g_mock_uart_fun_sel_count == 1`), sửa xong thì pass. Mock `GLB_UART_Fun_Sel` có thêm phần ghi lại lần gọi cuối.
  - 4 runner đều pass; `make` exit 0, không có warning; `switchv3.bin` 926.804 B (10:47).
  - Trên board, kỳ vọng `sig_sel` thành `0x36567212`.

- **Log `sdr_2026-11-03_11:07:54.log` (sau khi sửa SIG7):**
  - đã nhận `[CI13162] version=261102`, `rx_irq=52 rx_bytes=51`;
  - có `{"cmd":6,"data":[60,0,0]}`, WB2 gửi tiếp `[1,60,255,1]`;
  - chip trả `[17,60,0]` 2 lần (câu quá ngắn), rồi `[48,60,0]` (học thất bại).
  - Kết luận: RX đã chạy; lỗi SIG7 chính là nguyên nhân 52.
- **Câu hỏi "không gửi MQTT mà vẫn có decoded 227":**
  - Bản tin đến qua MQTT: có dòng `TOPIC=u/20244/HUN179361473625voiceboxv1N1/1793614818`, ngay sau `MQTT_EVENT_SUBSCRIBED`.
  - Ở mọi lần khởi động trong các log hôm nay đều có bản tin này.
  - Không phải mock test: `BUILD_TEST` là 0; test nhúng không đi qua hàng đợi và không in `decoded[...]`.
  - Rất có thể đây là bản tin **retained** trên broker: lần gửi 227 trước đã bật cờ retain. `keepalive=10`, firmware không đặt clean session (chưa kiểm giá trị mặc định của thư viện).
  - Hệ quả: mỗi lần khởi động thiết bị lại tự học một ngữ cảnh mới. Lần này cấp 60, cho thấy 56..59 có thể đã được thêm từ các lần trước.
  - Cách xóa: publish một payload rỗng có cờ retain lên đúng topic đó. Chưa làm, vì đây là hệ thống bên ngoài.

## Không làm

- Không publish lên broker để xóa bản tin retained.
- `lvl=0` trên GPIO3 có thể là vấn đề phần cứng thứ hai: chân bị kéo thấp, chip chưa chạy, hoặc chỗ đo logic analyzer không phải pad GPIO3. Chưa kiểm.

## Chưa kiểm

- Đấu dây thật trên board.
- Thời gian từ lúc gửi 227 tới lúc nhận 52.
- Chip có nhận lệnh 225 (đổi âm lượng) không.
- Chip gửi lệnh thoại lên thì WB2 có in `[CI13162][ASCII]` không.
