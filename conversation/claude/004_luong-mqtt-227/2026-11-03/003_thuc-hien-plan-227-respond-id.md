# Thực hiện plan 227 format mới (cmd_id 255/56..70, respond_id) bằng subagent

## Hỏi

Người dùng duyệt plan `002_plan-sua-227-respond-id.md` và chọn chạy bằng subagent.

## Branch

- `/home/taodc/wb2/ai-wb2-baremetal`: `voiceboxv1/taodc/dev_261102`

## Quyết định (ruling khi chạy)

- R1: Không commit. Review từng task bằng snapshot git tree.
- R2: Khoảng 08:36, `docs/voice/hnn_voiceboxv1.xlsx` biến mất khỏi working tree. Subagent không đụng tới file này, nên tôi không khôi phục. Lúc 08:51 người dùng commit bản mới (`ae7f4b0f`).
- R3: Lỗi Important ở review cuối: frame `cmd:15` đến muộn sau timeout bị tính cho lệnh sửa kế tiếp. Sửa bằng khoảng chờ 2 s sau timeout:
  - frame muộn bị bỏ và kết thúc khoảng chờ;
  - lệnh sửa gửi trong khoảng chờ nhận 67;
  - học, xóa, query và âm lượng không bị chặn.
- R4: Chip vẫn giữ câu phản hồi đã sửa cho `cmd_id` sau khi 228 xóa ngữ cảnh. Không sửa, vì nằm ngoài plan; hỏi người dùng.
- R5: Gộp các Minor về tài liệu và test thứ tự kiểm vào lượt sửa cuối.
- R6: Giữ workspace SDD `.superpowers/sdd/002_plan-sua-227-respond-id/`.

## Đã làm

- **Task 1:**
  - UART cmd 15: `aiio_ci13162_uart_send_resp_set()` gửi `{"cmd":15,"data":[cmd_id,value]}`;
  - parser nhận `cmd:15` với đúng 1 phần tử 0/1, không có `cmd_id`/`cwsl`;
  - callback `aiio_ci13162_uart_set_resp_set_callback()`.
- **Task 2:**
  - `app_cwsl_start_scene_learn(cmd, u, act, respond_id)`, LEARN luôn 4 phần tử.
  - Phiên `app_cwsl_start_scene_resp_set(cmd_id, respond_id)`:
    - mã trả về: 65 / 69 / 67 / 70;
    - chip trả `[1]` → 0, `[0]` → 64, hết 2 s → 52;
    - trong lúc chờ: tạm dừng hỏi version; 220..223, 227/228 nhận 67, còn 225/226 bị chặn và trả `result:0`.
- **Task 3:**
  - viết lại `app_scene_add`, thay phần người dùng đang sửa dở. Phần đó có lỗi ghi `uint32_t` vào biến `uint8_t`;
  - thứ tự kiểm: 64 → 65 → 67 → nhánh thêm (50) / nhánh sửa (65);
  - bản tin 227 kèm `cmd_id` và `respond_id`;
  - thêm `app_scene_on_resp_set_result()`;
  - runner tự định nghĩa 2 key.
- **Task 4:** cập nhật `luong_mqtt_227_them_ngu_canh_moi.md`, thêm mục 4A "Sửa câu phản hồi", và cập nhật `uart_wb2_ci13162.md`.
- **Review:**
  - 4 task đều Approved.
  - Review cuối (Opus) kết luận With fixes: 1 Important (R3), 4 Minor.
  - Lượt sửa cuối F1–F6 được re-review xác nhận đã xử lý hết.
- **Kết quả trên cây cuối:**
  - Runner: uart 17, cwsl 9 (`failed=0`), scene 17, voicebox 9 — đều pass.
  - `make` exit 0, không có warning ở 3 file C đã sửa. `switchv3.bin` 925.964 B, build sau lần sửa cuối.

## Không làm

- Không học lại giọng khi sửa.
- Không làm RESP_GET (16) và VOICE_SET/GET (17/18).
- Không sửa 228 để reset câu phản hồi (R4).
- Không sửa `CWSL_MQTT_SPEC.md` và `hnn_voiceboxv1.xlsx`.

## Chưa kiểm

- Chưa kiểm trên board:
  - thêm mới với `respond_id` 47;
  - sửa `cmd_id` 56 sang `respond_id` 50, chip trả `[1]`;
  - sửa với `respond_id` mà không giọng nào có, chip trả `[0]`, App nhận 64;
  - sửa ngữ cảnh chưa thêm, App nhận 65.
- Chưa commit.
