# Plan: MQTT 227 format mới — `cmd_id` 255 thêm / 56..70 sửa `respond_id`

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

Trạng thái: **đã duyệt** ngày 2026-11-03. Người dùng chọn chạy bằng subagent.

**Goal:** Request 227 thành `{"voiceboxv1":227,"u":..,"cmd_id":255|56..70,"act_id":..,"respond_id":..}`:
- `cmd_id` 255: thêm ngữ cảnh mới, học giọng và gán `respond_id`;
- `cmd_id` 56..70: chỉ đổi câu phản hồi của ngữ cảnh đó, qua UART cmd 15.

**Architecture:**
- **Tầng UART** (`aiio_ci13162_uart.c`): thêm gửi `{"cmd":15,...}` và đọc frame trả lời `cmd:15`, báo kết quả qua callback.
- **Tầng CWSL** (`app_cwsl.c`):
  - giữ một phiên "đổi câu phản hồi" đang chờ, có timeout 2 s;
  - phiên này tính là bận cho mọi lệnh CWSL và âm lượng;
  - báo kết quả lên `app_scene_on_resp_set_result()`.
- **Tầng ngữ cảnh** (`app_scene.c`): đọc request mới, rẽ nhánh thêm/sửa, và tạo bản tin 227 có `cmd_id` và `respond_id`.

**Tech Stack:** C (gnu11), FreeRTOS, BL602 SDK, cJSON. Host test bằng gcc với mock trong `smarthome_devices/test_mocks/`.

**Spec:**
- `conversation/claude/004_luong-mqtt-227/2026-11-03/001_lam-ro-yeu-cau-227-respond-id.md` (các quyết định của người dùng);
- contract chip: `docs/voice/commands.md` mục 3 (`CMD_RESP_SET`, cmd 15) và mục 4.2 (LEARN 4 phần tử);
- luồng 227 trước plan này: `docs/voice/luong_mqtt_227_them_ngu_canh_moi.md`.

## Global Constraints

- Repo `/home/taodc/wb2/ai-wb2-baremetal`, branch `voiceboxv1/taodc/dev_261102`. **Không commit.** Không sửa SDK.
- Request: `{"<type>":227,"u":<0..UINT32_MAX>,"cmd_id":<0..255>,"act_id":<0..255>,"respond_id":<0..255>}`. `cmd_id` và `respond_id` bắt buộc. Key `value` không còn ý nghĩa: bỏ qua như key lạ.
- `respond_id` hợp lệ: `0, 1, 2, 3, 11..19, 40..47, 50..68, 70, 71`.
- Bản tin 227:
  - thành công: `{"<type>":227,"cmd_id":%u,"respond_id":%u,"u":%lu,"act_id":%u,"result":1}`;
  - lỗi: `{"<type>":227,"cmd_id":%u,"respond_id":%u,"u":%lu,"act_id":%u,"result":0,"code":%u}`.
- Giá trị `cmd_id`/`respond_id` trong bản tin lỗi:
  - đọc được là số nguyên 0..255 thì ghi lại đúng số đó, kể cả khi không hợp lệ;
  - thiếu hoặc sai kiểu thì ghi 0;
  - khi thêm mới bị lỗi trước lúc bắt đầu học thì `cmd_id` là 255; sau khi đã bắt đầu học thì là số đã cấp.
