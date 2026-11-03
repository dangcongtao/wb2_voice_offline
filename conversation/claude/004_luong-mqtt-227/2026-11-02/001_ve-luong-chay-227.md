# Vẽ luồng chạy MQTT voiceboxv1:227

## Hỏi

Mục tiêu là sửa `voiceboxv1:227`. Bước này: vẽ luồng chạy của 227 ra file `.md`.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định

- Hiểu `voiceboxv1:227` là lệnh MQTT 227 (thêm ngữ cảnh bằng giọng nói) trên VOICEBOX, theo `process_data.c:118` và `app_scene.h:13`.
- Đặt file tài liệu cạnh các tài liệu giọng nói khác: `docs/voice/luong_mqtt_227_them_ngu_canh.md`. Sơ đồ vẽ bằng Mermaid.

## Đã làm

- Dò code: MQTT/BLE → `pqueue` → `app_handler_msg_task` → `app_scene_add` → `app_cwsl_start_scene_learn` → các pha QUERY_OLD / LEARNING / DELETING_OLD → `app_scene_on_learn_result` → publish 227; đường UART1 RX → `ci13162_uart` → `command_queue` → `aiio_ci13162_uart_process`; STOP 222; timeout; MQTT 224 khi dùng ngữ cảnh.
- Viết `docs/voice/luong_mqtt_227_them_ngu_canh.md`, gồm: tóm tắt, sơ đồ task, sơ đồ kiểm tra request, sequence thành công, FSM, bảng STOP, bảng mã kết quả, luồng 224, 8 điểm cần chú ý khi sửa, bảng hàm.
- Các điểm chính:
  - dải `cmd_id` ngữ cảnh không khớp (code 56..65, comment 58..67, spec 46..55, chip 56..70);
  - lưu flash lỗi sau LEARN_OK để lại mẫu rác trên chip;
  - xóa slot cũ lỗi vẫn báo thành công;
  - cmd5 `32` đến trong pha QUERY_OLD được tính là thành công;
  - FSM 227 chưa có host test.

## Không làm

- Không sửa code 227, chờ người dùng chốt hướng sửa.
- Không sửa comment/spec lệch dải `cmd_id`.

## Chưa kiểm

- Chưa render thử sơ đồ Mermaid.
- Chưa commit file tài liệu và nhật ký.
