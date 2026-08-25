# Giao thức lệnh quạt qua UART0

Tài liệu này là contract giữa chip giọng nói và MCU quạt. UART0 gánh toàn bộ
bốn nhóm chức năng: lệnh điều khiển quạt, hẹn giờ tắt, âm lượng loa, và học lệnh
CWSL kèm phản hồi trạng thái.

Quy ước chung của giao thức: **request và response luôn nằm ở hai `cmd` id khác
nhau**, để một giá trị dữ liệu không bao giờ bị đọc nhầm thành câu lệnh khi hai
chiều dùng chung một dây (`4` → `5`/`6` cho CWSL, `8` → `9` cho âm lượng).
Ngoại lệ duy nhất là `CMD_VER`, phân biệt được nhờ độ dài payload.

## 1. Cấu hình UART và frame

| Thuộc tính | Giá trị |
|---|---|
| Cổng trên chip giọng nói | UART0 |
| Baud rate | 115200 |
| Cấu hình khung | 8 data bits, no parity, 1 stop bit (8N1) |
| Dữ liệu | JSON ASCII |
| Kết thúc frame | `\r\n` |

Format chung:

```text
{"cmd":<id>,"data":[<value_0>,<value_1>,...]}
```

Ví dụ bật quạt:

```text
{"cmd":1,"data":[1]}\r\n
```

Chiều MCU → chip chỉ chấp nhận `cmd` và từng phần tử `data` là số nguyên
`0..255`. MCU chỉ nên giữ một CWSL request đang chờ tại một thời điểm.

## 2. Bảng command UART0

```c
typedef enum
{
    CMD_RESET       = 0,
    CMD_TURN        = 1,
    CMD_MODE        = 2,
    CMD_VER         = 3,
    CMD_CWSL        = 4,
    CMD_STATUS      = 5,
    CMD_CWSL_INFO   = 6,
    CMD_TIMER       = 7,
    CMD_VOLUME      = 8,
    CMD_VOLUME_INFO = 9,
    CMD_ASR_UNHANDLED = 10,
    CMD_VOLUME_GET    = 11,
    CMD_SCENE         = 12,
    CMD_VOLUME_CHANGE = 13,
} command_type_t;
```

| Enum | `cmd` | `data` hợp lệ | Chiều | Hành động |
|---|---:|---|---|---|
| `CMD_RESET` | 0 | `[]` | MCU → chip | Software reset chip giọng nói |
| `CMD_TURN` | 1 | `[0]` | Chip → MCU | Tắt quạt |
| `CMD_TURN` | 1 | `[1]` | Chip → MCU | Bật quạt |
| `CMD_MODE` | 2 | `[1]` | Chip → MCU | Tốc độ số 1 |
| `CMD_MODE` | 2 | `[2]` | Chip → MCU | Tốc độ số 2 |
| `CMD_MODE` | 2 | `[3]` | Chip → MCU | Tốc độ số 3 |
| `CMD_MODE` | 2 | `[4]` | Chip → MCU | Gió tự nhiên |
| `CMD_MODE` | 2 | `[5]` | Chip → MCU | Quay quạt |
| `CMD_MODE` | 2 | `[6]` | Chip → MCU | Dừng quay |
| `CMD_VER` | 3 | `[]` | MCU → chip | Yêu cầu version firmware |
| `CMD_VER` | 3 | `[YYMMDD]` | Chip → MCU | Trả version dạng một số, ví dụ `[260821]` |
| `CMD_CWSL` | 4 | xem mục 3 | MCU → chip | Điều khiển học/xóa mẫu giọng nói |
| `CMD_STATUS` | 5 | `[code, cmd_id, slot]` | Chip → MCU | Kết quả của CWSL request, xem mục 3.3 |
| `CMD_CWSL_INFO` | 6 | xem mục 3.5 | Chip → MCU | Trả lời QUERY: slot nào đang có mẫu |
| `CMD_TIMER` | 7 | `[0]` | Chip → MCU | Hủy hẹn giờ tắt quạt |
| `CMD_TIMER` | 7 | `[1]..[12]` | Chip → MCU | Hẹn giờ tắt quạt sau 1..12 giờ |
| `CMD_VOLUME` | 8 | `[1..7]` | MCU → chip | Đặt mức âm lượng; xem chi tiết bên dưới |
| `CMD_VOLUME_INFO` | 9 | `[level, result, reserved]` | Chip → MCU | Phản hồi mức âm lượng; MCU bỏ qua `reserved` |
| `CMD_ASR_UNHANDLED` | 10 | `[asr_cmd_id]` | Chip → MCU | Chẩn đoán: ASR command không có callback firmware |
| `CMD_VOLUME_GET` | 11 | `[]` | MCU → chip | Hỏi trạng thái âm lượng hiện tại |
| `CMD_SCENE` | 12 | `[1]..[10]` | Chip → MCU | Kích hoạt ngữ cảnh tương ứng trên MCU |
| `CMD_VOLUME_CHANGE` | 13 | `[1]..[7]` | Chip → MCU | Báo mức âm lượng mới sau lệnh thoại tăng/giảm âm lượng |