- Mã: request sai (`cmd_id`/`respond_id`/`u`/`act_id`) → 64; `cmd_id` không phải 255 và ngoài 56..70 → 65; sửa ngữ cảnh chưa kích hoạt → 65; đủ ngữ cảnh → 50; bận → 67; OTA → 69; gửi UART lỗi → 70.
- Sửa: gửi `{"cmd":15,"data":[<cmd_id>,<respond_id>]}\r\n`. Chip trả `[1]` → thành công; `[0]` → 64; không trả lời sau 2000 ms (`AIIO_CI13162_CWSL_QUERY_TIMEOUT_MS`) → 52.
- Thêm mới: luồng học như cũ. Lệnh LEARN luôn có 4 phần tử: `{"cmd":4,"data":[1,<cmd_id>,255,<respond_id>]}`.
- Trong lúc chờ chip trả lời lệnh sửa: 220, 221, 222, 223, 225, 226, 227, 228 nhận 67. 222 trả theo format reject cũ: `{"<type>":222,"u":..,"act_id":..,"result":0,"code":67}`.
- Giữ 2 define người dùng đã thêm trong `components/app_config/system_typedef.h`: `APP_SCENE_KEY_RESPOND_ID "respond_id"`, `APP_SCENE_KEY_CMD_ID "cmd_id"`.
- `test_app_scene_runner.c` bỏ qua `system_typedef.h` (định nghĩa `CJSON_KEYWORD_H__`), nên runner phải tự định nghĩa 2 key này, cạnh các `CJSON_KEY_*` đang có.
- Code mới theo style file đang sửa: hậu tố `U`, comment tiếng Anh, không thêm warning.
- Lệnh test host (chạy trong `smarthome_devices/`):
  ```bash
  OUT=/tmp/claude-1000/-home-taodc-wb2-ai-wb2-baremetal/9902f931-8544-4d1f-a8ab-0144142a38b1/scratchpad/sdd-227r
  mkdir -p $OUT
  INC="-Itest_mocks -I. -Icomponents/app_config -Icomponents/application -Icomponents/hardware -Icomponents/third_party/cJson -Icomponents/middle/ota -Icomponents/middle/fan -Icomponents/middle/gpio -Icomponents/middle/mqtt -Icomponents/middle/database -Icomponents/middle/wifi -Icomponents/middle/ble_slave -Icomponents/middle/event_driven -Icomponents/middle/http"
  gcc -std=gnu11 -w $INC <runner>.c -o $OUT/<runner> -lm && $OUT/<runner>
  ```
  `<runner>` ∈ `test_uart_protocol_runner`, `test_app_cwsl_runner`, `test_app_scene_runner`, `test_voicebox_runner`. Đạt khi exit 0 và dòng cuối là `ALL <N> ... PASSED 100%!`. `test_app_cwsl_runner` còn phải in `[CWSL][TEST] validation total=<N> failed=0`.
- Trước Task 3, `test_app_scene_runner` và build firmware chưa chạy được: phần sửa dở hiện có trong `app_scene_add`, cộng với chữ ký mới ở Task 2, đều chỉ được sửa ở Task 3. Task 1–2 chạy 3 runner còn lại.
- Build firmware: `cd smarthome_devices && BL60X_SDK_PATH=$HOME/wb2/Ai-Thinker-WB2/ CONFIG_CHIP_NAME=BL602 make -j16`, đạt khi exit 0.
- Khi thêm khối test, cập nhật số trong dòng `ALL <N> ...`.

## Review Focus

1. `respond_id` sai kiểu: `"47"`, `47.5`, `-1`, `256`, `null`. Cả thêm lẫn sửa đều trả 64 và không gửi UART. Test ở Task 3.
2. App cũ không gửi `cmd_id`, hoặc chỉ gửi `value`: trả 64, không học, không gửi UART. Test ở Task 3.
3. Chip trả `cmd:15` muộn, sau khi đã timeout: bản tin này bị bỏ, không có bản tin 227 thứ hai. Test ở Task 2.
4. Sửa ngữ cảnh trong lúc đang xóa tất cả (228 255): trả 67, không gửi UART. Test ở Task 3.
5. 222 đến khi đang chờ lệnh sửa: trả 67, không phải 68 (`NOT_IN_LEARN_MODE`), và không gửi STOP. Test ở Task 2.

---

### Task 1: UART cmd 15 (CMD_RESP_SET): gửi, đọc trả lời, callback

