# Plan: MQTT 227 thêm ngữ cảnh — 15 ngữ cảnh, `value`, code 50, 222 theo mục 14, 228 chưa kích hoạt

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

Trạng thái: **đã duyệt** ngày 2026-11-02. Người dùng chọn chạy bằng subagent.

**Goal:** 227/228/229/224 của voiceboxv1 chạy theo `docs/voice/luong_mqtt_227_them_ngu_canh_moi.md`. Cụ thể:
- 15 ngữ cảnh, `cmd_id` 56..70;
- key `value` tùy chọn, gửi xuống chip khi học;
- đủ ngữ cảnh thì trả code 50;
- 222 hủy thêm ngữ cảnh trả theo mục 14;
- 228 với ngữ cảnh chưa kích hoạt trả code 65.

**Architecture:**
- Dải ngữ cảnh do hằng trong `app_scene.h` quyết định. Trần chung của CWSL do `APP_CWSL_CMD_ID_MAX` trong `app_cwsl.h` quyết định. Parser UART dùng lại hai hằng này.
- `value` đi theo đường: `app_scene_add` kiểm → `app_cwsl_start_scene_learn` cất vào FSM → `app_cwsl_on_info` gửi LEARN 4 phần tử qua `aiio_ci13162_uart_send_cwsl`.
- Bản tin 222 khi hủy dùng lại `app_cwsl_publish_mutating_final` (đã có format `cmd_id, slot, code, u, act_id`).

**Tech Stack:** C (gnu11), FreeRTOS, BL602 SDK, cJSON. Host test bằng gcc với mock trong `smarthome_devices/test_mocks/`.

**Spec:** `docs/voice/luong_mqtt_227_them_ngu_canh_moi.md`. Contract chip: `docs/voice/commands.md` mục 2.1, 3.1, 4.

## Global Constraints

- Repo `/home/taodc/wb2/ai-wb2-baremetal`, branch `voiceboxv1/taodc/dev_261102`.
- **Không commit.** Người dùng tự commit. Không sửa SDK `~/wb2/Ai-Thinker-WB2`.
- `cmd_id` ngữ cảnh: 56..70 (ngữ cảnh 1..15), dùng chung cho MQTT, UART và BLE ADV. **Không đổi số.**
- `bit scene_mask = cmd_id - 56`. Mask 15 bit, `0x7FFF`. Bit 0..9 giữ nguyên ý nghĩa cũ.
- `value` hợp lệ: `0, 1, 2, 3, 11..19, 40..47, 50..68, 70, 71`. Key JSON là `"value"`, không bắt buộc.
- Mã: đủ ngữ cảnh → `50`; `value` sai → `64`; 228 chưa kích hoạt → `65`.
- Bản tin 222 khi kết thúc phiên thêm ngữ cảnh đã hủy: `{"<type>":222,"cmd_id":<id>,"slot":<slot>,"code":<code>,"u":<u>,"act_id":<act_id>}`, không có `result`.
- Không đổi:
  - BLE ADV (vẫn là `cmd_id` thật);
  - 224 cho lệnh âm lượng (vẫn gửi);
  - 220/221/223 trên VOICEBOX (chỉ `cmd_id` 1);
  - format reject của 222 (`u, act_id, result:0, code`).
- Code mới theo style file đang sửa: hậu tố `U`, comment tiếng Anh, không thêm warning.
- Lệnh test host (chạy trong `smarthome_devices/`):
  ```bash
  OUT=/tmp/claude-1000/-home-taodc-wb2-ai-wb2-baremetal/9902f931-8544-4d1f-a8ab-0144142a38b1/scratchpad
  INC="-Itest_mocks -I. -Icomponents/app_config -Icomponents/application -Icomponents/hardware -Icomponents/third_party/cJson -Icomponents/middle/ota -Icomponents/middle/fan -Icomponents/middle/gpio -Icomponents/middle/mqtt -Icomponents/middle/database -Icomponents/middle/wifi -Icomponents/middle/ble_slave -Icomponents/middle/event_driven -Icomponents/middle/http"
  gcc -std=gnu11 -w $INC <runner>.c -o $OUT/<runner> -lm && $OUT/<runner>
  ```
  `<runner>` ∈ `test_app_scene_runner`, `test_app_cwsl_runner`, `test_uart_protocol_runner`, `test_voicebox_runner`. Kết quả đạt: exit 0, dòng cuối `ALL <N> ... PASSED 100%!`. Riêng `test_app_cwsl_runner` phải in `[CWSL][TEST] validation total=<N> failed=0`.