Giá trị version ở chiều chip → MCU có thể lớn hơn một byte. Đây là ngoại lệ của
frame trả lời; parser chiều MCU → chip vẫn chỉ nhận từng phần tử `data` là byte.

### Hẹn giờ tắt quạt (`CMD_TIMER`)

`CMD_TIMER` chỉ đi **một chiều chip → MCU**. Chip không tự đếm giờ: nó chỉ dịch
câu lệnh thoại thành một con số và giao lại cho MCU, MCU là bên giữ đồng hồ và
thực sự tắt quạt.

| Frame | Ý nghĩa |
|---|---|
| `{"cmd":7,"data":[0]}` | `TIMER_DATA_CANCEL` — hủy hẹn giờ đang chạy |
| `{"cmd":7,"data":[N]}` | Hẹn tắt quạt sau `N` giờ, `N` thuộc `1..12` |

Mỗi mốc giờ có hai cách nói (“giờ” và “tiếng”) ứng với hai ASR command ID khác
nhau, nhưng cả hai cùng sinh ra một giá trị `N` — xem bảng ánh xạ ở mục 4.

### Kích hoạt ngữ cảnh (`CMD_SCENE`)

`CMD_SCENE` chỉ đi **một chiều chip → MCU**. Khi nhận diện câu lệnh
“KÍCH HOẠT NGỮ CẢNH N”, chip gửi index `N` để MCU tự thực thi ngữ cảnh tương
ứng. Firmware giọng nói không lưu hoặc xử lý nội dung của từng ngữ cảnh.

```text
KÍCH-HOẠT-NGỮ-CẢNH-MỘT  -> {"cmd":12,"data":[1]}
KÍCH-HOẠT-NGỮ-CẢNH-HAI  -> {"cmd":12,"data":[2]}
...
KÍCH-HOẠT-NGỮ-CẢNH-MƯỜI -> {"cmd":12,"data":[10]}
```

ASR command ID là `45 + N`, tức ID `46..55` lần lượt ánh xạ sang scene
`1..10`. Payload luôn có đúng một byte và giá trị hợp lệ là `1..10`.

### Âm lượng loa (`CMD_VOLUME` / `CMD_VOLUME_INFO`)

Âm lượng có 7 mức, `1` nhỏ nhất và `7` lớn nhất. Mức hiện tại được ghi
NVDATA/flash nên giữ nguyên sau mất điện.

| Chiều | Frame | Ý nghĩa |
|---|---|---|
| MCU → chip | `{"cmd":8,"data":[1..7]}` | Đặt mức âm lượng tuyệt đối |
| MCU → chip | `{"cmd":11,"data":[]}` | Chỉ hỏi trạng thái hiện tại |
| Chip → MCU | `{"cmd":9,"data":[level,result,reserved]}` | Phản hồi cho request `cmd:8` hoặc `cmd:11`; MCU bỏ qua `reserved` |
| Chip → MCU | `{"cmd":13,"data":[level]}` | Thông báo sau khi người dùng ra lệnh thoại “Tăng âm lượng” hoặc “Giảm âm lượng” |