**Files:**
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.h`:
  - thêm `typedef void (*aiio_ci13162_resp_set_cb_t)(uint8_t applied);`;
  - thêm 2 khai báo ở mục Interfaces, kèm doc comment.
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.c`:
  - enum lệnh (`:66-82`) thêm `AIIO_CI13162_CMD_RESP_SET = 15,`;
  - `aiio_ci13162_validate_message`: `case AIIO_CI13162_CMD_RESP_SET:` OK khi `data_count == 1U && data[0] <= 1U`, ngược lại `PARSE_INVALID_DATA`. Đây là frame trả lời, nên không được có `cmd_id`/`cwsl`; quy tắc metadata hiện có đã xử lý việc này;
  - `aiio_ci13162_uart_process`: ngay sau khối `AIIO_CI13162_CMD_CWSL_INFO` (`:1869`), thêm khối `AIIO_CI13162_CMD_RESP_SET`: gọi callback nếu khác NULL với `message.data[0]`, rồi `continue`;
  - biến static `s_aiio_ci13162_resp_set_cb`;
  - hàm gửi và hàm đặt callback, theo mẫu `aiio_ci13162_uart_send_cwsl`/`aiio_ci13162_uart_set_cwsl_callbacks`, có nhánh `#if AIIO_CI13162_UART_ENABLE == 1 ... #else`.
- Test: `smarthome_devices/test_uart_protocol_runner.c`.

**Interfaces:**
- Produces:
  - `int32_t aiio_ci13162_uart_send_resp_set(uint8_t cmd_id, uint8_t value);`
    - gửi đúng `{"cmd":15,"data":[<cmd_id>,<value>]}\r\n`;
    - trả 0 khi gửi đủ;
    - trả -1 khi command queue NULL, đang OTA (`s_aiio_ci13162_ota_held != 0U`), hoặc gửi thiếu.
  - `void aiio_ci13162_uart_set_resp_set_callback(aiio_ci13162_resp_set_cb_t cb);` — `applied` là `data[0]` của frame chip (1 = đã áp, 0 = từ chối).
  - `AIIO_CI13162_CMD_RESP_SET = 15`.

- [ ] **Step 1: Viết test (fail).** Khối mới "Test 17 - CMD_RESP_SET (cmd 15)". Trong runner thêm callback test `static int s_resp_set_cb_count; static uint8_t s_resp_set_cb_last; static void test_resp_set_cb(uint8_t applied)`:
  ```c
  reset_test_state();
  assert(aiio_ci13162_uart_send_resp_set(56U, 47U) == 0);
  assert(strcmp(s_uart_tx_buf, "{\"cmd\":15,\"data\":[56,47]}\r\n") == 0);
  s_aiio_ci13162_ota_held = 1U;
  assert(aiio_ci13162_uart_send_resp_set(56U, 47U) == -1);
  s_aiio_ci13162_ota_held = 0U;
  aiio_ci13162_message_t msg;
  assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[1]}", &msg) == AIIO_CI13162_PARSE_OK);
  assert(msg.command == AIIO_CI13162_CMD_RESP_SET && msg.data_count == 1U && msg.data[0] == 1U);
  assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[0]}", &msg) == AIIO_CI13162_PARSE_OK);
  assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[2]}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[]}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[1],\"cmd_id\":56,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
  s_resp_set_cb_count = 0;
  aiio_ci13162_uart_set_resp_set_callback(test_resp_set_cb);
  aiio_ci13162_message_t m = {.command = AIIO_CI13162_CMD_RESP_SET, .data_count = 1U, .data = {0U}};
  xQueueSend(NULL, &m, 0U);
  aiio_ci13162_uart_process();
  assert(s_resp_set_cb_count == 1 && s_resp_set_cb_last == 0U);
  aiio_ci13162_uart_set_resp_set_callback(NULL);
  ```
- [ ] **Step 2: Chạy `test_uart_protocol_runner`, xác nhận fail** (lỗi compile vì chưa có hàm hoặc enum).
- [ ] **Step 3: Sửa code theo phần Files ở trên.**
- [ ] **Step 4: Chạy `test_uart_protocol_runner`, `test_app_cwsl_runner`, `test_voicebox_runner`, xác nhận đạt.**
- [ ] **Step 5: Không commit.** Báo danh sách file đã sửa.

---

### Task 2: CWSL — phiên đổi câu phản hồi, LEARN luôn kèm `respond_id`