- Build firmware: `cd smarthome_devices && BL60X_SDK_PATH=$HOME/wb2/Ai-Thinker-WB2/ CONFIG_CHIP_NAME=BL602 make -j16`, exit 0.
- Khi thêm khối test vào runner, cập nhật số trong dòng tổng kết `ALL <N> ...`.

## Review Focus

1. Thiết bị đang chạy firmware cũ có `scene_mask` 10 bit (ví dụ `0x03FF`). Sau khi nâng cấp, ngữ cảnh 1..10 vẫn còn, và 227 tiếp theo cấp 66. Test ở Task 1.
2. `value` có mặt nhưng sai kiểu: chuỗi `"47"`, `47.5`, `-1`, `256`, `true`, `null`. Mỗi trường hợp trả 64 và không gọi FSM. Test ở Task 2.
3. Hủy 222 ở pha QUERY_OLD, sau đó chip mới trả `cmd:6`. Bản tin muộn này phải bị bỏ: không gửi LEARN, không có bản tin 227/222 thứ hai. Test ở Task 3.
4. 228 `cmd_id` 255 khi mask có bit 10..14. Vòng xóa phải chạy tới 70 và xóa sạch mask. Test ở Task 1.
5. 229 khi đủ 15 ngữ cảnh: liệt kê đủ 56..70, `total` 15, không bị cắt buffer. Test ở Task 1.

---

### Task 1: Mở rộng dải ngữ cảnh lên 56..70 (15 ngữ cảnh)

**Files:**
- Modify: `smarthome_devices/components/application/app_scene.h`
  - `APP_SCENE_COUNT_MAX` 10U → 15U;
  - `APP_SCENE_CMD_ID_MAX` 65U → 70U;
  - thêm `APP_SCENE_MASK_ALL`;
  - sửa comment 58..67 / 1..10 thành 56..70 / 1..15.
- Modify: `smarthome_devices/components/application/app_scene.c`:
  - `:151`: `&= 0x03FFU` → `&= APP_SCENE_MASK_ALL`;
  - `:218`: kiểm đầy bằng `s_app_scene_mask == APP_SCENE_MASK_ALL`;
  - test nhúng `:483,487`: dùng 16U và 71U làm biên.
- Modify: `smarthome_devices/components/application/app_cwsl.h:16`: `APP_CWSL_CMD_ID_MAX` 65U → 70U. Sửa comment 58..67 ở `:85-96`.
- Modify: `smarthome_devices/components/application/app_cwsl.c`, bảng test nhúng `app_cwsl_run_validation_tests` (khoảng `:770-840`):
  - LEARN 66 slot 0 → `APP_CWSL_CODE_BAD_REQUEST`;
  - thêm LEARN và DELETE 70 → `BAD_REQUEST`;
  - LEARN 71 → `BAD_COMMAND_ID`;
  - QUERY 66 → 0, thêm QUERY 70 → 0, QUERY 71 → `BAD_COMMAND_ID`.
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.c`:
  - `#include "app_scene.h"`;
  - validate `AIIO_CI13162_CMD_SCENE` (`:745`): `data[0]` 1..`APP_SCENE_COUNT_MAX`, `cmd_id == 55U + data[0]`;
  - bảng parser test nhúng (`:1203`): `[11]/66` → `PARSE_OK`, thêm `[15]/70` OK, `[16]/71` `INVALID_DATA`, `{"cmd":6,"data":[70,1,0]}` OK, `{"cmd":6,"data":[71,1,0]}` `INVALID_DATA`.
- Test: `smarthome_devices/test_app_scene_runner.c`, `test_app_cwsl_runner.c`, `test_uart_protocol_runner.c`.