`CMD_VOLUME_CHANGE` chỉ là event một chiều chip → MCU, không phải phản hồi
cho `CMD_VOLUME`. Chip gửi cả khi âm lượng đã ở biên: ví dụ, nói
“Tăng âm lượng” khi đang ở mức `7` vẫn gửi `{"cmd":13,"data":[7]}`.

`result` trong frame trả lời:

| Giá trị | Tên | Nghĩa |
|---:|---|---|
| `0` | `VOLUME_RESULT_REJECTED` | Payload không hợp lệ; trạng thái giữ nguyên |
| `1` | `VOLUME_RESULT_ACCEPTED` | Đã áp dụng request, hoặc đã trả lời query |

Giá trị `0` hoặc lớn hơn `7` trong payload `CMD_VOLUME` không hợp lệ và bị từ
chối. Query luôn dùng `CMD_VOLUME_GET` (`cmd:11`) riêng.

Hai điểm quan trọng:

- **Mỗi `cmd:8` và `cmd:11` luôn nhận đúng một `cmd:9`.** Payload sai cũng được
  trả lời với `result=0` thay vì bị nuốt im lặng, nên MCU không bao giờ phải đoán
  xem chip còn sống hay không.
- **Response nằm ở `cmd` id riêng.** `cmd:8` chỉ đi MCU → chip, `cmd:9` chỉ đi
  chip → MCU. Nhờ vậy một mức âm lượng không bao giờ bị nhầm thành câu lệnh khi
  đọc log hay khi hai chiều dùng chung một dây.

Ví dụ:

```text
MCU  -> chip:  {"cmd":8,"data":[3]}      # đặt mức 3
chip           phát beep 1000 (vì mức có thay đổi)
chip -> MCU:   {"cmd":9,"data":[3,1,0]}

MCU  -> chip:  {"cmd":8,"data":[3]}      # đặt lại đúng mức đang có
chip -> MCU:   {"cmd":9,"data":[3,1,0]}  # không beep, mức không đổi

MCU  -> chip:  {"cmd":11,"data":[]}      # query
chip -> MCU:   {"cmd":9,"data":[3,1,0]}

MCU  -> chip:  {"cmd":8,"data":[0]}      # ngoài dải
chip -> MCU:   {"cmd":9,"data":[3,0,0]}  # từ chối, mức vẫn là 3
```

Middleware `m_volume` cung cấp `m_volume_get()`, `m_volume_set()` và
`m_volume_handle_request()`. Beep voice ID `1000` chỉ phát khi mức **thực sự
thay đổi**, nên đặt lặp lại cùng một mức sẽ im lặng. Lệnh voice tăng/giảm dùng
cùng persistence này.

## 3. CWSL: học lệnh bằng giọng nói

CWSL cho phép người dùng cuối ghi thêm cách nói mới cho một lệnh có sẵn, hoặc
thêm từ đánh thức mới, mà không cần build lại model ASR. MCU điều khiển toàn bộ
qua UART0; không có khẩu lệnh thoại nào để vào/thoát chế độ học.

### 3.1. Mô hình địa chỉ: `(cmd_id, slot)`

Một mẫu đã học được định danh bằng **cặp `(cmd_id, slot)`**, ánh xạ xuống bộ ba
`(cmd_id, group_id, word_type)` của SDK — `slot` chính là `group_id`.

Hệ quả quan trọng:

- **Một `cmd_id` giữ được nhiều mẫu.** Học 3 câu khác nhau cho `cmd_id=7` thì cả
  3 câu đều bật quạt. Đây là quan hệ **1-n**, không phải 1-1.
