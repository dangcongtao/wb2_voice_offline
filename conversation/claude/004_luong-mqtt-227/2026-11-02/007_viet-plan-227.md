# Viết plan sửa 227 (15 ngữ cảnh, value, code 50, 222 mục 14, 228)

## Hỏi

Người dùng: "viết plan đi". Đề xuất trong `docs/voice/luong_mqtt_227_them_ngu_canh_moi.md` được coi là đã duyệt, kể cả các chỗ [tôi tự chọn].

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định

Plan ở `006_plan-sua-227-ngu-canh-15.md`, trạng thái chờ duyệt. Gồm 4 task:
1. Mở rộng dải ngữ cảnh lên 56..70.
2. `value` và code 50.
3. 222 theo mục 14, và 228 chưa kích hoạt trả 65.
4. Tài liệu và kiểm tra cuối.

Các lựa chọn khi viết plan:
- 222 dùng lại `app_cwsl_publish_mutating_final`, xóa `app_cwsl_publish_stop_response`.
- Ở 228, bước kiểm "chưa kích hoạt" đặt sau bước kiểm `pending`. Nhờ vậy, trong lúc đang xóa tất cả, request vẫn nhận 67 như Test 8 hiện tại.
- Parser UART include `app_scene.h` để dùng `APP_SCENE_COUNT_MAX`.
- Không sửa `CWSL_MQTT_SPEC.md`.

## Đã làm

- Đọc 3 runner (scene, cwsl, uart) và các bảng test nhúng, để tìm những test bị ảnh hưởng bởi biên 65/66/68/71 và `0x03FF`.
- Chạy cả 4 runner trên cây hiện tại làm mốc: đều pass (9/6/13/14), CWSL validation `failed=0`.

## Không làm

- Chưa sửa code.

## Chưa kiểm

- Chưa build, chưa commit.
