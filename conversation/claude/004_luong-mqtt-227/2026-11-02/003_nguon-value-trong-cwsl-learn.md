# Nguồn của `value` trong `[1,cmd_id,slot,value]`

## Hỏi

`value` trong `{"cmd":4,"data":[1,cmd_id,slot,value]}` lấy từ đâu? Nếu đoán thì phải nói là đoán.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Đã làm

- Đọc `docs/voice/commands.md` mục 3, 3.1 và 4.2. `value` là response id của câu chip nói khi lệnh đó chạy, cùng nghĩa với `value` trong `CMD_RESP_SET` (cmd 15):
  - `0`: ngẫu nhiên;
  - `1`: câu mặc định;
  - `2..99`: một response id cụ thể.
- Đọc `docs/voice/response_text.txt`: có lời câu theo 4 giọng, cùng id.
- Tìm nơi phía WB2 có thể lấy `value`:
  - `CWSL_MQTT_SPEC.md`: không có field nào cho nó;
  - `docs/voice/hnn_voiceboxv1.xlsx` (PHẦN 3, MQTT 220/227): không có;
  - code: không có;
  - source chip (`response.h`, `main_resp.c`): không có trên máy, chưa đọc.
- Thấy thêm: workbook ghi 227 cấp `cmd_id` trong dải 46..55 và báo đầy bằng code 50. Code dùng 56..65 và code 71.

## Không làm

- Không sửa code, tài liệu.

## Chưa kiểm

- Chưa đọc source chip. Chưa kiểm trên board.
