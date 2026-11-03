# Ghi luồng chạy 227 sau khi sửa (bản đề xuất)

## Hỏi

Với plan đã mô tả, luồng chạy cmd 227 sẽ như thế nào? Ghi ra file `.md` để review.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định

- Đặt file cạnh luồng hiện tại: `docs/voice/luong_mqtt_227_them_ngu_canh_moi.md`. Ghi rõ đây là đề xuất, chưa code.
- Những chỗ người dùng chưa nói được đánh dấu **[tôi tự chọn]**:
  - đổi số áp dụng cho mọi loại thiết bị;
  - kiểm `value` trước bước kiểm bận hoặc đầy;
  - 222 hủy trước khi có slot thì `slot` = 255;
  - vẫn gửi bản tin 227 kết thúc;
  - reject của 222 giữ format cũ;
  - lệnh âm lượng vẫn nháy LED và vẫn phát BLE ADV.

## Đã làm

- Viết file gồm:
  - bảng thay đổi so với hiện tại;
  - bảng đổi số App ↔ chip ↔ BLE ↔ bit;
  - sơ đồ kiểm tra request (có `value`, code 50);
  - sequence thành công kèm `value` 47;
  - FSM, chỉ ghi chỗ đổi;
  - bảng hủy 222 theo mục 14;
  - bảng mã 227;
  - luồng 224 (ngữ cảnh 46, âm lượng không gửi 224);
  - 228/229;
  - phần không làm và lỗi cũ còn lại;
  - test dự kiến.
- Ghi thêm một điểm cũ: chip trả 32 đúng lúc vừa nhận STOP thì không có bản tin 222 trả lời người hủy.
- Sửa ví dụ 225: `act_id` là 13 (`SRC_ACTION_VOICE`).
- Lỡ publish file `.md` này thành artifact private (https://claude.ai/artifact/K2oa99oeNbfjcsVP8f7iHg) khi sửa ví dụ 225. Người dùng không yêu cầu publish; đã báo người dùng và hỏi có xóa không.

- Người dùng sửa lại:
  - Dải 46..55 là thông tin cũ; `cmd_id` hợp lệ bắt đầu từ 56.
  - Chọn 15 ngữ cảnh (56..70).
  - Lệnh âm lượng giữ 224 như code hiện tại, vì không còn trùng số.
- Viết lại file đề xuất:
  - bỏ đổi số, App/chip/BLE dùng chung 56..70;
  - `scene_mask` 15 bit;
  - nâng `APP_SCENE_CMD_ID_MAX`, `APP_SCENE_COUNT_MAX`, `APP_CWSL_CMD_ID_MAX` lên 70;
  - parser `cmd:12` nhận `data` 1..15;
  - đủ 15 trả code 50;
  - thêm tác động phụ: 220/221 với `cmd_id` 66..70 đổi từ code 65 sang 64;
  - ghi chú 15 ngữ cảnh chiếm hết 15 slot học dùng chung.

## Không làm

- Chưa viết plan, chưa sửa code.

## Chưa kiểm

- Chưa render thử Mermaid. Chưa commit.
