# Bổ sung cmd_id mới vào phần 5 file Excel dự án

## Hỏi

Người dùng thêm file Excel tài liệu dự án `docs/voice/hnn_voiceboxv1.xlsx`. Phần 5 (sheet `Loa thông minh`) chưa có các cmd_id mới. Cần bổ sung và tô vàng các ô thêm mới để phân biệt.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định

- Nguồn cmd_id: `docs/voice/[60000]{cmd_info_old}.xlsx`, sheet `<0>cmd` (model 2026-11-01, khớp `commands.md`). File cùng tên ở gốc repo là model quạt cũ, không dùng.
- Mỗi cmd_id một dòng, nối sau dòng 58 (STT 7): âm lượng 48–55, ngữ cảnh 56–70, lệnh nhà 71–103. Tổng 56 dòng, dòng 59..114, STT 8..63.
- Cột ghi chú: loại lệnh và frame UART chip gửi (`cmd 13` cho âm lượng, `cmd 12` + `data` cho ngữ cảnh / lệnh nhà).
- Sửa thẳng XML trong file xlsx (`sheet72.xml`, `styles.xml`, `sharedStrings.xml`), không dùng openpyxl để lưu. File có 72 sheet, ảnh, chart, threaded comment; openpyxl lưu lại sẽ làm mất các phần đó.
- Ô mới dùng style mới (xf 1598): giống style dòng cũ (viền, font Roboto 10, canh trên, wrap) nhưng nền vàng `FFFFFF00` (fill có sẵn số 11).

## Đã làm

- Script `add_rows.py` (trong scratchpad) chèn 56 dòng, thêm 108 chuỗi vào sharedStrings, cập nhật `count`/`uniqueCount`, thêm 1 xf. Các entry khác trong zip giữ nguyên byte và thứ tự.
- Kiểm tra: zip hợp lệ, 3 file XML parse được, chỉ 3 entry thay đổi. Đối chiếu tự động 56 cặp (câu lệnh, cmd_id) với cmd_info: khớp hết. openpyxl (venv tạm) mở được, `A59`/`D114` nền `FFFFFF00`, dòng cũ giữ nền cũ.
- Ghi đè `docs/voice/hnn_voiceboxv1.xlsx` sau khi xác nhận file chưa đổi kể từ lúc đọc và đang trùng HEAD.

## Không làm

- Không sửa các dòng cũ của phần 5, dù có chỗ lệch:
  - Dòng 52–57: tên wake word là bộ cũ (XIN-CHÀO-HU-NÔ-NÍT…). Model mới dùng ID 1..6 cho HÂY-HU-NÔ-NÍCH, XIN-CHÀO-HU-NÔ-NÍCH, HÂY-HU-NÔ, XIN-CHÀO-HU-NÔ, HU-NÔ-ƠI, HU-NÔ-NÍCH-ƠI.
  - Dòng 58: ghi ngữ cảnh tự học là `46 – 55`, trùng dải âm lượng 48–55 của model mới. Code WB2 dùng 56–65 (`app_scene.h`).
- Không sửa phần 3 (ví dụ `cmd_id` 46..55 trong MQTT 220/227/228/229) và phần 4.
- Không thêm ID đặc biệt `<welcome>` 10001, `<inactivate>` 10002, `<beep>` 10003, `<notify>` 10004.
- Không sửa firmware.

## Chưa kiểm

- Chưa mở file bằng Excel / Google Sheets để xem hiển thị (máy không có LibreOffice).
- Chưa commit file Excel và file nhật ký.
