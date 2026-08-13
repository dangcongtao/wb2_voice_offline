# Giao thức lệnh quạt từ CI13162 qua UART

> Trạng thái: bản nháp để duyệt. Bảng ánh xạ lệnh thoại được đọc từ
> `[60000]{cmd_info_old}.xlsx`, sheet `<0>cmd`.
>
> CI13162 gửi từ UART1 TX; phía thiết bị BL602 nhận bằng UART0 RX tại GPIO7.
> UART1 của BL602 (GPIO4/GPIO3, 9600 baud) vẫn dành cho FT62F28.

## 1. Cấu hình UART

| Thuộc tính | Giá trị |
|---|---|
| Cổng trên chip giọng nói | UART1 |
| Baud rate | 115200 |
| Cấu hình khung UART | 8N1 |
| Kiểu dữ liệu | ASCII |
| Kết thúc một message | `\r\n` |

## 2. Format message

Firmware BL602 chấp nhận cả hai format dưới đây:

```text
# Format JSON chuẩn từ CI13162 (khuyến nghị)
{"cmd":<id>,"data":[<value_0>,<value_1>,...]}

# Format tương thích cũ, key không có dấu nháy
{cmd:<id>,data:[<value_0>,<value_1>,...]}
```

Ví dụ tương đương:

```text
{"cmd":1,"data":[1]}\r\n
{cmd:1,data:[1]}\r\n
```

Parser cho phép khoảng trắng quanh các token. Hai key phải cùng thứ tự `cmd`, `data`;
không chấp nhận key phụ hoặc dữ liệu theo sau dấu `}`.

| Trường | Ý nghĩa |
|---|---|
| `cmd` | ID của nhóm lệnh trong `command_type_t` |
| `data` | Mảng tham số của lệnh; để trống nếu lệnh không cần tham số |

## 3. Enum command

```c
typedef enum
{
    CMD_RESET   = 0,
    CMD_TURN    = 1,
    CMD_MODE    = 2,
    CMD_VERSION = 3,
} command_type_t;
```

| Enum | `cmd` | `data` hợp lệ | Hướng | Hành động |
|---|---:|---|---|---|
| `CMD_RESET` | 0 | `[]` | CI13162 → BL602 | Kích hoạt hàm software reset |
| `CMD_TURN` | 1 | `[0]` | CI13162 → BL602 | Tắt quạt |
| `CMD_TURN` | 1 | `[1]` | CI13162 → BL602 | Bật quạt |
| `CMD_MODE` | 2 | `[1]` | CI13162 → BL602 | Chọn tốc độ số 1 |
| `CMD_MODE` | 2 | `[2]` | CI13162 → BL602 | Chọn tốc độ số 2 |
| `CMD_MODE` | 2 | `[3]` | CI13162 → BL602 | Chọn tốc độ số 3 |
| `CMD_MODE` | 2 | `[4]` | CI13162 → BL602 | Chọn chế độ gió tự nhiên |
| `CMD_VERSION` | 3 | `[]` | BL602 → CI13162 | Hỏi firmware version của chip giọng nói |
| `CMD_VERSION` | 3 | `[<version>]` | CI13162 → BL602 | Trả version; BL602 chỉ log, không điều khiển quạt |

`CMD_VERSION` khác các lệnh quạt: BL602 chủ động hỏi, CI13162 trả lời. Message hỏi dùng `data` rỗng; message trả lời bắt buộc đúng 1 số. Parser RX của BL602 từ chối `{"cmd":3,"data":[]}` (`INVALID_DATA`) vì đó là frame TX, không phải reply.

## 4. Ánh xạ lệnh thoại sang message UART1

| Câu lệnh | ASR command ID | Message gửi sang MCU | Ý nghĩa |
|---|---:|---|---|
| `BẬT-QUẠT` | 7 | `{"cmd":1,"data":[1]}` | Bật quạt |
| `TẮT-QUẠT` | 8 | `{"cmd":1,"data":[0]}` | Tắt quạt |
| `BẬT-SỐ-MỘT` | 9 | `{"cmd":2,"data":[1]}` | Chọn tốc độ số 1 |
| `BẬT-SỐ-HAI` | 10 | `{"cmd":2,"data":[2]}` | Chọn tốc độ số 2 |
| `BẬT-SỐ-BA` | 11 | `{"cmd":2,"data":[3]}` | Chọn tốc độ số 3 |
| `CHẾ-ĐỘ-GIÓ-TỰ-NHIÊN` | 12 | `{"cmd":2,"data":[4]}` | Chọn chế độ gió tự nhiên |

`CMD_RESET` và `CMD_VERSION` chưa được ánh xạ với một câu lệnh thoại trong workbook hiện tại.

## 4.1. Hỏi version chip giọng nói (`cmd` = 3)

Hàm `aiio_ci13162_uart_request_version()` gửi chuỗi cố định trên UART0 TX GPIO16:

```c
static const char s_aiio_ci13162_version_query[] = "{\"cmd\":3,\"data\":[]}\r\n";
```

Hiện được gọi từ `app_get_info()` (MQTT lệnh 105). Không gửi khi UART chưa sẵn sàng hoặc khi OTA đang giữ UART0.

Luồng:

```text
BL602  -- {"cmd":3,"data":[]}\r\n          -->  CI13162
BL602  <-- {"cmd":3,"data":[<version>]}\r\n --  CI13162
```

Khi nhận reply hợp lệ, `aiio_ci13162_uart_process()` chỉ in log:

```text
[CI13162] version=<version>
```

Ví dụ test trong parser: `{"cmd":3,"data":[260813]}` là hợp lệ; `{cmd:3,data:[]}` nhận về là không hợp lệ.

## 5. Ví dụ message hoàn chỉnh

```text
# Software reset
{"cmd":0,"data":[]}\r\n

# Bật quạt
{"cmd":1,"data":[1]}\r\n

# Tắt quạt
{"cmd":1,"data":[0]}\r\n

# Chọn tốc độ số 2
{"cmd":2,"data":[2]}\r\n

# Chọn chế độ gió tự nhiên
{"cmd":2,"data":[4]}\r\n

# BL602 hỏi firmware version của CI13162
{"cmd":3,"data":[]}\r\n

# CI13162 trả firmware version (ví dụ 260813)
{"cmd":3,"data":[260813]}\r\n
```

## 6. Điểm cần sửa trong workbook

Hai sheet đang không đồng nhất ở lệnh `CHẾ-ĐỘ-GIÓ-TỰ-NHIÊN`:

| Sheet | Command ID | Semantic ID | Prompt ID |
|---|---:|---|---:|
| `<0>cmd` | 12 | `0x000D` | 12 |
| `<1>wake` | 13 | `0x000D` | 13 |

Nên thống nhất dòng này thành command ID `12` và prompt ID `12` ở cả hai sheet
trước khi compile lại `cmd_info`.