**Files:**
- Modify: `smarthome_devices/components/application/app_cwsl.h`:
  - đổi chữ ký `app_cwsl_start_scene_learn`;
  - thêm `app_cwsl_start_scene_resp_set`;
  - cập nhật doc comment cả hai, gồm danh sách mã trả về.
- Modify: `smarthome_devices/components/application/app_scene.h`: thêm khai báo `void app_scene_on_resp_set_result(uint8_t code);`. Doc comment: `code` 0 = chip đã áp, 64 = chip từ chối, 52 = timeout.
- Modify: `smarthome_devices/components/application/app_cwsl.c`:
  - `app_cwsl_mutating_t`: bỏ `has_value`/`value`, thêm `uint8_t respond_id;`. `app_cwsl_start_mutating` và `app_cwsl_start_scene_delete` đặt `respond_id = 0U`;
  - `app_cwsl_start_scene_learn` lưu `respond_id`;
  - `app_cwsl_on_info` (QUERY_OLD → LEARNING) luôn gửi `[cmd_id, 255, respond_id]` với `data_count` 3;
  - thêm `typedef struct { uint8_t active; uint8_t cmd_id; TickType_t start_tick; } app_cwsl_resp_set_t;` và `static app_cwsl_resp_set_t s_app_cwsl_resp_set;`;
  - thêm `static void app_cwsl_on_resp_set(uint8_t applied)`:
    - không có phiên chờ thì return;
    - có thì xóa phiên, bật lại hỏi version, rồi gọi `app_scene_on_resp_set_result((applied != 0U) ? 0U : APP_CWSL_CODE_BAD_REQUEST)`;
  - `app_cwsl_init`: memset phiên và gọi `aiio_ci13162_uart_set_resp_set_callback(app_cwsl_on_resp_set)`;
  - `app_cwsl_poll`: phiên chờ quá `AIIO_CI13162_CWSL_QUERY_TIMEOUT_MS` thì xóa phiên, bật lại hỏi version, gọi `app_scene_on_resp_set_result(APP_CWSL_CODE_TIMEOUT)`;
  - bận (67) khi `s_app_cwsl_resp_set.active != 0U`, ở các chỗ sau:
    - `app_cwsl_start_mutating` (220/221, publish reject 67);
    - `app_cwsl_query` (223);
    - `app_cwsl_stop` (222): kiểm **trước** nhánh "không có phiên học → 68";
    - `app_cwsl_start_scene_learn` và `app_cwsl_start_scene_delete`;
  - `app_cwsl_is_pending()` tính cả phiên này. 225/226 dùng hàm này, nên cũng nhận 67.
- Test: `smarthome_devices/test_app_cwsl_runner.c`. Thêm mock:
  - `aiio_ci13162_uart_send_resp_set`: ghi lại số lần gọi, `cmd_id`, `value`; trả `s_mock_resp_set_ret`;
  - `aiio_ci13162_uart_set_resp_set_callback`: lưu `s_registered_resp_set_cb`;
  - `app_scene_on_resp_set_result`: ghi lại `s_scene_resp_set_result_count` và `_code`.

  Reset tất cả trong `reset_cwsl_mocks()`. Sửa Test 7 và Test 8 theo chữ ký mới:
  - Test 7: thay ca "có value 47" bằng `respond_id` 47 (vẫn 3 byte `[56,255,47]`), bỏ ca "không có value";
  - Test 8: thay `(56U, 101U, 1U, false, 0U)` bằng `(56U, 101U, 1U, 47U)`.

**Interfaces:**
- Consumes: `aiio_ci13162_uart_send_resp_set`, `aiio_ci13162_uart_set_resp_set_callback` (Task 1).
- Produces:
  - `uint8_t app_cwsl_start_scene_learn(uint8_t cmd_id, uint32_t user_id, uint8_t act_id, uint8_t respond_id);` — 0, 65, 67, 69, 70 như cũ;
  - `uint8_t app_cwsl_start_scene_resp_set(uint8_t cmd_id, uint8_t respond_id);`
    - trả 65 nếu `cmd_id` ngoài 56..70;
    - trả 69 nếu đang OTA;
    - trả 67 nếu đang chờ âm lượng, có phiên học/xóa, có query, hoặc đã có phiên đổi câu;
    - gửi UART lỗi thì bật lại hỏi version và trả 70;
    - trả 0 khi đã gửi; lúc đó dừng hỏi version và bắt đầu tính timeout;
  - `void app_scene_on_resp_set_result(uint8_t code);` (khai báo ở đây, Task 3 viết thân hàm).