**Interfaces:**
- Produces:
  - `APP_SCENE_CMD_ID_MIN 56U`, `APP_SCENE_CMD_ID_MAX 70U`, `APP_SCENE_COUNT_MAX 15U`;
  - `#define APP_SCENE_MASK_ALL ((uint16_t)((1U << APP_SCENE_COUNT_MAX) - 1U))`;
  - `APP_CWSL_CMD_ID_MAX 70U`.
  - Các hàm public giữ nguyên chữ ký.

- [ ] **Step 1: Viết test (fail) trong `test_app_scene_runner.c`**
  - Test 1 thêm:
    - `assert(app_scene_is_active(16) == false);`
    - `assert(app_scene_is_cmd_id_active(70) == false);`
    - `assert(app_scene_is_cmd_id_active(71) == false);`
  - Khối mới "Test 14 - 15 scenes":
    - (a) `mock_flash_mask = 0x4000; app_scene_init();` → `app_scene_is_active(15) == true`, `app_scene_is_cmd_id_active(70) == true`.
    - (b) Mask cũ 10 bit: `mock_flash_mask = 0x03FF; app_scene_init();` → `app_scene_is_active(10) == true`, `app_scene_is_active(11) == false`. Sau đó `app_scene_add({"fanoem":227,"u":1,"act_id":0})` → `mock_cwsl_learn_last_cmd_id == 66`.
    - (c) `mock_flash_mask = 0x7FFF; app_scene_init();` rồi query `{"fanoem":229,"u":1,"act_id":0}` → `strstr(last_published, "\"scenes\":[56,57,58,59,60,61,62,63,64,65,66,67,68,69,70]")` và `"\"total\":15"`.
    - (d) `mock_flash_mask = 0x7FFF;` add → reject, `publish_count == 1`, `"\"result\":0"`. Task 1 vẫn là code 71; Task 2 đổi thành 50.
  - Test 3: đổi `mock_flash_mask = 0x03FF` thành `0x7FFF`.
  - Test 7: vòng `58..64` thành `58..69`; bước cuối `app_scene_on_delete_result(51, 70)`.
  - Test 8: vòng `56..65` thành `56..70`.
  - Thêm vào Test 7 (Review Focus 4): mask khởi đầu `0x4005` → sau khi xong `mock_flash_mask == 0`.

- [ ] **Step 2: Viết test (fail) trong `test_app_cwsl_runner.c`**
  - Test 2:
    - 220 `cmd_id` 66 slot 0 → `"\"code\":64"`;
    - thêm 220 `cmd_id` 70 slot 255 → `"\"code\":64"`;
    - thêm 220 `cmd_id` 71 slot 0 → `"\"code\":65"`.
  - Test 5:
    - "68 must not publish" đổi thành 71 → `s_publish_count == 0`;
    - thêm `app_cwsl_notify_voice_cmd(70U, 255U)` → `strcmp(s_last_published, "{\"fanoem\":224,\"cmd_id\":70,\"cwsl\":255}") == 0`.
  - Test 6 (voiceboxv1) thêm `app_cwsl_notify_voice_cmd(66U, 2U)` → `"{\"voiceboxv1\":224,\"cmd_id\":66,\"cwsl\":2}"` và `s_mock_ble_beacon_cmd_id == 66U`.

- [ ] **Step 3: Viết test (fail) trong `test_uart_protocol_runner.c`**

  Thêm khối "Test 15 - Scene 1..15 parser":
  ```c
  aiio_ci13162_message_t msg;
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[11],\"cmd_id\":66,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_OK);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[15],\"cmd_id\":70,\"cwsl\":3}", &msg) == AIIO_CI13162_PARSE_OK);
  assert(msg.cmd_id == 70U && msg.cwsl == 3U && msg.data[0] == 15U);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[16],\"cmd_id\":71,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[15],\"cmd_id\":69,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":6,\"data\":[70,1,0]}", &msg) == AIIO_CI13162_PARSE_OK);
  assert(aiio_ci13162_parse_line("{\"cmd\":6,\"data\":[71,1,0]}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  ```

