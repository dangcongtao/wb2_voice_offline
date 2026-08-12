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
    CMD_RESET = 0,
    CMD_TURN  = 1,
    CMD_MODE  = 2,
} command_type_t;
```

| Enum | `cmd` | `data` hợp lệ | Hành động |
|---|---:|---|---|
| `CMD_RESET` | 0 | `[]` | Kích hoạt hàm software reset |
| `CMD_TURN` | 1 | `[0]` | Tắt quạt |
| `CMD_TURN` | 1 | `[1]` | Bật quạt |
| `CMD_MODE` | 2 | `[1]` | Chọn tốc độ số 1 |
| `CMD_MODE` | 2 | `[2]` | Chọn tốc độ số 2 |
| `CMD_MODE` | 2 | `[3]` | Chọn tốc độ số 3 |
| `CMD_MODE` | 2 | `[4]` | Chọn chế độ gió tự nhiên |

## 4. Ánh xạ lệnh thoại sang message UART1

| Câu lệnh | ASR command ID | Message gửi sang MCU | Ý nghĩa |
|---|---:|---|---|
| `BẬT-QUẠT` | 7 | `{"cmd":1,"data":[1]}` | Bật quạt |
| `TẮT-QUẠT` | 8 | `{"cmd":1,"data":[0]}` | Tắt quạt |
| `BẬT-SỐ-MỘT` | 9 | `{"cmd":2,"data":[1]}` | Chọn tốc độ số 1 |
| `BẬT-SỐ-HAI` | 10 | `{"cmd":2,"data":[2]}` | Chọn tốc độ số 2 |
| `BẬT-SỐ-BA` | 11 | `{"cmd":2,"data":[3]}` | Chọn tốc độ số 3 |
| `CHẾ-ĐỘ-GIÓ-TỰ-NHIÊN` | 12 | `{"cmd":2,"data":[4]}` | Chọn chế độ gió tự nhiên |

`CMD_RESET` chưa được ánh xạ với một câu lệnh thoại trong workbook hiện tại.

Khi bật macro test `AIIO_CI13162_UART_SEND_RESET_ON_TURN_OFF_ENABLE`, sau khi
nhận lệnh tắt quạt, BL602 gửi lại `{cmd:0,data:[]}\r\n` trên UART0 TX GPIO16.
Message test này dùng format tương thích cũ; CI13162 cần chấp nhận format đó nếu dùng phép thử này.

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
```

## 6. Điểm cần sửa trong workbook

Hai sheet đang không đồng nhất ở lệnh `CHẾ-ĐỘ-GIÓ-TỰ-NHIÊN`:

| Sheet | Command ID | Semantic ID | Prompt ID |
|---|---:|---|---:|
| `<0>cmd` | 12 | `0x000D` | 12 |
| `<1>wake` | 13 | `0x000D` | 13 |

Nên thống nhất dòng này thành command ID `12` và prompt ID `12` ở cả hai sheet
trước khi compile lại `cmd_info`.