- [ ] **Step 1: Viết test (fail).** Khối mới "Test 9 - Scene respond_id edit", `device_type = 10`:
  - (a) Thành công:
    ```c
    assert(app_cwsl_start_scene_resp_set(56U, 47U) == 0U);
    assert(s_resp_set_send_count == 1 && s_resp_set_last_cmd_id == 56U && s_resp_set_last_value == 47U);
    assert(app_cwsl_is_pending() == true);
    s_registered_resp_set_cb(1U);
    assert(s_scene_resp_set_result_count == 1 && s_scene_resp_set_result_code == 0U);
    assert(app_cwsl_is_pending() == false);
    ```
  - (b) Chip từ chối: start, rồi `s_registered_resp_set_cb(0U)` → `s_scene_resp_set_result_code == 64U`.
  - (c) Timeout (Review Focus 3):
    - start lúc `s_mock_tick = 0`;
    - `s_mock_tick = 1999; app_cwsl_poll();` → `s_scene_resp_set_result_count == 0`;
    - `s_mock_tick = 2000; app_cwsl_poll();` → count 1, code 52;
    - sau đó `s_registered_resp_set_cb(1U)` → count vẫn là 1.
  - (d) Gửi lỗi: `s_mock_resp_set_ret = -1;` → `app_cwsl_start_scene_resp_set(56U, 47U) == 70U` và `app_cwsl_is_pending() == false`.
  - (e) Ngoài dải: `(55U, 47U)` và `(71U, 47U)` đều trả 65, `s_resp_set_send_count == 0`.
  - (f) Bận khi đang chờ lệnh sửa (Review Focus 5). Sau `start(56U, 47U) == 0U`:
    - `app_cwsl_start_scene_learn(57U, 1U, 0U, 47U) == 67U`;
    - `app_cwsl_start_scene_delete(57U, 1U, 0U) == 67U`;
    - `app_cwsl_start_scene_resp_set(57U, 47U) == 67U`;
    - 220 `{"voiceboxv1":220,"cmd_id":1,"slot":255,"u":3,"act_id":0}` → `strstr(s_last_published, "\"code\":67")`;
    - 223 `{"voiceboxv1":223,"cmd_id":1,"u":3,"act_id":0}` → `"code":67`;
    - 222 `{"voiceboxv1":222,"u":3,"act_id":0}` → `strcmp(s_last_published, "{\"voiceboxv1\":222,\"u\":3,\"act_id\":0,\"result\":0,\"code\":67}") == 0`, và không gửi STOP (`s_uart_send_count` không tăng).
  - (g) Bận ngược lại: `app_cwsl_start_scene_learn(56U, 1U, 0U, 47U) == 0U` → `app_cwsl_start_scene_resp_set(57U, 47U) == 67U`.
- [ ] **Step 2: Chạy `test_app_cwsl_runner`, xác nhận fail.**
- [ ] **Step 3: Sửa code theo phần Files ở trên.**
- [ ] **Step 4: Chạy `test_app_cwsl_runner`, `test_uart_protocol_runner`, `test_voicebox_runner`, xác nhận đạt và `failed=0`.**
- [ ] **Step 5: Không commit.** Báo danh sách file đã sửa.

---

### Task 3: 227 đọc request mới, rẽ nhánh thêm/sửa, bản tin có `cmd_id` và `respond_id`