- [ ] **Step 4: Chạy 3 runner, xác nhận fail ở các assert mới**

- [ ] **Step 5: Sửa code theo phần Files ở trên**

- [ ] **Step 6: Chạy cả 4 runner, xác nhận đạt.** `test_app_cwsl_runner` in `failed=0`.

- [ ] **Step 7: Không commit.** Báo danh sách file đã sửa.

---

### Task 2: 227 nhận `value` tùy chọn và trả code 50 khi đủ ngữ cảnh

**Files:**
- Modify: `smarthome_devices/components/application/app_scene.h`: `APP_SCENE_CODE_FULL` 71U → 50U.
- Modify: `smarthome_devices/components/application/app_scene.c`:
  - thêm `#define APP_SCENE_KEY_VALUE "value"`;
  - thêm `static bool app_scene_value_is_valid(uint32_t value)`, trả true cho đúng tập ở Global Constraints;
  - trong `app_scene_add`, ngay sau khối kiểm `cmd_type`/`u`/`act_id` (`:205-211`) và **trước** `app_scene_is_pending()`:
    - nếu `cJSON_GetObjectItem(js_root, APP_SCENE_KEY_VALUE) != NULL` thì đọc bằng `app_scene_read_number(js_root, APP_SCENE_KEY_VALUE, UINT8_MAX, &value)`;
    - đọc lỗi hoặc `!app_scene_value_is_valid(value)` → `app_scene_publish_add_reject(u, act_id, 64U)` và return;
  - gọi `app_cwsl_start_scene_learn(candidate_cmd_id, u, (uint8_t)act_id, has_value, (uint8_t)value)`.
- Modify: `smarthome_devices/components/application/app_cwsl.h`: đổi chữ ký `app_cwsl_start_scene_learn` (xem Interfaces), cập nhật doc comment.
- Modify: `smarthome_devices/components/application/app_cwsl.c`:
  - `app_cwsl_mutating_t` thêm `uint8_t has_value;` và `uint8_t value;`;
  - `app_cwsl_start_scene_learn` lưu hai trường này;
  - `app_cwsl_start_mutating` và `app_cwsl_start_scene_delete` đặt `has_value = 0U`;
  - trong `app_cwsl_on_info` (`:665-676`), LEARN gửi `[cmd_id, 255, value]` với `data_count` 3 khi `has_value != 0U`, ngược lại `[cmd_id, 255]` với `data_count` 2.
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.c` `aiio_ci13162_uart_send_cwsl` (`:2197-2207`):
  - action 1 nhận `data_count` 2 hoặc 3; với 3 thì tạo `{"cmd":4,"data":[1,%u,%u,%u]}\r\n`;
  - action 2 vẫn bắt buộc 2.
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.h`: comment của `aiio_ci13162_uart_send_cwsl` ghi LEARN nhận thêm `value`.
- Test:
  - `test_app_scene_runner.c`: mock `app_cwsl_start_scene_learn` theo chữ ký mới, ghi lại `has_value`, `value`, số lần gọi;
  - `test_app_cwsl_runner.c`: mock `app_scene_on_learn_result` ghi lại `count`, `code`, `cmd_id`;
  - `test_uart_protocol_runner.c`.

**Interfaces:**
- Consumes: hằng từ Task 1.
- Produces: `uint8_t app_cwsl_start_scene_learn(uint8_t cmd_id, uint32_t user_id, uint8_t act_id, bool has_value, uint8_t value);` (0 khi bắt đầu được, còn lại là mã lỗi như cũ).
- `int32_t aiio_ci13162_uart_send_cwsl(uint8_t action, const uint8_t *data, uint8_t data_count);` giữ nguyên chữ ký; LEARN nhận thêm `data_count == 3`.

