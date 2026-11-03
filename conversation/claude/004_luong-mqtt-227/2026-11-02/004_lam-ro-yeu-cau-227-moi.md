# Làm rõ yêu cầu: 227 chạy theo mô tả mới (mục 13 workbook)

## Hỏi

Sửa MQTT 227 (Thêm ngữ cảnh giọng nói) chạy theo mô tả mới ở mục 13. Không rõ thì hỏi lại, không đoán.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Đã làm

- Đọc sheet "Loa thông minh" trong `docs/voice/hnn_voiceboxv1.xlsx` (mtime 15:50:29, chưa commit), PHẦN 3, mục 13 tới 16, và PHẦN 5.
- Mục 13:
  - Request: `{"voiceboxv1":227,"u","act_id"}`.
  - Thành công: `{"voiceboxv1":227,"cmd_id":46,"u","act_id","result":1}`. Cấp ô trống nhỏ nhất trong dải 46..55, tối đa 10.
  - Đầy: `{"voiceboxv1":227,"u","act_id","result":0,"code":50}`.
- So với code hiện tại:
  - dải `cmd_id`: code dùng 56..65, workbook ghi 46..55;
  - mã đầy: code dùng 71, workbook ghi 50;
  - format request và response: giống nhau.
- Mâu thuẫn tìm thấy:
  - Trong chính PHẦN 5, 48..55 là các lệnh âm lượng, còn 56..65 là KÍCH-HOẠT-NGỮ-CẢNH 1..10.
  - Theo `commands.md` mục 4.6, chip chỉ nhận học cho ID có trong cmd_info. 46 và 47 không có trong đó.
- Mục 13 không nói 227 có kèm học giọng hay không. Mục 7 ghi 220 dùng 46..55 cho ngữ cảnh. Mục 14 lại ghi 222 dùng để hủy "quá trình thêm ngữ cảnh".

## Quyết định

- Người dùng: `hnn_voiceboxv1.xlsx` là tài liệu cũ. Họ chỉ tới danh sách id hợp lệ ở `commands.md` mục 3.1: 1–3, 11–19, 40–47, 50–68, 70, 71.
- Danh sách đó là **response id** (mã câu chip nói, dùng làm `value`), không phải `cmd_id`. Theo `commands.md` mục 4.6, `cmd_id` của ngữ cảnh là 56..70.
- Người dùng trả lời qua bảng hỏi:
  - Mô tả 227 theo mục 13 của hnn: `cmd_id` 46..55, đầy trả code 50.
  - Danh sách ở mục 3.1 dùng làm `value` gửi kèm khi học.
  - WB2 tự đổi số khi nói với chip: App dùng 46..55, chip dùng 56..65.
  - Key `"value"` không bắt buộc. `value` sai thì WB2 chặn trước, trả code 64.
  - Sửa cả 224, 228, 229 cho khớp 46..55.
  - Bỏ MQTT 224 cho lệnh âm lượng 48..55, để không trùng số.
  - BLE ADV giữ số chip 56..65.
  - 228 giữ `cmd_id` 255 (xóa tất cả).
  - 222 hủy thêm ngữ cảnh trả theo mục 14: `cmd_id`, `slot`, `code`, `u`, `act_id`, không có `result`.
- Đã gửi người dùng mô tả tổng hợp, chờ duyệt.

## Không làm

- Chưa sửa code.

## Chưa kiểm

- Chưa thử trên board chip trả gì khi học `cmd_id` 46.
