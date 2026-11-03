# Làm rõ yêu cầu: 3 nút nhấn VOICEBOX (IO 21, 20, 0)

## Hỏi

- Sửa module nút nhấn: có 3 nút nhấn, lần lượt ở IO 21, 20, 0.
- Nút 2 (IO20): ấn giữ chạy chức năng cấu hình, ấn bình thường đổi voice_id.
- Nút 1 và nút 3: lần lượt là tăng và giảm âm lượng.
- Người dùng nhắc: "không đoán mò, không rõ gì thì hỏi tôi".

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định (người dùng chốt)

- **Nút và chân:**
  - bỏ nút GPIO1 cũ;
  - nút 1 = IO21, nút 2 = IO20, nút 3 = IO0;
  - nhấn = mức 0, WB2 bật pull-up.
- **Nút 2:**
  - giữ: giống hệt nút cũ, theo code là giữ đủ 5 s rồi thả thì smart config, giữ đủ 10 s rồi thả thì AP config. Hai mốc này lấy theo code; trước đó Claude nói nhầm là 15 s;
  - nhấn nhanh (thả trước 5 s): đổi giọng theo vòng 1→2→3→4→1 bằng cmd 17;
  - WB2 chưa biết giọng thì hỏi cmd 18 trước.
- **Nút 1 tăng, nút 3 giảm âm lượng:**
  - chạy ngay khi nhấn xuống, ±1 mức trong 1..7;
  - giữ lâu không lặp và không chạy cấu hình;
  - WB2 chưa biết mức thì hỏi cmd 11 trước.
- **Ở biên:**
  - mức 7 mà nhấn tăng, mức 1 mà nhấn giảm: gửi lại `cmd 8` với mức hiện tại, rồi báo App;
  - đang tắt tiếng: tăng thì lên mức 1;
  - đang tắt tiếng mà giảm: gửi `cmd 8 [8]` để giữ tắt tiếng, báo App `vol` 0.
- **Âm báo:**
  - không beep lúc nhấn; chip tự beep khi mức âm lượng đổi, và phát welcome khi đổi giọng;
  - đang bận thì bỏ qua lần nhấn và cho chip beep dài (`cmd 200 [1]`);
  - riêng lúc đang trong phiên học (220, hoặc 227 thêm mới, ở mọi giai đoạn) thì bỏ qua và im lặng;
  - các thao tác khác (221, 223, 227 sửa, 228) vẫn beep dài.
- **Báo App khi thành công:**
  - giọng: `{"voiceboxv1":230,"u":0,"voice_id":v,"act_id":1,"result":1}`;
  - âm lượng: `{"voiceboxv1":225,"vol":v,"u":0,"act_id":1,"result":1}`. Đang tắt tiếng thì `vol` là 0.
- **Báo App khi thất bại** (chip trả `[0]`/`result 0`, không trả lời trong 2 s, hoặc bước hỏi trước không có trả lời):
  - 230: `{"voiceboxv1":230,"u":0,"voice_id":v,"act_id":1,"result":0,"code":N}`. Mã: 64 chip trả `[0]`, 52 không trả lời, 70 gửi UART lỗi;
  - 225: `{"voiceboxv1":225,"vol":v,"u":0,"act_id":1,"result":0}`;
  - `v` là giá trị WB2 định đặt (tắt tiếng = 0). Lỗi ngay ở bước hỏi, chưa có giá trị định đặt, thì `v` = 0.
- **Lệnh App trong lúc lần nhấn đang chờ chip:** nhận lỗi bận như hiện nay.
  - 220–223, 227, 228, 230, 231 nhận 67;
  - 225/226 nhận `result:0`.
  - Lần nhấn dùng chung cơ chế chờ chip với 225/230: chờ 2 s, rồi 2 s bỏ frame đến muộn.
- **cmd 9:**
  - parser chỉ nhận đúng 3 phần tử `[level, result, muted]`, muted 0/1;
  - dạng 2 phần tử bị từ chối (MQTT 299).
- **Bản tin 105 và 226 giữ như hiện tại.** Cờ muted chỉ dùng cho nút nhấn.

## Đã làm

- Đọc `m_button.c`, `button.c`, `hardware.h`, `app_btn.c`, `main.c`, `m_fan.c`, `commands.md` mục 2.2.
- **IO 0, 20, 21** là `OE_PIN`, `STCP_PIN`, `SHCP_PIN` của 74HC595, nhưng 74HC595 và quạt không khởi tạo trên VOICEBOX. `m_fan_prepare_for_ota` cũng return sớm trên VOICEBOX, nên ba chân này đang trống.
- **Thư viện nút:** HOLD ở 5 s; HOLD_SO_LONG ở 10 s; HOLD_HONG_PHIM ở 25 s, VOICEBOX bỏ qua sự kiện này.
- **Nút GPIO1 cũ có vẻ ngược mức:** thư viện coi mức nghỉ là 0 (`BTN_IDLE_LEVEL`), nhưng `hw_btn_read` trả 1 khi không nhấn. Ba nút mới đặt mức nghỉ là 1.
- **Callback nút chạy trong `led_button_task`,** còn trạng thái 225/230 và UART chạy trong task ứng dụng. Vì vậy lần nhấn phải được chuyển sang task ứng dụng, không gọi thẳng.

## Không làm

- Không đổi thư viện nút `huytv_button`.
- Không đổi hành vi nút trên các thiết bị không phải VOICEBOX.

## Chưa kiểm

- Mạch thật của 3 nút (pull-up ngoài, chống dội) chưa xem.