- [ ] **Step 1: Viết test (fail) trong `test_app_scene_runner.c`**

  Khối mới "Test 15 - value":
  - `{"fanoem":227,"u":1,"act_id":0,"value":47}` → learn được gọi 1 lần, `mock_cwsl_learn_has_value == true`, `mock_cwsl_learn_value == 47`, `publish_count == 0`.
  - Không có key `value` → `mock_cwsl_learn_has_value == false`.
  - `value` 0, 1, 2, 3, 11, 19, 40, 47, 50, 68, 70, 71: mỗi giá trị cho learn được gọi với đúng `value` đó. Chạy `reset_mocks()` giữa các lần.
  - Sai giá trị: `4, 10, 20, 39, 48, 49, 69, 72, 99, 255, 256, -1`, và sai kiểu: `47.5`, `"47"`, `true`, `null`. Với mỗi trường hợp:
    ```c
    assert(publish_count == 1);
    assert(strstr(last_published, "\"result\":0") != NULL);
    assert(strstr(last_published, "\"code\":64") != NULL);
    assert(mock_cwsl_learn_call_count == 0);
    ```
  - Test 3 và Test 14(d): `"\"code\":71"` → `"\"code\":50"`.

- [ ] **Step 2: Viết test (fail) trong `test_app_cwsl_runner.c`**

  Khối mới "Test 7 - Scene add FSM with value", `device_type = 10`:
  ```c
  assert(app_cwsl_start_scene_learn(56U, 101U, 1U, true, 47U) == 0U);
  assert(s_mock_uart_action == APP_CWSL_ACTION_QUERY && s_mock_uart_data_count == 1 && s_mock_uart_data[0] == 56U);
  uint8_t empty_bitmap[2] = {0U, 0U};
  s_registered_info_cb(56U, empty_bitmap, 2U);
  assert(s_mock_uart_action == APP_CWSL_ACTION_LEARN && s_mock_uart_data_count == 3);
  assert(s_mock_uart_data[0] == 56U && s_mock_uart_data[1] == 255U && s_mock_uart_data[2] == 47U);
  s_registered_status_cb(32U, 56U, 3U);
  assert(s_scene_learn_result_count == 1 && s_scene_learn_result_code == 32U && s_scene_learn_result_cmd_id == 56U);
  ```
  Chạy lại với `app_cwsl_start_scene_learn(57U, 101U, 1U, false, 0U)`: sau info callback → `s_mock_uart_data_count == 2`, data `{57, 255}`.

- [ ] **Step 3: Viết test (fail) trong `test_uart_protocol_runner.c`**

  Khối mới "Test 16 - CWSL LEARN with value", sau `reset_test_state()` (hàm này đặt sẵn `s_aiio_ci13162_command_queue`, nên không cần gọi init):
  ```c
  uint8_t learn3[3] = {56U, 255U, 47U};
  uint8_t learn2[2] = {56U, 255U};
  assert(aiio_ci13162_uart_send_cwsl(1U, learn3, 3U) == 0);
  assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":4,\"data\":[1,56,255,47]}\r\n") == 0);
  assert(aiio_ci13162_uart_send_cwsl(1U, learn2, 2U) == 0);
  assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":4,\"data\":[1,56,255]}\r\n") == 0);
  assert(aiio_ci13162_uart_send_cwsl(2U, learn3, 3U) == -1);
  assert(aiio_ci13162_uart_send_cwsl(1U, learn3, 4U) == -1);
  ```

- [ ] **Step 4: Chạy 3 runner, xác nhận fail ở các assert mới**

- [ ] **Step 5: Sửa code theo phần Files ở trên**

- [ ] **Step 6: Chạy cả 4 runner, xác nhận đạt**

- [ ] **Step 7: Không commit.** Báo danh sách file đã sửa.

---

### Task 3: 222 hủy thêm ngữ cảnh theo mục 14, và 228 chưa kích hoạt trả 65

