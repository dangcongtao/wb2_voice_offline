# Thêm log báo lỗi cho MQTT 227 như 220

## Hỏi

"thêm log báo lỗi vào cmd 227 như cmd 220": dạng `[ERROR]: <nguyên nhân>, in[<hàm>]`, như các log 220 ở file 001/002.

## Branch

- /home/taodc/wb2/rada/ai-wb2-2026: voiceboxv1/taodc/dev_261102

## Quyết định (người dùng chốt)

- **Repo:** chỉ `rada/ai-wb2-2026`, vì log 220 chỉ có ở repo này. Không đụng `ai-wb2-baremetal`.
- **Phạm vi:** log cả lỗi trả ngay lẫn lỗi trả sau (timeout 52, mã chip, hủy 34, lưu mask lỗi 70, sửa mà chip trả `[0]`).
- **Vị trí:** log ngay tại chỗ kiểm trong `app_cwsl.c` (`app_cwsl_start_scene_learn`, `app_cwsl_start_scene_resp_set`), giống `app_cwsl_start_mutating` của 220.
- **Claude tự quyết:**
  - Lỗi trả sau đi qua 2 hàm nhận kết quả, `app_scene_on_learn_result` và `app_scene_on_resp_set_result`. Mỗi hàm in một dòng kết quả kèm `code`.
  - Chỗ WB2 tự phát hiện lỗi in thêm một dòng nguyên nhân, để phân biệt hai loại timeout 52, và phân biệt UART lỗi với flash lỗi. Mã lỗi của chip (3x/4x, 34) chỉ in ở dòng kết quả.

## Đã làm

- **`app_scene.c`:**
  - `app_scene_add`:
    - tách điều kiện gộp thành từng nhánh, mỗi nhánh một log: js_root NULL, có `action`, sai loại lệnh, `u`, `act_id`, `cmd_id`, `respond_id` thiếu hoặc sai kiểu, `respond_id` ngoài tập hợp lệ;
    - các lỗi khác: `cmd_id` không phải 255 hoặc 56..70, đang xóa tất cả, đủ 15 ngữ cảnh (in mask), sửa `cmd_id` chưa kích hoạt (in mask).
    - Gộp hai nhánh "đủ ngữ cảnh" thành một kiểm `bit_idx`. Mã trả về và bản tin giữ nguyên.
  - `app_scene_on_learn_result`:
    - log `cmd 227 add cmd_id=.. failed, code=..`;
    - log học xong mà `cmd_id` ngoài dải, hoặc không lưu được `scene_mask` (70);
    - viết lại bằng return sớm, hành vi giữ nguyên.
  - `app_scene_on_resp_set_result`: log `cmd 227 edit cmd_id=.. respond_id=.. failed, code=..`.
  - Ba hàm trên đổi sang dạng `{` xuống dòng, tối đa 80 cột.
- **`app_cwsl.c`:**
  - Thêm `app_cwsl_trace_scene_busy()` để in phiên đang bận (mutating, query, resp_set), dùng chung cho 2 hàm start của 227.
  - `app_cwsl_start_scene_learn`: log `cmd_id` ngoài 56..70, OTA, volume/voice đang chờ chip, phiên bận, gửi QUERY lỗi.
  - `app_cwsl_start_scene_resp_set`: log như trên, thêm trường hợp đang bỏ frame cmd 15 đến muộn và gửi cmd 15 lỗi.
  - `app_cwsl_on_resp_set`: log chip trả cmd 15 `[0]`.
  - `app_cwsl_on_info`: log gửi LEARN lỗi (nhánh 227).
  - `app_cwsl_poll`, ba loại timeout:
    - không có cmd 6 trong 2000 ms;
    - học không có kết quả trong 120000 ms (đã gửi STOP);
    - không có cmd 15 trong 2000 ms.
  - Không đụng các dòng người dùng đang sửa dở (dòng 220–330).
- **Kiểm tra:**
  - Runner trên cây làm việc:
    - uart 21, scene 17, voicebox 10, voice 46 đều pass; log 227 in đúng dạng, ví dụ `[ERROR]: respond_id=9 invalid, in[app_scene_add]`;
    - cwsl abort ở `test_app_cwsl_runner.c:493`, đã có từ trước khi sửa. Lý do: chỗ sửa dở đổi điều kiện VOICEBOX `!= 1` thành `> 103`, nên test "220 cmd_id 7 phải bị từ chối 65" không còn đúng.
  - Chạy cwsl trên bản sao trong scratchpad, đã trả riêng hunk VOICEBOX đó về như HEAD: 13/13 pass, các log mới ra đúng chỗ.
  - `-Wall -Wextra -Wformat=2`: không có warning ở code mới. Warning duy nhất là `%lu` với `uint32_t` ở dòng 264, dòng người dùng đang sửa dở, chỉ hiện trên host.
  - `make -j16` exit 0; `app_cwsl.o` và `app_scene.o` được build lại, không có warning. `switchv3.bin` 933.868 B.

## Không làm

- Không thêm log cho 228/229.
- Không thêm log khi xóa slot cũ thất bại trong pha DELETING_OLD: 227 vẫn trả `result:1` nên không tính là lỗi.
- Không thêm log cho final event của 220.
- Không sửa test cwsl đang fail do chỗ sửa dở.

## Chưa kiểm

- Chưa flash, chưa xem log trên board.
- Chưa commit.
