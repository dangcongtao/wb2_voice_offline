# Giao thức lệnh qua UART0 (module giọng nói nhà thông minh)

Tài liệu này là contract giữa chip giọng nói CI13162 và MCU. UART0 gánh toàn bộ:
lệnh thoại điều khiển nhà (ngữ cảnh, đèn, công tắc, thiết bị), âm lượng loa,
câu trả lời tùy chọn cho từng lệnh, và học lệnh CWSL kèm phản hồi trạng thái.

Quy ước chung: **request và response nằm ở hai `cmd` id khác nhau** (`4` → `5`/`6`
cho CWSL, `8`/`11` → `9` cho âm lượng), để một giá trị dữ liệu không bao giờ bị đọc
nhầm thành câu lệnh khi hai chiều dùng chung một dây. Ngoại lệ: `CMD_VER`,
`CMD_RESP_SET`, `CMD_RESP_GET` dùng cùng id hai chiều, phân biệt bằng chiều gửi.

## 1. Cấu hình UART và frame

| Thuộc tính | Giá trị |
|---|---|
| Cổng trên chip giọng nói | UART0 |
| Baud rate | 115200 |
| Cấu hình khung | 8N1 |
| Dữ liệu | JSON ASCII |
| Kết thúc frame | `\r\n` |

```text
{"cmd":<id>,"data":[<value_0>,<value_1>,...]}
```

Mọi frame **chip → MCU** do một **câu điều khiển** kích hoạt có thêm hai key:

```text
{"cmd":12,"data":[72],"cmd_id":72,"cwsl":255}\r\n
{"cmd":12,"data":[72],"cmd_id":72,"cwsl":0}\r\n
```

| Key | Ý nghĩa |
|---|---|
| `"cmd_id"` | `cmd_id_t` trong sheet `<0>cmd` (lệnh vừa nhận, ví dụ 72 = bật đèn phòng khách) |
| `"cwsl"` | `255` nếu nói câu gốc; `0..14` nếu nói mẫu CWSL đã học cho **đúng** `cmd_id` đó |

- **Từ đánh thức không gửi UART.** Wake mặc định và wake đã học chỉ đánh thức chip.
- Frame trả lời (version, volume info, CWSL status/query, resp, debug) không có hai key này.
- Chiều MCU → chip chỉ cần `cmd` và `data`; mỗi phần tử `data` là số nguyên `0..255`,
  tối đa **24 phần tử**, cả dòng ≤ 128 byte. Parser bỏ qua field lạ, từ chối số thập phân.
- MCU chỉ nên giữ một CWSL request đang chờ tại một thời điểm.
- Frame gửi lúc chip đang khởi động (trước prompt khởi động) có thể mất.

## 2. Bảng command UART0

```c
typedef enum
{
    CMD_RESET         = 0,
    /* 1 CMD_TURN, 2 CMD_MODE, 7 CMD_TIMER, 14 CMD_STATE: quạt, đã bỏ */
    CMD_VER           = 3,
    CMD_CWSL          = 4,
    CMD_STATUS        = 5,
    CMD_CWSL_INFO     = 6,
    CMD_VOLUME        = 8,
    CMD_VOLUME_INFO   = 9,
    CMD_ASR_UNHANDLED = 10,
    CMD_VOLUME_GET    = 11,
    CMD_SCENE         = 12,
    CMD_VOLUME_CHANGE = 13,
    CMD_RESP_SET      = 15,
    CMD_RESP_GET      = 16,
    CMD_VOICE_SET     = 17,
    CMD_VOICE_GET     = 18,
    CMD_BEEP          = 200,
    CMD_CWSL_DEBUG    = 201,   /* DEBUG_ENABLE 1 + CWSL_TRACE_ENABLE 1 */
    CMD_ASR_LOG       = 202,   /* DEBUG_ENABLE 1 */
    CMD_MIC_HPF       = 203,   /* DEBUG_ENABLE 1 + MIC_LOG_ENABLE 1 */
    CMD_PLAY_RESP     = 204,   /* DEBUG_ENABLE 1 */
} command_type_t;
```

Nguồn: `user/application/commands.h`.