**Files:**
- Modify: `smarthome_devices/components/application/app_cwsl.c`. Thay mọi lời gọi `app_cwsl_publish_stop_response(...)` bằng `app_cwsl_publish_mutating_final(&pending, <cmd_id>, <slot>, <code>)`. Bản sao `pending` có `stop_sent != 0U` nên `mqtt_cmd` là 222. Các chỗ cần thay:
  - `app_cwsl_stop`, nhánh QUERY_OLD (`:1089`): `cmd_id = pending.request.cmd_id`, `slot = APP_CWSL_ANY`, `code = APP_CWSL_CODE_EXIT_OK`;
  - `app_cwsl_on_status`, nhánh LEARNING khi `stop_sent` (`:574`): `cmd_id`, `slot` và `code` lấy từ frame chip;
  - `app_cwsl_on_info`, nhánh `stop_sent` (`:655`): `APP_CWSL_ANY`, `APP_CWSL_CODE_EXIT_OK`;
  - `app_cwsl_poll`, LEARNING timeout khi `stop_sent` (`:1205`): `APP_CWSL_ANY`, `APP_CWSL_CODE_TIMEOUT`.

  Sau đó xóa hàm `app_cwsl_publish_stop_response` vì không còn ai gọi. Thứ tự giữ nguyên: `app_scene_on_learn_result(...)` trước, bản tin 222 sau.
- Modify: `smarthome_devices/components/application/app_scene.c` `app_scene_delete`: sau khối `app_scene_is_pending()` (`:298-301`) và trước nhánh xóa một ngữ cảnh, nếu `!app_scene_is_cmd_id_active((uint8_t)cmd_id)` thì `app_scene_publish_delete_reject((uint8_t)cmd_id, u, (uint8_t)act_id, 65U)` và return. `cmd_id` 255 không qua bước này.
- Test: `test_app_cwsl_runner.c`, `test_app_scene_runner.c`.

**Interfaces:**
- Consumes: `app_cwsl_start_scene_learn(..., bool has_value, uint8_t value)` từ Task 2. Mock `s_scene_learn_result_*` từ Task 2.
- Produces: không có hàm public mới.

- [ ] **Step 1: Viết test (fail) trong `test_app_cwsl_runner.c`**

  Khối mới "Test 8 - Scene add cancel (222)", `device_type = 10`. `STOP` là `{"voiceboxv1":222,"u":9,"act_id":2}`.
  - (a) Hủy ở QUERY_OLD: `start(56,101,1,false,0)`, ghi lại `s_uart_send_count`, gọi `app_cwsl_stop(STOP)`:
    ```c
    assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"cmd_id\":56,\"slot\":255,\"code\":34,\"u\":9,\"act_id\":2}") == 0);
    assert(s_scene_learn_result_code == 34U);
    ```
    `s_uart_send_count` không tăng. Sau đó `s_registered_info_cb(56U, empty_bitmap, 2U)` → `s_uart_send_count` và `s_publish_count` không đổi (Review Focus 3).
  - (b) Hủy ở LEARNING: start, info callback, rồi `app_cwsl_stop(STOP)` → `s_mock_uart_action == APP_CWSL_ACTION_STOP`. Sau đó `s_registered_status_cb(34U, 56U, 3U)`:
    ```c
    assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"cmd_id\":56,\"slot\":3,\"code\":34,\"u\":9,\"act_id\":2}") == 0);
    ```
  - (c) Như (b), nhưng chip trả `s_registered_status_cb(48U, 56U, 255U)`:
    ```c
    assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"cmd_id\":56,\"slot\":255,\"code\":48,\"u\":9,\"act_id\":2}") == 0);
    ```
  - (d) Như (b), nhưng không có frame chip: `s_mock_tick = 120000; app_cwsl_poll();`:
    ```c
    assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"cmd_id\":56,\"slot\":255,\"code\":52,\"u\":9,\"act_id\":2}") == 0);
    assert(s_scene_learn_result_code == 52U);
    ```
  - (e) Hủy ở DELETING_OLD: start, `info_cb(56U, (uint8_t[]){0x01U, 0x00U}, 2U)`, `status_cb(32U, 56U, 3U)` → `s_mock_uart_action == APP_CWSL_ACTION_DELETE`. Rồi `app_cwsl_stop(STOP)`:
    ```c
    assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"u\":9,\"act_id\":2,\"result\":0,\"code\":67}") == 0);
    ```
  - Với (a) đến (d): `strstr(s_last_published, "\"result\"") == NULL`.