- **Từ đánh thức không phải ca đặc biệt.** Nó chỉ là `cmd_id = WAKE_UP_ID = 1`,
  dùng chung lệnh học/xóa với command word. Firmware tự suy ra `word_type` từ
  `cmd_id`.
- **Xóa được từng mẫu một.** Vì mỗi mẫu có `slot` riêng, xóa wake word thứ 3
  không đụng tới các câu còn lại.

Sức chứa:

| Loại | `cmd_id` | Slot hợp lệ | Trần |
|---|---|---|---:|
| Từ đánh thức | `1` (`WAKE_UP_ID`) | `0..4` | 5 mẫu |
| Câu điều khiển | ID trong bảng ASR | `0..14` | 15 mẫu, **chia tự do** cho mọi lệnh |
| Tổng | | | `CICWSL_TOTAL_TEMPLATE = 20` |

15 slot command là **một pool dùng chung**, không chia đều theo lệnh và không có
trần riêng cho từng lệnh. Một lệnh có thể chiếm nhiều slot nếu người dùng muốn,
miễn tổng không vượt 15.

Số slot **không phải** là index vật lý toàn cục `0..19`. Địa chỉ thực luôn là
`(cmd_id, slot)`, vì vậy `(6,0)` và `(7,0)` là hai mẫu khác nhau và có thể cùng
tồn tại. SDK vẫn giới hạn tổng mọi command template là 15; khi tổng này đầy,
học thêm sẽ nhận `NO_SLOT` dù command ID mới chưa dùng slot nào.

### 3.2. Action MCU gửi (`cmd:4`)

```c
typedef enum
{
    CWSL_ACTION_LEARN  = 1, /* data=[1, cmd_id, slot] */
    CWSL_ACTION_DELETE = 2, /* data=[2, cmd_id, slot] */
    CWSL_ACTION_STOP   = 4, /* data=[4]               */
    CWSL_ACTION_QUERY  = 6, /* data=[6, cmd_id]       */
} cwsl_action_t;

#define CWSL_ANY  255U   /* 0xFF: giá trị đặc biệt, không phải slot thật */
```

| Request | Ý nghĩa |
|---|---|
| `{"cmd":4,"data":[1,<cmd_id>,<slot>]}` | Học mẫu vào đúng slot đó, **đè** nếu slot đã có |
| `{"cmd":4,"data":[1,<cmd_id>,255]}` | Học vào slot trống thấp nhất của `cmd_id` |
| `{"cmd":4,"data":[1,1,255]}` | Học thêm một từ đánh thức |
| `{"cmd":4,"data":[2,<cmd_id>,<slot>]}` | Xóa đúng một mẫu |
| `{"cmd":4,"data":[2,<cmd_id>,255]}` | Xóa mọi mẫu của `cmd_id` đó |
| `{"cmd":4,"data":[2,255,255]}` | Xóa toàn bộ mẫu đã học (wake + command) |
| `{"cmd":4,"data":[4]}` | Dừng phiên học đang chạy |
| `{"cmd":4,"data":[6,<cmd_id>]}` | Hỏi slot nào của `cmd_id` đang có mẫu |
| `{"cmd":4,"data":[6,255]}` | Hỏi tổng quan sức chứa |

`slot` là một byte (8 bit), nhưng giá trị slot hợp lệ chỉ là `0..4` cho wake
word và `0..14` cho command word. `255` (`0xFF`) được dành làm giá trị đặc biệt:
với `LEARN` là tự chọn slot; với `DELETE` là mọi slot; với `cmd_id=255` trong
DELETE là mọi command ID.

Không còn action "vào chế độ học". Một request `LEARN` đi thẳng vào việc thu mẫu.

### 3.3. Status trả về (`cmd:5`)

Mọi request CWSL kiểu `LEARN`, `DELETE` hoặc `STOP` sinh ra **đúng một** frame
status, kể cả khi request bị từ chối trước khi chạm SDK. Dạng frame là
`{"cmd":5,"data":[code, cmd_id, slot]}`. Hai byte sau lặp lại địa chỉ của
request/phiên đang chạy để MCU ghép response. `QUERY` là ngoại lệ: nó chỉ trả
`CMD_CWSL_INFO`, không trả `CMD_STATUS`.

