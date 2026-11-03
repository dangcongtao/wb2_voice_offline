# Thực hiện plan 227 (15 ngữ cảnh, value, code 50, 222 mục 14, 228) bằng subagent

## Hỏi

Người dùng duyệt plan `006_plan-sua-227-ngu-canh-15.md` và chọn chạy bằng subagent. Giữa chừng phiên bị ngắt vì giới hạn API; người dùng đăng nhập lại và bảo chạy tiếp.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định (ruling khi chạy)

- R1: Ca xóa tất cả với mask khởi đầu `0x4005` viết thành Test 7b riêng. Test 7 cũ giữ nguyên.
- R2: Không commit. Review từng task bằng snapshot git tree.
- R3: Lúc chạy, `aiio_ci13162_config.h` đang để TX=3/RX=4 (sửa lúc 17:24, trước khi plan chạy). Không hoàn tác. Test chạy bằng bản sao đã bỏ 5 assert chân UART. Người dùng đổi lại TX=4/RX=3 lúc 17:54, nên ruling này không còn tác dụng; runner gốc pass.
- R4: Reviewer cuối thấy `value` của ngữ cảnh đã xóa còn lưu trên chip, và lần 227 sau dùng lại `cmd_id` đó mà không có `value` thì chip vẫn đáp câu cũ. Không sửa code, vì người dùng đã chọn "không có `value` thì gửi 3 phần tử, chip giữ câu đang có". Hỏi lại người dùng.
- R5: Đợt sửa cuối chỉ sửa 4 chỗ tài liệu/comment lệch. Các minor về style để lại.
- R6: Câu "chỉ còn cách xóa bằng 228 `cmd_id` 255" trong mục 11 của file đề xuất bỏ sót MQTT 221 255/255. Đường này cũng xóa được, nhưng xóa cả wake. Giữ nguyên câu, báo người dùng.
- R7: Giữ workspace SDD (`.superpowers/sdd/006_plan-sua-227-ngu-canh-15/`), vì chưa có commit nào làm bản ghi.

## Đã làm

- **Task 1:** nâng dải ngữ cảnh lên 56..70 (15 ngữ cảnh).
  - `APP_SCENE_COUNT_MAX` 15, `APP_SCENE_CMD_ID_MAX` 70, thêm `APP_SCENE_MASK_ALL` (`0x7FFF`), `APP_CWSL_CMD_ID_MAX` 70.
  - Parser `cmd:12` nhận `data` 1..15, `cmd:6` nhận `cmd_id` tới 70.
  - Sửa các bảng test nhúng và runner.
- **Task 2:**
  - 227 nhận key `"value"` tùy chọn và kiểm theo tập hợp lệ; sai thì trả 64.
  - FSM giữ `has_value`/`value`. LEARN gửi `[1,cmd_id,255,value]`.
  - `app_cwsl_start_scene_learn` có thêm 2 tham số.
  - Đủ ngữ cảnh trả 50.
- **Task 3:**
  - 222 khi hủy thêm ngữ cảnh trả `cmd_id, slot, code, u, act_id`, dùng lại `app_cwsl_publish_mutating_final`; bỏ `app_cwsl_publish_stop_response`.
  - 228 với ngữ cảnh chưa kích hoạt trả 65.
- **Task 4:**
  - Cập nhật `uart_wb2_ci13162.md`.
  - Ghi trạng thái "đã code" vào `luong_mqtt_227_them_ngu_canh_moi.md`.
  - Thêm dòng trỏ sang luồng mới ở đầu `luong_mqtt_227_them_ngu_canh.md`.
- **Review:**
  - 4 task đều Approved. Task 4 cần 1 vòng sửa: dòng `cmd:6` trong tài liệu còn ghi `1..65`.
  - Review cuối (Opus): sẵn sàng merge, 1 Important (R4), 4 Minor. Đã sửa cả 4 Minor; re-review xác nhận.
- **Kết quả trên cây cuối:**
  - Runner gốc: scene 16, cwsl 8 (`failed=0`), uart 16, voicebox 9 — đều pass.
  - `make -j16` exit 0, không có warning ở file đã sửa. `switchv3.bin` 925.004 B.

## Không làm

- Không sửa `aiio_ci13162_config.h` và `m_database.c`; người dùng tự sửa hai file này lúc 17:54.
- Không sửa `CWSL_MQTT_SPEC.md`, `hnn_voiceboxv1.xlsx`, firmware chip, 220/221/223.
- Không sửa các lỗi cũ đã liệt kê ở mục 11 của file đề xuất.

## Chưa kiểm

- Chưa kiểm trên board:
  - 227 với `value` 47;
  - ngữ cảnh thứ 11 nhận `cmd_id` 66;
  - 222 hủy ở hai pha;
  - 228 với ngữ cảnh chưa kích hoạt.
- Chưa commit.