- [ ] **Step 2: Viết test (fail) trong `test_app_scene_runner.c`**
  - Khối mới "Test 16 - 228 inactive":
    - `mock_flash_mask = 0x0001; app_scene_init();` → delete `{"fanoem":228,"cmd_id":57,"u":4,"act_id":1}` → `strcmp(last_published, "{\"fanoem\":228,\"cmd_id\":57,\"u\":4,\"act_id\":1,\"result\":0,\"code\":65}") == 0` và `mock_cwsl_delete_call_count == 0`.
    - `mock_flash_mask = 0x0400; app_scene_init();` → delete `cmd_id` 66 → `mock_cwsl_delete_last_cmd_id == 66`.
  - Test 6, phần ba: trước khi xóa 57 lần nữa, đặt `mock_flash_mask = 0x0002; app_scene_init();` để 57 đang kích hoạt. Kỳ vọng giữ nguyên: chip trả 70 → reject 70.

- [ ] **Step 3: Chạy 2 runner, xác nhận fail ở các assert mới**

- [ ] **Step 4: Sửa code theo phần Files ở trên**

- [ ] **Step 5: Chạy cả 4 runner, xác nhận đạt.** Build firmware không có warning `unused function`.

- [ ] **Step 6: Không commit.** Báo danh sách file đã sửa.

---

### Task 4: Tài liệu và kiểm tra cuối

**Files:**
- Modify: `docs/voice/uart_wb2_ci13162.md`. Sửa đúng các dòng mô tả code:
  - `:56`: LEARN gửi thêm `value` khi request 227 có key `value`;
  - `:73`: `56..70` qua MQTT 220/221 bị từ chối;
  - `:91`: `12 SCENE` `[1..15]`, `55 + data` (56..70);
  - `:99`: bỏ `cmd_id` lớn hơn 70;
  - `:174`: ngữ cảnh 1..15 được nhận; lệnh nhà 71..103 vẫn bị từ chối; notify bỏ `cmd_id` > 70;
  - `:179`: LEARN gửi 4 phần tử khi 227 có `value`, các đường khác vẫn 3 phần tử.
- Modify: `docs/voice/luong_mqtt_227_them_ngu_canh_moi.md`. Dòng trạng thái đổi thành `Đã code ngày 2026-11-02, chưa kiểm trên board.`
- Modify: `docs/voice/luong_mqtt_227_them_ngu_canh.md`. Thêm một dòng đầu file: `> Đây là luồng trước khi sửa (10 ngữ cảnh, không có value). Luồng hiện hành: luong_mqtt_227_them_ngu_canh_moi.md.`

**Interfaces:** không có.

- [ ] **Step 1: Sửa 3 file tài liệu như trên**
- [ ] **Step 2: Chạy cả 4 runner, xác nhận đạt và `failed=0`**
- [ ] **Step 3: Build firmware, exit 0.** `grep -E "warning" ` trên log build cho các file `app_scene.c`, `app_cwsl.c`, `aiio_ci13162_uart.c` không có warning mới.
- [ ] **Step 4: Không commit.** Liệt kê việc kiểm trên board cho người dùng:
  - 227 kèm `value` 47 học xong; nói câu đã học thì chip đáp câu 47 và WB2 gửi 224 `cmd_id` 56;
  - thêm ngữ cảnh thứ 11 nhận `cmd_id` 66; chip học được, và nói câu đó thì có 224 `cmd_id` 66;
  - 222 hủy ở hai pha trả đúng format;
  - 228 với ngữ cảnh chưa kích hoạt trả 65.

## Không làm

- `CWSL_MQTT_SPEC.md` và `hnn_voiceboxv1.xlsx`. Cả hai còn dải 46..55 cũ.
- Firmware chip.
- 220/221/223. BLE ADV. 224 cho lệnh âm lượng.
- Các lỗi cũ ở mục 11 của file đề xuất:
  - không có ACK;
  - mẫu rác khi lưu flash lỗi;
  - xóa slot cũ lỗi vẫn báo thành công;
  - cmd5 32 đến trong QUERY_OLD;
  - STOP đụng LEARN_OK.