```c
typedef enum
{
    CWSL_ST_NONE              = 0x00,

    /* Kết thúc thành công */
    CWSL_ST_LEARN_OK          = 0x20, /* đã lưu mẫu; prompt 211 */
    CWSL_ST_DELETE_OK         = 0x21, /* đã xóa; prompt 213 */
    CWSL_ST_EXIT_OK           = 0x22, /* đã thoát học lệnh; prompt 217 */

    /* Kết thúc thất bại */
    CWSL_ST_LEARN_FAIL        = 0x30, /* hết số lần thử; prompt 218/1021 */
    CWSL_ST_LEARN_ABORTED     = 0x31, /* SDK abort giữa chừng */
    CWSL_ST_NO_SLOT           = 0x32, /* hết slot; prompt 212 */
    CWSL_ST_DELETE_FAIL       = 0x33, /* không xóa được; prompt 214 */
    CWSL_ST_TIMEOUT           = 0x34, /* 30 s không tiến triển */

    /* Bị từ chối, chưa chạm SDK */
    CWSL_ST_BAD_REQUEST       = 0x40, /* payload sai hoặc action lạ */
    CWSL_ST_BAD_COMMAND_ID    = 0x41, /* cmd_id không có trong bảng ASR */
    CWSL_ST_BAD_SLOT          = 0x42, /* slot vượt trần của loại đó */
    CWSL_ST_BUSY              = 0x43, /* đang có thao tác CWSL chạy dở */
    CWSL_ST_NOT_IN_LEARN_MODE = 0x44, /* gửi STOP khi không đang học */
} cwsl_status_t;
```

Nibble cao là **lớp kết quả**, nên MCU không quan tâm chi tiết chỉ cần đọc
`code >> 4`:

| `code >> 4` | Nghĩa |
|---:|---|
| `2` | Xong, thành công |
| `3` | Xong, thất bại |
| `4` | Request bị từ chối |

Firmware **không** gửi status tiến trình trung gian. Các prompt 1007/1008/1009
vẫn phát cho người dùng nghe khi phải nói lại, nhưng không sinh frame UART.

### 3.4. Flow học và xóa

```text
# Học thêm một cách nói cho lệnh "bật quạt" (ID 7), tự chọn slot
MCU  -> chip:  {"cmd":4,"data":[1,7,255]}
chip           phát prompt 1010 "mời nói câu lệnh mới"
người dùng     nói câu cần học
chip           phát prompt 211 nếu thành công
chip -> MCU:   {"cmd":5,"data":[32,7,0]}      # 0x20 LEARN_OK, đã vào slot 0

# Học từ đánh thức
MCU  -> chip:  {"cmd":4,"data":[1,1,255]}
chip           phát prompt 1020, thu, rồi phát 211
chip -> MCU:   {"cmd":5,"data":[32,1,2]}      # đã vào wake slot 2

# Xóa đúng wake slot 2, các wake khác giữ nguyên
MCU  -> chip:  {"cmd":4,"data":[2,1,2]}
chip           phát prompt 213
chip -> MCU:   {"cmd":5,"data":[33,1,2]}      # 0x21 DELETE_OK

# Học thất bại vì hết slot
MCU  -> chip:  {"cmd":4,"data":[1,7,255]}
chip           phát prompt 212
chip -> MCU:   {"cmd":5,"data":[50,7,255]}    # 0x32 NO_SLOT
```

Sau **mọi** kết quả cuối, chip tự thoát chế độ học và về wake-only im lặng
(`APP_ST_IDLE`). Không cần gửi STOP sau mỗi lần học; STOP chỉ dùng để hủy một
phiên đang chạy dở:

```text
MCU  -> chip:  {"cmd":4,"data":[1,7,255]}
chip           phát prompt 1010, đang chờ người dùng nói
MCU  -> chip:  {"cmd":4,"data":[4]}           # đổi ý, hủy
chip           phát prompt 217
chip -> MCU:   {"cmd":5,"data":[34,7,0]}      # 0x22 EXIT_OK; target của phiên bị hủy
```

Trong lúc một thao tác đang chạy, mọi request CWSL khác bị từ chối bằng
`0x43 BUSY`. Gửi STOP khi không đang học nhận `0x44 NOT_IN_LEARN_MODE`.

Timeout CWSL là 30 giây không có tiến triển (`CWSL_IDLE_TIMEOUT_MS`), tách khỏi
timeout service/wake 15 giây. Bộ đếm được gia hạn khi SDK bắt đầu đăng ký mẫu và
sau mỗi lần thu kết thúc. Hết giờ thì abort, trả `0x34 TIMEOUT` và về wake-only.

### 3.5. Query sức chứa (`cmd:6`)

```text
# Lệnh 7 đang có mẫu ở slot nào
MCU  -> chip:  {"cmd":4,"data":[6,7]}
chip -> MCU:   {"cmd":6,"data":[7,5,0]}       # bitmap 0x0005 -> slot 0 và 2

# Wake word
MCU  -> chip:  {"cmd":4,"data":[6,1]}
chip -> MCU:   {"cmd":6,"data":[1,3,0]}       # bitmap 0x0003 -> wake slot 0, 1

# Tổng quan
MCU  -> chip:  {"cmd":4,"data":[6,255]}
chip -> MCU:   {"cmd":6,"data":[255,2,6,12]}  # 2 wake, 6 command, 12 slot trống
```

- Query một `cmd_id`: `data = [cmd_id, bitmap_low, bitmap_high]`, bitmap 16 bit
  little-endian, bit N bật nghĩa là slot N đang có mẫu. Wake chỉ dùng bit `0..4`,
  command chỉ dùng bit `0..14`; các bit còn lại luôn bằng 0.
- Query tổng quan (`cmd_id = 255`): `data = [255, wake_used, command_used, free]`.

Query là thao tác đọc thuần, trả lời được ở mọi trạng thái kể cả khi đang bận
học. Nó không sinh frame `CMD_STATUS`.

### 3.6. Điều kiện học

- Học mới chỉ thêm **cách nói**, không tạo chức năng mới. Học cho `cmd_id=7` thì
  câu mới vẫn chạy đúng hành động bật quạt.
- Firmware hiện kiểm tra `cmd_id` có tồn tại trong bảng ASR, không kiểm tra nó có
  action quạt hay không. Vì vậy một ID như `6` có thể học/xóa/query thành công nếu
  nó tồn tại trong `cmd_info`, nhưng khi nhận diện thì `cmd_dispatch()` không map
  ID đó sang UART fan command. Với product hiện tại, MCU chỉ nên học command ID
  `7..14`, timer ID `15..39`, volume ID `40..43` hoặc scene ID `46..55`; ID `1`
  dành cho learned wake word.
- Câu học không được trùng command có sẵn trong model. Trong lúc thu, firmware
  chuyển mọi command mặc định nhận được vào `cwsl_reg_check_other_asrcmd()` và
  `send_nn_end_msg_to_cwsl()`. SDK trả `CWSL_RECORD_FAILED_BY_DEFAULTCMD`, chip
  phát prompt 1009 và mời nói câu khác.
- Mẫu chỉ được ghi sau `CWSL_REG_FINISHED`. Too-short, conflict, invalid, abort
  hoặc quá số lần thử đều không chiếm slot.
- Học đè lên slot đã có mẫu thì mẫu cũ bị **xóa trước**, rồi mới thu mẫu mới.
  Nếu lần thu đó thất bại, câu cũ đã mất và MCU nhận `0x30 LEARN_FAIL`.

Prompt phát trong quá trình học:

| Voice ID | Nội dung / trường hợp |
|---:|---|
| 211 | Học lệnh thành công |
| 212 | Bộ nhớ học lệnh đã đầy |
| 213 / 214 | Xóa thành công / thất bại |
| 217 | Đã kết thúc chế độ học lệnh |
| 218 | Học lệnh thất bại |
| 1007 | Chưa nghe rõ, nói lại |
| 1008 | Câu lệnh quá ngắn |
| 1009 | Câu lệnh bị trùng với lệnh mặc định |
| 1010 | Mời nói câu lệnh điều khiển mới |
| 1020 | Mời nói từ đánh thức mới |
| 1021 | Không thể học từ đánh thức |

Voice 209 ("đã vào chế độ học lệnh") không còn được dùng vì không còn bước Enter.

### 3.7. FSM toàn hệ thống

Application là nơi duy nhất được phép chạy action điều khiển quạt.

| State | Quyền xử lý |
|---|---|
| `APP_ST_IDLE` | Chỉ nhận wake word hoặc request CWSL từ MCU |
| `APP_ST_SERVING` | Nhận wake/command điều khiển quạt, hoặc request CWSL |
| `APP_ST_CWSL_BUSY` | Một thao tác CWSL độc chiếm engine; không phục vụ lệnh quạt |

Một request `LEARN`/`DELETE` đi thẳng từ `IDLE` hoặc `SERVING` vào
`APP_ST_CWSL_BUSY`, chạy tới đúng một kết quả, trả status rồi về `APP_ST_IDLE`.

Trong `APP_ST_CWSL_BUSY` không tồn tại transition cho `EV_WAKE`/`EV_COMMAND`, nên
kết quả nhận dạng không thể gọi `cmd_dispatch()`. Tuy vậy command mặc định vẫn
được chuyển nội bộ vào `cwsl_reg_check_other_asrcmd()` để SDK phát hiện trùng;
nhờ đó câu như "tắt quạt" không bị lưu thành câu học mới.

## 4. Ánh xạ lệnh thoại mặc định sang MCU

| Câu lệnh | ASR command ID | Message gửi sang MCU | Ý nghĩa |
|---|---:|---|---|
| `BẬT-QUẠT` | 7 | `{"cmd":1,"data":[1]}` | Bật quạt |
| `TẮT-QUẠT` | 8 | `{"cmd":1,"data":[0]}` | Tắt quạt |
| `BẬT-SỐ-MỘT` | 9 | `{"cmd":2,"data":[1]}` | Tốc độ số 1 |
| `BẬT-SỐ-HAI` | 10 | `{"cmd":2,"data":[2]}` | Tốc độ số 2 |
| `BẬT-SỐ-BA` | 11 | `{"cmd":2,"data":[3]}` | Tốc độ số 3 |
| `CHẾ-ĐỘ-GIÓ-TỰ-NHIÊN` | 12 | `{"cmd":2,"data":[4]}` | Gió tự nhiên |
| `BẬT-CHẾ-ĐỘ-QUAY` | 13 | `{"cmd":2,"data":[5]}` | Quay quạt |
| `DỪNG-QUAY` | 14 | `{"cmd":2,"data":[6]}` | Dừng quay |
| `TẮT-QUẠT-SAU-<N>-GIỜ` / `...-TIẾNG` | 15..38 | `{"cmd":7,"data":[N]}` | Hẹn tắt sau N giờ (`N=1..12`) |
| `HỦY-HẸN-GIỜ` | 39 | `{"cmd":7,"data":[0]}` | Hủy hẹn giờ |
| `GIẢM-ÂM-LƯỢNG` | 40 | Chip-local | Giảm một mức, clamp tại 1 |
| `TĂNG-ÂM-LƯỢNG` | 41 | Chip-local | Tăng một mức, clamp tại 7 |
| `ÂM-LƯỢNG-NHỎ-NHẤT` | 42 | Chip-local | Đặt mức 1 |
| `ÂM-LƯỢNG-LỚN-NHẤT` | 43 | Chip-local | Đặt mức 7 |
| `KÍCH-HOẠT-NGỮ-CẢNH-<N>` | 46..55 | `{"cmd":12,"data":[N]}` | Kích hoạt ngữ cảnh `N` (`N=1..10`) |