**Files:**
- Modify: `smarthome_devices/components/application/app_scene.h`: thêm `#define APP_SCENE_CMD_ID_NEW 255U`. Sửa doc comment `app_scene_add` theo format mới.
- Modify: `smarthome_devices/components/application/app_scene.c`:
  - đổi tên `app_scene_value_is_valid` thành `app_scene_respond_id_is_valid` (cùng tập giá trị);
  - `app_scene_publish_add_reject(uint8_t cmd_id, uint8_t respond_id, uint32_t u, uint8_t act_id, uint8_t code)` và `app_scene_publish_add_success(uint8_t cmd_id, uint8_t respond_id, uint32_t u, uint8_t act_id)` theo đúng 2 format ở Global Constraints;
  - thêm `static uint8_t s_app_scene_pending_cmd_id;` và `static uint8_t s_app_scene_pending_respond_id;`, reset trong `app_scene_init`;
  - **viết lại toàn bộ thân `app_scene_add`**, thay phần người dùng đang sửa dở. Thứ tự:
    1. Đọc thử `u`, `act_id`, `cmd_id`, `respond_id` để trả lại trong bản tin lỗi. Dùng biến `uint32_t` tạm qua `app_scene_read_number(..., UINT8_MAX hoặc UINT32_MAX, ...)`; đọc lỗi thì giữ 0.
    2. `js_root == NULL` → 64 (mọi số 0).
    3. Có key `action` → 64.
    4. Kiểm `<type>` == 227, `u`, `act_id`, `cmd_id` (bắt buộc, 0..255) và `respond_id` (bắt buộc, 0..255, nằm trong tập hợp lệ). Sai → 64.
    5. `cmd_id != 255` và ngoài 56..70 → 65.
    6. `app_scene_is_pending()` (đang xóa tất cả) → 67.
    7. Nhánh thêm (`cmd_id == 255`):
       - đủ ngữ cảnh → 50;
       - lấy bit trống thấp nhất, gọi `app_cwsl_start_scene_learn(candidate, u, act_id, respond_id)`;
       - trả mã khác 0 thì reject với `cmd_id` 255;
       - bắt đầu được thì lưu uid, act_id, `respond_id`, `cmd_id = candidate`.
    8. Nhánh sửa (56..70):
       - `!app_scene_is_cmd_id_active(cmd_id)` → 65;
       - gọi `app_cwsl_start_scene_resp_set(cmd_id, respond_id)`; trả mã khác 0 thì reject với mã đó;
       - bắt đầu được thì lưu uid, act_id, `respond_id`, `cmd_id`.
  - `app_scene_on_learn_result`: bản tin thành công và lỗi dùng `cmd_id` đã cấp cùng `s_app_scene_pending_respond_id`;
  - thêm `void app_scene_on_resp_set_result(uint8_t code)`: code 0 → success với pending `cmd_id`/`respond_id`/uid/act_id; khác 0 → reject với code đó. Không đổi `scene_mask`.
- `components/app_config/system_typedef.h`: giữ nguyên 2 define của người dùng.
- Test: `smarthome_devices/test_app_scene_runner.c`:
  - thêm `#define APP_SCENE_KEY_RESPOND_ID "respond_id"` và `#define APP_SCENE_KEY_CMD_ID "cmd_id"` cạnh `CJSON_KEY_*`;
  - mock `app_cwsl_start_scene_learn` theo chữ ký 4 tham số, ghi `respond_id` và số lần gọi;
  - mock mới `app_cwsl_start_scene_resp_set`, ghi số lần gọi, `cmd_id`, `respond_id`; trả `mock_cwsl_resp_set_ret`;
  - sửa mọi request 227 thêm mới đang có (Test 2, 3, 4, 8, 13, 14, 15) cho có `"cmd_id":255,"respond_id":47`. Test 5 (key `action`) giữ nguyên kỳ vọng 64;
  - Test 15 (value) đổi thành kiểm `respond_id`.

**Interfaces:**
- Consumes:
  - `app_cwsl_start_scene_learn(uint8_t, uint32_t, uint8_t, uint8_t)`, `app_cwsl_start_scene_resp_set(uint8_t, uint8_t)` từ Task 2;
  - khai báo `app_scene_on_resp_set_result(uint8_t)` từ Task 2.
- Produces: thân `app_scene_on_resp_set_result`; `APP_SCENE_CMD_ID_NEW`.

