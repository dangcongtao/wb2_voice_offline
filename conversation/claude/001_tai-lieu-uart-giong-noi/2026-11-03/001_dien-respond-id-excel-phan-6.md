# Điền respond_id vào phần 6 file Excel dự án

## Hỏi

Người dùng đã sửa lại `docs/voice/hnn_voiceboxv1.xlsx` (còn 1 sheet `Trang tính1`). Phần 6 "DANH SÁCH CÁC CÂU PHẢN HỒI" mới có header và STT 1..7. Yêu cầu: điền các `respond_id` dựa vào `docs/voice/response_text.txt`. Không đoán; thiếu thông tin thì hỏi lại.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định (người dùng trả lời qua bảng hỏi)

- Mỗi dòng 1 `respond_id`. B = câu phản hồi, C = `respond_id`, D = nhóm và lệnh nào dùng câu đó làm mặc định.
- Đủ tập 227 chấp nhận, 42 id: `0, 1, 2, 3, 11..19, 40..47, 50..68, 70, 71`. Ghi chú rõ id nào do chip tự phát.
- Ô B: gộp câu trùng giữa các giọng, mỗi câu 1 dòng trong ô, đầu dòng là tên giọng (`Nam Bắc`, `Nữ Bắc`, `Nam MN`, `Nữ MN`). Cả 4 giọng giống nhau (70, 71) thì ghi 1 dòng, không có nhãn.
- Nền vàng nhạt `FFF2CC` giống các dòng mới của Phần 5 (style xf 24).

## Đã làm

- Nguồn cột D:
  - câu mặc định của từng `cmd_id`: cột 播报音0ID trong `docs/voice/[60000]{cmd_info_old}.xlsx`, khớp bảng `commands.md` mục 3;
  - câu thêm khi ngẫu nhiên (2, 3, 47, 60, 61): `commands.md` mục 3;
  - nghĩa của 11..19, 70, 71: `commands.md` mục 3.1 và `response_text.txt`.
- Điền dòng 122..163 (STT 1..42), cột A..D. Dòng 122 là id 0 "(Không có câu riêng)".
- Đổi header `B121` từ "Câu lệnh / Nhóm lệnh" sang "Câu phản hồi", theo preview người dùng chọn. `B54` (Phần 5) giữ nguyên.
- Sửa thẳng XML bằng script `fill_part6.py` trong scratchpad, như lần sửa Phần 5. Chỉ `sheet1.xml` và `sharedStrings.xml` đổi: thêm 84 chuỗi, cập nhật `count` 337, `uniqueCount` 319. Các entry khác giữ nguyên byte, nên `persons.xml` và drawing vẫn còn.
- Kiểm tra:
  - XML parse được, zip hợp lệ;
  - so từng ô với file cũ: chỉ đổi `B121` và A..D dòng 122..163;
  - tách ngược ô B theo nhãn giọng: 41 id có câu đều khớp từng giọng trong `response_text.txt`;
  - style ô mới: Roboto, nền `FFF2CC`, viền thin, wrap, canh trên.
- Ghi đè `docs/voice/hnn_voiceboxv1.xlsx` sau khi xác nhận hash chưa đổi từ lúc đọc (trùng commit `ae7f4b0f`).

## Không làm

- Không sửa Phần 3 (dòng `B37`, request 227 còn lỗi cú pháp `"respond_id"1:`; `D37` ghi dải 56..70 nhưng `C37` vẫn ví dụ 46..55) và Phần 5.
- Không đặt chiều cao dòng. File gốc cũng không đặt cho các ô nhiều dòng, nên giữ như vậy.
- Không sửa firmware.

## Chưa kiểm

- Chưa mở bằng Excel / Google Sheets để xem hiển thị, nhất là chiều cao các dòng nhiều câu.
- Chưa commit file Excel và file nhật ký.