Khi một command ID học được nhận diện, nó đi lại đúng pipeline ASR mặc định,
do đó message UART gửi sang MCU giống với câu lệnh mặc định mà nó trỏ tới.

## 5. Smoke test bằng web GUI

Tool test nằm tại `vendor/tools/cwsl_uart_web.py`. Tool chỉ bind local host, hiển thị
COM port, gửi đúng JSON + CRLF và hiển thị đồng thời TX/RX.

Chuẩn bị và chạy từ thư mục gốc project:

```powershell
py -m pip install pyserial
py .\vendor\tools\cwsl_uart_web.py
```

Trình duyệt sẽ mở `http://127.0.0.1:8765/`. Chọn COM đang nối UART0, giữ baud
`115200`, nhấn **Connect**, sau đó test theo thứ tự:

1. Nhấn **Query capacity** để xem sức chứa hiện tại.
2. Nhập Command ID (ví dụ `7`), để Slot `255`, nhấn **Learn**.
3. Nói sau prompt 1010 và đợi frame `cmd:5`. Tool tự giải mã ra tên status,
   ví dụ `<- LEARN_OK (0x20, finished ok)  cmd_id=7 slot=0`.
4. Nói lại câu vừa học để xác nhận chip phát ra command UART tương ứng.
5. Nhấn **Query this ID** để xác nhận slot đã được đánh dấu đang dùng.
6. Lặp bước 2 với câu khác để kiểm tra 1-n: cùng `cmd_id=7` phải vào slot kế
   tiếp, và cả hai câu đều điều khiển được quạt.
7. Xóa lẻ: đặt Slot đúng số vừa học rồi nhấn **Delete**, kiểm tra bằng
   **Query this ID** rằng các slot khác còn nguyên.

Với wake word, dùng khu **Wake word shortcuts**: học nhiều câu đánh thức rồi
xóa đúng một slot để xác nhận các câu còn lại vẫn đánh thức được.

Kiểm tra âm lượng ở khu **Speaker volume**:

1. Nhấn **Query level** → phải nhận `cmd:9`, tool giải mã thành ví dụ
   `<- volume level 7 (accepted)`.
2. Nhấn **3** → nghe beep một tiếng, nhận `{"cmd":9,"data":[3,1,0]}`.
3. Gửi custom `cmd 8` với data `0` → nhận `[3,0,<reserved>]`, tức bị từ chối và
   trạng thái giữ nguyên. Đây là cách xác nhận chip không nuốt im lặng request sai.

Lưu ý: nạp lại full package thường xóa NVDATA, nên âm lượng quay về
`VOLUME_DEFAULT = 7` sau mỗi lần nạp. Nếu vừa nạp xong mà thấy mức khác với lúc
trước, đó là lý do. Sau khi nạp, chờ chip boot xong (phát prompt khởi động) rồi
mới gửi lệnh — frame gửi trong lúc chip còn đang boot sẽ mất vì UART RX chỉ được
bật ở cuối `app_init()`.

Nếu danh sách COM trống hoặc mở cổng thất bại, đóng Serial Monitor/tool khác
đang giữ cổng rồi nhấn **Refresh**. Có thể đổi web port bằng
`py .\vendor\tools\cwsl_uart_web.py --port 9000`.

## 6. Lưu ý workbook

Hai sheet cần dùng cùng command ID/prompt ID cho `CHẾ-ĐỘ-GIÓ-TỰ-NHIÊN`:

| Sheet | Command ID | Semantic ID | Prompt ID |
|---|---:|---|---:|
| `<0>cmd` | 12 | `0x000D` | 12 |
| `<1>wake` hiện tại | 13 | `0x000D` | 13 |

Trước khi compile lại `cmd_info`, sửa dòng `<1>wake` thành command ID `12` và
prompt ID `12` để đồng nhất với `<0>cmd`.