- [ ] **Step 1: Viết test (fail).** Khối mới "Test 17 - 227 add/edit with respond_id", `device_type = 0` ("fanoem"):
  1. Thêm thành công:
     - `mock_flash_mask = 0x0001; app_scene_init();`
     - request `{"fanoem":227,"u":1001,"cmd_id":255,"act_id":5,"respond_id":47}` → learn được gọi với `cmd_id` 57 và `respond_id` 47; `publish_count == 0`;
     - `app_scene_on_learn_result(32U, 57U)` → `strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":57,\"respond_id\":47,\"u\":1001,\"act_id\":5,\"result\":1}") == 0`.
  2. Học lỗi: lặp ca 1, rồi `app_scene_on_learn_result(48U, 57U)` → `"{\"fanoem\":227,\"cmd_id\":57,\"respond_id\":47,\"u\":1001,\"act_id\":5,\"result\":0,\"code\":48}"`.
  3. Đủ ngữ cảnh: `mock_flash_mask = 0x7FFF`, request `{"fanoem":227,"u":1,"cmd_id":255,"act_id":0,"respond_id":47}` → `"{\"fanoem\":227,\"cmd_id\":255,\"respond_id\":47,\"u\":1,\"act_id\":0,\"result\":0,\"code\":50}"`.
  4. Thiếu `cmd_id`: `{"fanoem":227,"u":1,"act_id":0,"respond_id":47}` → `"{\"fanoem\":227,\"cmd_id\":0,\"respond_id\":47,\"u\":1,\"act_id\":0,\"result\":0,\"code\":64}"`; learn và resp_set đều không được gọi (Review Focus 2).
  5. Thiếu `respond_id`: `{"fanoem":227,"u":1,"cmd_id":255,"act_id":0}` → `"...\"cmd_id\":255,\"respond_id\":0,...\"code\":64}"`. Chỉ có `value`: `{"fanoem":227,"u":1,"cmd_id":255,"act_id":0,"value":47}` → code 64, `"respond_id":0` (Review Focus 2).
  6. `respond_id` sai:
     - giá trị 9 → code 64, bản tin ghi `"respond_id":9`;
     - `"47"`, `47.5`, `-1`, `256`, `null` → code 64, bản tin ghi `"respond_id":0`.

     Áp dụng cho cả `cmd_id` 255 và `cmd_id` 56 (mask `0x0001`); mỗi ca learn/resp_set đều không được gọi (Review Focus 1).
  7. `cmd_id` ngoài dải: 55, 71, 254 → code 65, bản tin ghi lại đúng `cmd_id` đó.
  8. Sửa thành công:
     - `mock_flash_mask = 0x0001`;
     - `{"fanoem":227,"u":2,"cmd_id":56,"act_id":3,"respond_id":50}` → resp_set được gọi với `(56, 50)`, learn không được gọi, `publish_count == 0`;
     - `app_scene_on_resp_set_result(0U)` → `"{\"fanoem\":227,\"cmd_id\":56,\"respond_id\":50,\"u\":2,\"act_id\":3,\"result\":1}"`;
     - `mock_flash_mask == 0x0001`.
  9. Sửa lỗi:
     - lặp ca 8 rồi `app_scene_on_resp_set_result(64U)` → `"...\"cmd_id\":56,\"respond_id\":50,...\"result\":0,\"code\":64}"`;
     - lặp với `52U` → `"code":52`.
  10. Sửa ngữ cảnh chưa kích hoạt: mask `0x0001`, `cmd_id` 57 → `"{\"fanoem\":227,\"cmd_id\":57,\"respond_id\":50,\"u\":2,\"act_id\":3,\"result\":0,\"code\":65}"`; resp_set không được gọi.
  11. Sửa nhưng không bắt đầu được: `mock_cwsl_resp_set_ret = 67` → reject `"cmd_id":56`, `"code":67`.
  12. Sửa trong lúc xóa tất cả: mask `0x0001`, gửi `{"fanoem":228,"cmd_id":255,"u":9,"act_id":0}`, rồi sửa `cmd_id` 56 → `"code":67`; resp_set không được gọi; `app_scene_is_pending() == true` (Review Focus 4).
  13. voiceboxv1: `device_type = 10`, `{"voiceboxv1":227,"u":55519,"cmd_id":255,"act_id":0,"respond_id":1}` → learn được gọi với `respond_id` 1; sau `on_learn_result(32U, 56U)` → `"{\"voiceboxv1\":227,\"cmd_id\":56,\"respond_id\":1,\"u\":55519,\"act_id\":0,\"result\":1}"`. Trả `device_type = 0`.