| Enum | `cmd` | `data` | Chiều | Hành động |
|---|---:|---|---|---|
| `CMD_RESET` | 0 | `[]` | MCU → chip | Software reset chip giọng nói |
| `CMD_VER` | 3 | `[]` | MCU → chip | Hỏi version firmware |
| `CMD_VER` | 3 | `[YYMMDD]` | Chip → MCU | Version dạng một số, ví dụ `[261102]` |
| `CMD_CWSL` | 4 | xem mục 4 | MCU → chip | Học / xóa / dừng / hỏi mẫu giọng nói |
| `CMD_STATUS` | 5 | `[code, cmd_id, slot]` | Chip → MCU | Kết quả CWSL request (mục 4.3) |
| `CMD_CWSL_INFO` | 6 | xem mục 4.5 | Chip → MCU | Trả lời QUERY |
| `CMD_VOLUME` | 8 | `[1..7]` / `[8]` / `[9]` | MCU → chip | Đặt mức / tắt tiếng / bật tiếng (mục 2.2) |
| `CMD_VOLUME_INFO` | 9 | `[level, result, muted]` | Chip → MCU | Trả lời mọi `cmd:8` và `cmd:11` |
| `CMD_ASR_UNHANDLED` | 10 | `[cmd_id]` | Chip → MCU | Chẩn đoán: lệnh thoại không có hàng trong `s_frames` |
| `CMD_VOLUME_GET` | 11 | `[]` | MCU → chip | Hỏi trạng thái âm lượng |
| `CMD_SCENE` | 12 | `[1..15]` / `[71..103]` | Chip → MCU | Ngữ cảnh 1..15, hoặc lệnh nhà theo `cmd_id` (mục 2.1) |
| `CMD_VOLUME_CHANGE` | 13 | `[0..7]` | Chip → MCU | Sau lệnh thoại âm lượng: `0` đang tắt tiếng, `1..7` mức hiện tại |
| `CMD_RESP_SET` | 15 | `[cmd_id, value]…` | MCU → chip | Chọn câu trả lời cho từng lệnh (mục 3), tối đa 12 cặp |
| `CMD_RESP_SET` | 15 | `[1]` / `[0]` | Chip → MCU | Đã áp / từ chối, không áp cặp nào |
| `CMD_RESP_GET` | 16 | `[cmd_id, …]` | MCU → chip | Hỏi câu trả lời đang chọn |
| `CMD_RESP_GET` | 16 | `[cmd_id, value]…` | Chip → MCU | Trả lời theo thứ tự hỏi |
| `CMD_VOICE_SET` | 17 | `[1..4]` | MCU → chip | Chọn giọng (mục 3.2); chip phát câu welcome của giọng mới |
| `CMD_VOICE_SET` | 17 | `[1]` / `[0]` | Chip → MCU | Đã đổi / từ chối (data không phải đúng 1 số trong 1..4), giọng giữ nguyên |
| `CMD_VOICE_GET` | 18 | `[]` | MCU → chip | Hỏi giọng đang chọn |
| `CMD_VOICE_GET` | 18 | `[1..4]` | Chip → MCU | Giọng đang chọn |
| `CMD_BEEP` | 200 | `[]` / `[0]` / `[1]` | MCU → chip | Beep (mục 2.3) |
| `CMD_CWSL_DEBUG` | 201 | `[stage, id, v0, v1]` | Chip → MCU | Trace học lệnh; xem [CWSL guide](cwsl_guide.md#trace-khi-kiểm-tra-học-wake-word) |
| `CMD_ASR_LOG` | 202 | `[]` + `"log":"<text>"` | Chip → MCU | Log SDK (VAD, điểm decoder, kết quả ASR, điểm mẫu học; mức mic khi bật `MIC_LOG_ENABLE`). MCU bỏ qua |
| `CMD_MIC_HPF` | 203 | `[i]` | MCU → chip | Đổi HPF codec lúc chạy, `i` 0..3 = 1/20/245/612 Hz; chip trả `[i_set, i_reg]` |
| `CMD_PLAY_RESP` | 204 | `[resp_id]` | MCU → chip | Phát thử một câu trả lời (mục 3.1) bằng giọng đang chọn, tính lại thời gian thức. Không có frame trả lời |

`CMD_VER` chiều chip → MCU có thể lớn hơn một byte; đây là ngoại lệ của frame trả lời.
Các `cmd` 201..204 chỉ có trong bản build `DEBUG_ENABLE 1`, tắt khi ra sản phẩm. 201 cần thêm
`CWSL_TRACE_ENABLE 1`, 203 cần thêm `MIC_LOG_ENABLE 1` (cả hai mặc định 0 để bản debug vừa
flash), xem [app_config.md](../app_config.md#log-debug-debug_enable-1).

### 2.1. Lệnh thoại → frame MCU (`CMD_SCENE`)

Chip không lưu trạng thái thiết bị, chỉ dịch câu nói thành một con số rồi giao cho
MCU. Bảng nguồn: `s_frames` trong `user/application/main_task/main_cmd.c`.

| Câu lệnh | `cmd_id` | Frame (câu gốc) |
|---|---:|---|
| `KÍCH-HOẠT-NGỮ-CẢNH-MỘT` … `MƯỜI-LĂM` | 56..70 | `{"cmd":12,"data":[1..15],"cmd_id":56..70,"cwsl":255}` |
| Lệnh nhà (đèn, điều hòa, quạt, ti vi, rèm, tất cả thiết bị, chế độ) | 71..103 | `{"cmd":12,"data":[<cmd_id>],"cmd_id":<cmd_id>,"cwsl":255}` |

Với lệnh nhà, `data[0]` chính là `cmd_id`; MCU tự ánh xạ ID sang thiết bị. Danh sách
33 lệnh nhà kèm ID trong `user/application/commands.h`.
Lệnh âm lượng 48..55 gửi `CMD_VOLUME_CHANGE` (mục 2.2). Từ đánh thức 1..6 không gửi.

> Model 2026-11-01 (V01972) còn 62 lệnh, lệnh nhà đánh lại 71..103 (bỏ công tắc, ổ cắm,
> bình nóng lạnh, điều hòa chế độ ngủ, ngữ cảnh MƯỜI-NĂM). Ví dụ BẬT-QUẠT là 87.
> MCU phải cập nhật bảng ánh xạ theo ID mới.

### 2.2. Âm lượng loa (`CMD_VOLUME` / `CMD_VOLUME_INFO`)

7 mức, `1` nhỏ nhất, `7` lớn nhất, mặc định `4` (`VOLUME_DEFAULT`). Mức và trạng thái
tắt tiếng lưu flash, còn sau mất điện.

| Chiều | Frame | Ý nghĩa |
|---|---|---|
| MCU → chip | `{"cmd":8,"data":[1..7]}` | Đặt mức (tự bật tiếng) |
| MCU → chip | `{"cmd":8,"data":[8]}` | Tắt tiếng |
| MCU → chip | `{"cmd":8,"data":[9]}` | Bật tiếng |
| MCU → chip | `{"cmd":11,"data":[]}` | Chỉ hỏi trạng thái |
| Chip → MCU | `{"cmd":9,"data":[level,result,muted]}` | Trả lời **mọi** `cmd:8` / `cmd:11`; `muted` 1 = đang tắt tiếng |
| Chip → MCU | `{"cmd":13,"data":[state]}` | Event sau lệnh thoại âm lượng; `0` tắt tiếng, `1..7` mức |

`result`: `1` đã áp hoặc đã trả lời query, `0` payload sai, trạng thái giữ nguyên.

```text
MCU  -> chip:  {"cmd":8,"data":[3]}      # đặt mức 3
chip           beep 1000 (chỉ khi mức thật sự đổi)
chip -> MCU:   {"cmd":9,"data":[3,1,0]}

MCU  -> chip:  {"cmd":8,"data":[8]}      # tắt tiếng
chip -> MCU:   {"cmd":9,"data":[3,1,1]}

MCU  -> chip:  {"cmd":8,"data":[0]}      # ngoài dải
chip -> MCU:   {"cmd":9,"data":[3,0,1]}  # từ chối, giữ nguyên
```

Lệnh thoại: tăng/giảm một mức (clamp 1..7), bé/nhỏ nhất = 1, to/lớn nhất = 7, tắt âm
thanh (tắt tiếng **sau** khi phát xong câu xác nhận), bật âm thanh. Đang tắt tiếng mà
tăng/giảm/đặt mức thì chip tự bật tiếng. `CMD_VOLUME_CHANGE` gửi cả khi đã ở biên
(nói "tăng" ở mức 7 vẫn gửi `[7]`). Code: `main_cmd_volume()` trong `main_cmd.c`.

### 2.3. Beep (`CMD_BEEP`)

Một chiều MCU → chip, không có frame trả lời, phát cả khi đang tắt tiếng.

| `data` | Voice ID | File |
|---|---:|---|
| `[]` hoặc `[0]` | 1000 | `[1000]beep.mp3` |
| `[1]` | 1001 | `[1001]BEEP-LONG` (trước là 220, đổi để không trùng vùng file của giọng 3) |

Payload khác bị bỏ qua. Định dạng clip: [voice_mp3_format.md](voice_mp3_format.md).

## 3. Câu trả lời tùy chọn (`CMD_RESP_SET` / `CMD_RESP_GET`)

Mọi câu chip nói là một **response id 1..99** (bảng `resp_id_t` trong
`user/application/response.h`, kèm bảng câu của từng giọng). Mỗi lệnh có một hàng trong
`s_rows` (`main_resp.c`): `resp[0]` là câu mặc định, cả hàng là các câu cho chế độ ngẫu
nhiên. Mỗi lệnh lưu một `value` 1 byte:

| `value` | Nghĩa |
|---:|---|
| `0` | **Ngẫu nhiên** trong hàng của lệnh (giá trị khi chưa set), không lặp câu vừa phát. Wake: câu 1, 2, 3 |
| `1` | Câu mặc định `resp[0]` của lệnh. Mỗi wake word (1..6) có value riêng, muốn đổi cả 6 thì gửi 6 cặp trong một khung |
| `2..99` | Đúng response id đó, **lệnh nào cũng set được**, miễn có ít nhất một giọng thu câu đó |

| Lệnh | Mặc định | Thêm khi ngẫu nhiên |
|---|---|---|
| Wake 1..6 | 1 `RESP_WAKE_1` | 2, 3 |
| Âm lượng 48..55 | 40..45 theo lệnh | — |
| Ngữ cảnh 56..70 | 46 `RESP_SCENE` | 47 `RESP_DONE` |
| Bật / tắt đèn (71..82, 102, 103) | 50 / 51 | 47 |
| Bật / tắt tất cả đèn (83, 84) | 50 / 51 | 60 / 61, 47 |
| Điều hòa, quạt, ti vi, rèm, tất cả thiết bị (85..94) | 52..61 | 47 |
| Chế độ (95..101) | 62..68 | 47 |

```text
MCU → chip  {"cmd":15,"data":[1,3, 71,0, 72,1]}   # wake 1 → câu 3, 71 ngẫu nhiên, 72 mặc định
chip → MCU  {"cmd":15,"data":[1]}
MCU → chip  {"cmd":15,"data":[1,1, 2,1, 3,1, 4,1, 5,1, 6,1]}   # cả 6 wake word → câu mặc định
chip → MCU  {"cmd":15,"data":[1]}
MCU → chip  {"cmd":15,"data":[89,50]}              # bật ti vi trả lời bằng câu "đèn sáng"
chip → MCU  {"cmd":15,"data":[1]}
MCU → chip  {"cmd":15,"data":[71,9]}               # không giọng nào có câu 9
chip → MCU  {"cmd":15,"data":[0]}
MCU → chip  {"cmd":16,"data":[1,71,72]}
chip → MCU  {"cmd":16,"data":[1,4,71,0,72,1]}
```

- **SET là tất cả hoặc không**: độ dài chẵn, tối đa 12 cặp; sai một cặp thì không áp cặp nào, trả `[0]`.
- **GET** trả `[cmd_id, value]` theo thứ tự hỏi, 12 cặp mỗi frame; `cmd_id` không có trả `255`.
- Câu học (CWSL) dùng chung `value` với lệnh gốc cùng `cmd_id`.
- Lưu RAM 128 B + 1 item NV 128 B (`NVDATA_ID_USER_START + 8`), còn sau khi tắt nguồn.
  Value không còn hợp lệ sau khi đổi voice.bin (không giọng nào có câu đó) tự về `0` lúc khởi động.
- Đang tắt tiếng thì không phát.
- Câu trả lời và giọng độc lập: value chọn **câu nào**, giọng (mục 3.2) chọn **ai nói**.

### 3.1. Danh sách response id

Nguồn: `resp_id_t` trong `user/application/response.h`. Cả 4 giọng thu đủ 41 câu dưới đây,
cùng id, cùng ý (lời từng giọng xem `response.h` hoặc web tool). Dùng làm `value` của
`CMD_RESP_SET` / CWSL learn và `data` của `CMD_PLAY_RESP`.

| Id | Tên | Câu dùng cho |
|---:|---|---|
| 1, 2, 3 | `RESP_WAKE_1..3` | Trả lời wake word (1 là mặc định) |
| 11 | `RESP_LEARN_START` | Bắt đầu học lệnh / học wake: mời nói |
| 12 | `RESP_LEARN_AGAIN` | Lượt thu hỏng, nói lại |
| 13 | `RESP_LEARN_TOO_SHORT` | Câu quá ngắn |
| 14 | `RESP_LEARN_DUPLICATE` | Trùng lệnh / câu đã học |
| 15 | `RESP_LEARN_OK` | Học thành công |
| 16 | `RESP_LEARN_FAIL` | Học thất bại, hết giờ |
| 17 | `RESP_LEARN_FULL` | Hết chỗ học |
| 18 | `RESP_DELETE_FAIL` | Xóa thất bại |
| 19 | `RESP_LEARN_EXIT` | MCU dừng học |
| 40, 41 | `RESP_VOLUME_DOWN`, `_UP` | Giảm / tăng âm lượng |
| 42, 43 | `RESP_VOLUME_MIN`, `_MAX` | Âm lượng nhỏ nhất / lớn nhất |
| 44, 45 | `RESP_MUTE`, `RESP_UNMUTE` | Tắt / bật âm thanh |
| 46 | `RESP_SCENE` | Đã kích hoạt ngữ cảnh |
| 47 | `RESP_DONE` | Xong việc (dùng chung, random cho mọi lệnh) |
| 50, 51 | `RESP_LIGHT_ON`, `_OFF` | Bật / tắt đèn |
| 52, 53 | `RESP_AC_ON`, `_OFF` | Bật / tắt điều hòa |
| 54, 55 | `RESP_FAN_ON`, `_OFF` | Bật / tắt quạt |
| 56, 57 | `RESP_TV_ON`, `_OFF` | Bật / tắt ti vi |
| 58, 59 | `RESP_CURTAIN_OPEN`, `_CLOSE` | Mở / đóng rèm |
| 60, 61 | `RESP_ALL_ON`, `_OFF` | Bật / tắt tất cả |
| 62..68 | `RESP_MODE_SLEEP` … `RESP_MODE_GUEST` | Chế độ đi ngủ, vắng nhà, nghỉ ngơi, về nhà, thức dậy, xem phim, tiếp khách |
| 70 | `RESP_WELCOME` | Khởi động, sau `CMD_VOICE_SET` |
| 71 | `RESP_SLEEP` | Hết giờ thức, về chờ wake |

Id không có trong bảng (4..10, 20..39, 48, 49, 69, 72..99) bị `CMD_RESP_SET` từ chối.
Id 1 không chọn đích danh được vì `value 1` nghĩa là "mặc định" (với wake thì chính là câu 1).
Beep 1000 / 1001 dùng `CMD_BEEP`.

### 3.2. Giọng (`CMD_VOICE_SET` / `CMD_VOICE_GET`)

| Giọng | Tên | File voice |
|---:|---|---|
| 1 | Nam bắc (mặc định khi NV trống) | `[1..99]` |
| 2 | Nữ bắc | `[101..199]` |
| 3 | Nam miền Nam | `[201..299]` |
| 4 | Nữ miền Nam | `[301..399]` |

| Chiều | Frame | Ý nghĩa |
|---|---|---|
| MCU → chip | `{"cmd":17,"data":[v]}`, `v` 1..4 | Chọn giọng, lưu NV, phát câu welcome (70) của giọng `v` |
| Chip → MCU | `{"cmd":17,"data":[1]}` | Đã áp. Gửi đúng giọng đang chọn cũng trả `[1]` và phát lại welcome (để nghe thử) |
| Chip → MCU | `{"cmd":17,"data":[0]}` | Từ chối: `data` rỗng, nhiều hơn 1 số, hoặc ngoài 1..4. Giọng giữ nguyên, không phát gì |
| MCU → chip | `{"cmd":18,"data":[]}` | Hỏi giọng đang chọn |
| Chip → MCU | `{"cmd":18,"data":[v]}` | Giọng đang chọn, 1..4 |

```text
MCU → chip  {"cmd":17,"data":[2]}     # đổi sang nữ bắc
chip        phát welcome của nữ bắc (file 170)
chip → MCU  {"cmd":17,"data":[1]}
MCU → chip  {"cmd":18,"data":[]}
chip → MCU  {"cmd":18,"data":[2]}
MCU → chip  {"cmd":17,"data":[5]}     # ngoài 1..4
chip → MCU  {"cmd":17,"data":[0]}     # vẫn là giọng 2
```

- Có hiệu lực ngay với mọi câu sau đó: trả lời wake, lệnh, học lệnh, welcome, sleep.
- Đang tắt tiếng: vẫn đổi và lưu, không phát welcome.
- Đang thức (sau wake word): thời gian thức 15 s tính lại từ lúc set, nên welcome dài vài
  giây không làm máy đi ngủ ngay sau đó. Đang ngủ thì vẫn ngủ.
- Giọng lưu NV 1 byte (`NVDATA_ID_USER_START + 9`), còn sau khi tắt nguồn.
- Cách phát: firmware, cmd_info và `CMD_RESP_SET` chỉ dùng response id. Bộ phát
  (`prompt_player.c`, `prompt_player_set_voice_offset()`) cộng `100 × (giọng − 1)` vào mọi
  id dưới 100 lúc phát; giọng đang chọn thiếu file thì phát file của giọng 1 (hiện cả 4
  giọng đủ câu). Beep 1000, 1001 dùng chung.

Thu lại / thêm câu cho một giọng:

1. Đặt wav (48 kHz mono 16-bit) vào `Downloads/resp/<giọng>/{wake,learn,volume,scene,commands,special}`,
   tên file là lời nói; `special/` là `welcome.wav`, `sleep.wav`.
2. Sửa tên file trong bảng `SLOTS` của `vendor/tools/import_voices.py`, rồi chạy
   `python vendor/tools/import_voices.py check <v>`, `import <v>` (xóa file cũ của giọng `v`
   trong `vendor/firmware/voice/src` rồi chép file mới), `header` (sinh `response.h`).
3. Chạy `vendor/firmware/BUILD_ALL.bat` (build code, tạo bin có cắt lặng voice, kiểm tra flash),
   rồi PACK_UPDATE_TOOL **Refresh** → **Packaging**. Không dùng bat tạo bin của SDK: nó không cắt
   lặng, voice.bin tràn vào vùng NV.

## 4. CWSL: học lệnh bằng giọng nói

CWSL cho người dùng ghi thêm cách nói mới cho một lệnh có sẵn, hoặc thêm từ đánh thức
mới, không cần build lại model ASR. MCU điều khiển qua UART0; không có khẩu lệnh thoại
để vào/thoát chế độ học.

### 4.1. Địa chỉ mẫu: `(cmd_id, slot)`

Một mẫu đã học định danh bằng cặp `(cmd_id, slot)`, ánh xạ xuống bộ ba
`(cmd_id, group_id, word_type)` của SDK (`slot` = `group_id`).

- **Một `cmd_id` giữ được nhiều mẫu** (1-n). Học 3 câu cho `cmd_id=72` thì cả 3 đều
  bật đèn phòng khách.
- **Từ đánh thức** là `cmd_id = WAKE_UP_ID = 1`, dùng chung lệnh học/xóa.
- **Xóa được từng mẫu** nhờ `slot` riêng.

| Loại | `cmd_id` | Slot hợp lệ | Trần |
|---|---|---|---:|
| Từ đánh thức | `1` | `0..4` | 5 mẫu |
| Câu điều khiển | ID trong cmd_info | `0..14` | 15 mẫu, **dùng chung** cho mọi lệnh |
| Tổng | | | `CICWSL_TOTAL_TEMPLATE = 20` |

15 slot command là một pool chung: một lệnh có thể chiếm nhiều slot, miễn tổng ≤ 15.
Khi tổng đầy, học thêm nhận `NO_SLOT` dù lệnh mới chưa dùng slot nào.

Slot trên dây là một byte; `255` (`CWSL_ANY`) là giá trị đặc biệt, không phải slot thật:

| Pha | Frame | Chỗ để số | Giá trị |
|---|---|---|---|
| Học / xóa / status | `cmd:4`, `cmd:5` | `data[2]` | `0..4` (wake), `0..14` (command), hoặc `255` |
| Hỏi occupancy | `cmd:6` | bitmap `data[1..2]`, bit N = slot N | |
| Nhận diện | `cmd:12`, `cmd:13` | key `"cmd_id"` + `"cwsl"` | `cwsl=255` câu gốc, `0..14` mẫu đã học |

Khi người dùng nói câu đã học, `cmd`/`data` giống hệt câu gốc, chỉ `"cwsl"` đổi:

```text
{"cmd":12,"data":[72],"cmd_id":72,"cwsl":255}   # câu gốc "bật đèn phòng khách"
{"cmd":12,"data":[72],"cmd_id":72,"cwsl":0}     # mẫu CWSL slot 0 của cùng lệnh
```

`"cwsl"` hiện là slot thấp nhất đang có mẫu của `cmd_id` đó (`m_cwsl_used_slot`); lệnh
đã học nhiều câu thì số này có thể không đúng ô vừa khớp.

### 4.2. Action MCU gửi (`cmd:4`)

```c
typedef enum
{
    CWSL_ACTION_LEARN  = 1, /* [1, cmd_id, slot] hoặc [1, cmd_id, slot, value] */
    CWSL_ACTION_DELETE = 2, /* [2, cmd_id, slot] */
    CWSL_ACTION_STOP   = 4, /* [4]               */
    CWSL_ACTION_QUERY  = 6, /* [6, cmd_id]       */
} cwsl_action_t;

#define CWSL_ANY  255U
```

| Request | Ý nghĩa |
|---|---|
| `{"cmd":4,"data":[1,<cmd_id>,<slot>]}` | Học vào đúng slot, **đè** nếu slot đã có. Câu trả lời giữ nguyên |
| `{"cmd":4,"data":[1,<cmd_id>,<slot>,<value>]}` | Như trên, thêm key câu trả lời: học **thành công** thì đặt `value` của `cmd_id` như `CMD_RESP_SET` (`0` ngẫu nhiên, `1` mặc định, `2..99` response id). Không giọng nào có câu đó → `0x40` BAD_REQUEST, không học |
| `{"cmd":4,"data":[1,<cmd_id>,255]}` | Học vào slot trống thấp nhất của `cmd_id` (có thể thêm `value` ở cuối) |
| `{"cmd":4,"data":[1,1,255]}` | Học thêm một từ đánh thức |
| `{"cmd":4,"data":[2,<cmd_id>,<slot>]}` | Xóa đúng một mẫu |
| `{"cmd":4,"data":[2,<cmd_id>,255]}` | Xóa mọi mẫu của `cmd_id` |
| `{"cmd":4,"data":[2,255,255]}` | Xóa toàn bộ mẫu (wake + command) |
| `{"cmd":4,"data":[4]}` | Dừng phiên học đang chạy |
| `{"cmd":4,"data":[6,<cmd_id>]}` | Hỏi slot nào của `cmd_id` đang có mẫu |
| `{"cmd":4,"data":[6,255]}` | Hỏi tổng quan sức chứa |

Không có action "vào chế độ học": `LEARN` đi thẳng vào việc thu mẫu. Lệnh chưa từng
set câu trả lời có `value = 0` (ngẫu nhiên), nên học không kèm `value` thì lệnh vẫn
trả lời ngẫu nhiên.

### 4.3. Status trả về (`cmd:5`)

Mọi `LEARN`, `DELETE`, `STOP` sinh **đúng một** frame kết thúc (lớp 2/3/4), kể cả khi bị
từ chối trước khi chạm SDK. Trong lúc thu có thể có frame tiến trình lớp 1 trước đó.
Dạng `{"cmd":5,"data":[code, cmd_id, slot]}`; hai byte sau lặp lại địa chỉ request.
`QUERY` chỉ trả `CMD_CWSL_INFO`.

```c
typedef enum
{
    CWSL_ST_NONE              = 0x00,
    CWSL_ST_RETRY_SPEAK_AGAIN = 0x10, /* lượt thu hỏng; câu 12 */
    CWSL_ST_RETRY_TOO_SHORT   = 0x11, /* quá ít tiếng nói; câu 13 */
    CWSL_ST_RETRY_DUPLICATE   = 0x12, /* trùng lệnh cố định / câu đã học; câu 14 */
    CWSL_ST_LEARN_OK          = 0x20, /* đã lưu mẫu; câu 15 */
    CWSL_ST_DELETE_OK         = 0x21, /* đã xóa; không prompt */
    CWSL_ST_EXIT_OK           = 0x22, /* đã thoát học; câu 19 */
    CWSL_ST_LEARN_FAIL        = 0x30, /* hết lượt thử; câu 16 */
    CWSL_ST_LEARN_ABORTED     = 0x31, /* SDK abort */
    CWSL_ST_NO_SLOT           = 0x32, /* hết slot; câu 17 */
    CWSL_ST_DELETE_FAIL       = 0x33, /* không xóa được; câu 18 */
    CWSL_ST_TIMEOUT           = 0x34, /* 30 s không tiến triển; câu 16 */
    CWSL_ST_BAD_REQUEST       = 0x40, /* payload sai, action lạ, value câu trả lời sai */
    CWSL_ST_BAD_COMMAND_ID    = 0x41, /* cmd_id không có trong cmd_info */
    CWSL_ST_BAD_SLOT          = 0x42, /* slot vượt trần */
    CWSL_ST_BUSY              = 0x43, /* đang có thao tác CWSL chạy dở */
    CWSL_ST_NOT_IN_LEARN_MODE = 0x44, /* STOP khi không đang học */
} cwsl_status_t;
```

| `code >> 4` | Nghĩa |
|---:|---|
| `1` | Chưa xong: chip bảo nói lại, tiếp tục chờ |
| `2` | Xong, thành công |
| `3` | Xong, thất bại |
| `4` | Request bị từ chối |

Tối đa 3 lượt thu; lượt hỏng thứ 3 không gửi lớp 1 mà phát câu 16 và trả `0x30 LEARN_FAIL`.
Số "câu N" là response id (mục 3.1), phát bằng giọng đang chọn.

### 4.4. Flow học và xóa

```text
# Học thêm một cách nói cho "tắt đèn phòng khách" (72), tự chọn slot,
# đồng thời chọn câu trả lời 47 (xong việc)
MCU  -> chip:  {"cmd":4,"data":[1,72,255,47]}
chip           phát câu 11 "lệnh mới là gì"
người dùng     nói câu cần học một lần
chip           phát câu 15
chip -> MCU:   {"cmd":5,"data":[32,72,0]}     # 0x20 LEARN_OK, slot 0; value của 72 = 47

# Học từ đánh thức
MCU  -> chip:  {"cmd":4,"data":[1,1,255]}
chip           phát câu 11, thu một lần, phát câu 15
chip -> MCU:   {"cmd":5,"data":[32,1,2]}      # wake slot 2

# Xóa đúng wake slot 2
MCU  -> chip:  {"cmd":4,"data":[2,1,2]}
chip -> MCU:   {"cmd":5,"data":[33,1,2]}      # 0x21 DELETE_OK

# Value câu trả lời sai (không giọng nào có câu 9)
MCU  -> chip:  {"cmd":4,"data":[1,72,255,9]}
chip -> MCU:   {"cmd":5,"data":[64,72,255]}   # 0x40 BAD_REQUEST, không học

# Hủy giữa chừng
MCU  -> chip:  {"cmd":4,"data":[1,72,255]}
MCU  -> chip:  {"cmd":4,"data":[4]}
chip           phát câu 19
chip -> MCU:   {"cmd":5,"data":[34,72,0]}     # 0x22 EXIT_OK
```

Sau mọi kết quả cuối, chip tự thoát học và về chờ wake (`APP_ST_IDLE`). Đang chạy thì
request CWSL khác nhận `0x43 BUSY`. Timeout 30 s không tiến triển
(`CWSL_IDLE_TIMEOUT_MS`), gia hạn khi SDK bắt đầu đăng ký mẫu và sau mỗi lượt thu.

### 4.5. Query sức chứa (`cmd:6`)

```text
MCU  -> chip:  {"cmd":4,"data":[6,72]}
chip -> MCU:   {"cmd":6,"data":[72,5,0]}      # bitmap 0x0005 -> slot 0 và 2

MCU  -> chip:  {"cmd":4,"data":[6,255]}
chip -> MCU:   {"cmd":6,"data":[255,2,6,12]}  # 2 wake, 6 command, 12 slot trống
```

- Một `cmd_id`: `[cmd_id, bitmap_low, bitmap_high]`, bit N = slot N đang có mẫu.
- Tổng quan: `[255, wake_used, command_used, free]`.
- Đọc thuần, trả lời được ở mọi trạng thái kể cả khi đang học.

### 4.6. Điều kiện học

- Học chỉ thêm **cách nói**, không tạo chức năng mới: câu học cho `cmd_id=72` chạy
  đúng hành động của 72.
- Chip chỉ kiểm tra `cmd_id` có trong cmd_info. ID hợp lệ: wake `1..6` (học wake dùng
  `1`), âm lượng `48..55`, ngữ cảnh `56..70`, lệnh nhà `71..103`.
- Một lượt nói vừa kiểm tra trùng vừa ghi mẫu. Trùng lệnh đang học, lệnh cố định khác
  hoặc mẫu đã học → phát câu 14 và thu lại cùng slot; đủ 3 lần mới fail.
- Học đè slot đã có mẫu thì mẫu cũ bị **xóa trước**; lần thu đó thất bại thì câu cũ mất.

| Response id | Prompt |
|---:|---|
| 11 | Mời nói câu mới (học lệnh và học wake) |
| 12 | Chưa nghe rõ, nói lại |
| 13 | Câu quá ngắn |
| 14 | Câu bị trùng |
| 15 | Học thành công |
| 16 | Học thất bại (lệnh, wake, hết giờ) |
| 17 | Bộ nhớ học lệnh đã đầy |
| 18 | Xóa thất bại (xóa thành công không phát câu nào) |
| 19 | Đã kết thúc học lệnh |

### 4.7. FSM

Một FSM duy nhất trong `user/application/main_task/main_task.c`; CWSL là trạng thái
con trong `main_cwsl.c`.

| State | Xử lý |
|---|---|
| `APP_ST_IDLE` | Chỉ nhận wake word hoặc request từ MCU |
| `APP_ST_SERVING` | Đã wake: nhận lệnh thoại, request MCU |
| `APP_ST_CWSL_BUSY` | Một thao tác CWSL độc chiếm engine, không phục vụ lệnh thoại |

## 5. Smoke test bằng web tool

`vendor/tools/cwsl_uart_web.py`, chỉ bind localhost, hiển thị TX/RX và giải mã frame.

```powershell
py -m pip install pyserial
py .\vendor\tools\cwsl_uart_web.py          # đổi cổng: --port 9000
```

Mở `http://127.0.0.1:8765/`, chọn COM nối UART0, baud `115200`, **Connect**.

1. **Học lệnh**: Command ID `72`, Slot `255`, **Learn**, nói một lần sau prompt, đợi
   `<- LEARN_OK ... cmd_id=72 slot=0`. Nói lại câu vừa học, phải thấy
   `{"cmd":12,"data":[72],"cmd_id":72,"cwsl":0}`. **Query this ID** để xem slot.
2. **Giọng**: mục "Giọng", nhấn **1..4** → chip phát welcome giọng đó, log `VOICE_SET OK`;
   **Get giọng** hiện giọng đang chọn. "Phát thử resp" (bản debug, `cmd 204`) phát một câu bằng
   giọng đang chọn, bên dưới hiện câu của cả 4 giọng; **Phát lần lượt tất cả** nghe hết bộ
   (3,5 s một câu), **Dừng** để ngừng (câu đang phát vẫn nói hết).
3. **Câu trả lời**: chọn cmd_id + value (danh sách resp id), **Set** phải ra `RESP_SET OK`;
   **Get** ra `RESP 71=random` / `default` / `50 RESP_LIGHT_ON`. Nút **Tất cả lệnh** set lệnh
   56..103; hàng **Wake** set cùng lúc cả 6 wake word (ngẫu nhiên / mặc định / câu 2 / câu 3).
4. **Âm lượng**: **Query level** → `cmd:9`; nhấn **3** → beep và `[3,1,0]`; custom `cmd 8`
   data `0` → `[3,0,x]` (từ chối).

Mục "Fan MCU simulator" trong tool là di sản từ bản quạt, firmware hiện tại không dùng.
Danh sách COM trống hoặc mở cổng lỗi: đóng tool khác đang giữ cổng rồi **Refresh**.

## 6. Lưu ý workbook cmd_info

- Hai sheet `<0>cmd` và `<1>wake` phải dùng **cùng** `命令词ID` cho cùng một câu.
- Đổi danh sách lệnh / đánh số lại ID thì phải cập nhật cùng lúc: model ASR
  (`vendor/firmware/asr/[0]`, `[1]…dat`), Excel cmd_info, `user/application/commands.h`,
  `s_frames` (`main_cmd.c`), `s_rows` (`main_resp.c`). Chỉ pack cmd_info mà không đổi
  firmware sẽ làm lệnh lệch.
- Cột prompt (播报音0ID) là **response id** (giọng 1), bộ phát tự cộng offset theo giọng:
  wake → 1, âm lượng → 40..45, ngữ cảnh → 46, lệnh nhà → 50..68, `<welcome>` → 70,
  `<inactivate>` → 71, `<beep>` → 1000, `<notify>` → 1001.