- [ ] **Step 2: Chạy `test_app_scene_runner`, xác nhận fail.**
- [ ] **Step 3: Sửa code theo phần Files ở trên.**
- [ ] **Step 4: Chạy cả 4 runner, xác nhận đạt.** Build firmware: exit 0, không có warning ở `app_scene.c`, `app_cwsl.c`, `aiio_ci13162_uart.c`.
- [ ] **Step 5: Không commit.** Báo danh sách file đã sửa.

---

### Task 4: Tài liệu và kiểm tra cuối

**Files:**
- Modify: `docs/voice/luong_mqtt_227_them_ngu_canh_moi.md`. Viết lại theo format mới:
  - dòng trạng thái đầu file: thêm `Cập nhật 2026-11-03: format cmd_id/respond_id.`;
  - mục 1: request/response mới; bỏ `value`;
  - mục 3: sơ đồ kiểm tra theo thứ tự ở Task 3;
  - mục 4: ví dụ thêm mới với `respond_id` 47; LEARN `[1,56,255,47]`;
  - thêm mục mới "Sửa câu phản hồi (`cmd_id` 56..70)": sequence `{"cmd":15,"data":[56,50]}` → `{"cmd":15,"data":[1]}` → 227 thành công; ghi các mã 64/52/70/65/67/69;
  - mục 7: bảng mã, thêm ca sửa;
  - bỏ câu "Request không có `value`...";
  - giữ phần 222/228/229/224 và các mục còn đúng.
- Modify: `docs/voice/uart_wb2_ci13162.md`:
  - mục WB2 → chip thêm dòng `{"cmd":15,"data":[<cmd_id>,<value>]}` (chỉ dùng cho 227 sửa);
  - mục chip → WB2 thêm `15 RESP_SET` `[0]/[1]` → `app_cwsl_on_resp_set()`;
  - dòng LEARN: 227 luôn gửi 4 phần tử;
  - bảng khác biệt (dòng `CMD_RESP_SET/GET ... Chưa cài`): RESP_SET đã cài, chỉ cho 227 sửa; RESP_GET, VOICE_SET/GET vẫn chưa.

**Interfaces:** không có.

- [ ] **Step 1: Sửa 2 file tài liệu như trên.** Mọi câu về code phải khớp code sau Task 1–3.
- [ ] **Step 2: Chạy cả 4 runner, xác nhận đạt.** Build firmware: exit 0, không có warning ở 3 file C đã sửa. Báo kích thước `build_out/switchv3.bin`.
- [ ] **Step 3: Không commit.** Liệt kê việc kiểm trên board:
  - thêm mới với `respond_id` 47: nói câu đã học thì chip đáp câu 47;
  - sửa `cmd_id` 56 sang `respond_id` 50: chip trả `{"cmd":15,"data":[1]}`, nói câu ngữ cảnh 1 thì chip đáp câu 50;
  - sửa với `respond_id` mà không giọng nào có: xem chip trả `[0]` và App nhận code 64;
  - sửa ngữ cảnh chưa thêm: App nhận 65.

## Không làm

- Không học lại giọng khi sửa.
- Không đọc câu phản hồi hiện tại (`CMD_RESP_GET`, cmd 16).
- Không làm chọn giọng (cmd 17/18).
- Không sửa 220/221/223, 228/229 (ngoài phần nhận 67 khi bận), BLE ADV, firmware chip, `CWSL_MQTT_SPEC.md`, `hnn_voiceboxv1.xlsx`.
- Các lỗi cũ ở mục 11 của `luong_mqtt_227_them_ngu_canh_moi.md` giữ nguyên.
