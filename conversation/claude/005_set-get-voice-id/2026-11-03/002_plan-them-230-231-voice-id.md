# MQTT 230/231 set/get voice_id Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Cho App đặt giọng (MQTT 230) và đọc giọng (MQTT 231) của VOICEBOX qua UART cmd 17/18 của CI13162, đồng thời đưa `voice_id` vào bản tin trạng thái 100/105.

**Architecture:**
- Driver UART (`aiio_ci13162_uart.c`):
  - gửi và parse cmd 17/18;
  - nhớ voice_id;
  - hỏi cmd 18 một lần sau khi nhận version.
- Module mới `app_voice.c`, làm theo mẫu `app_volume.c`:
  - xử lý 230/231 với một phiên chờ timeout 2 s;
  - có khoảng chờ frame muộn 2 s;
  - chặn hai chiều với CWSL, âm lượng và OTA.
- `process_data.c` nối dispatch, init, poll, chặn OTA và thêm `voice_id` vào 100/105.

**Tech Stack:** C (BL602 SDK, FreeRTOS, cJSON), runner host build bằng gcc với `test_mocks/`.

**Spec:** `conversation/claude/005_set-get-voice-id/2026-11-03/001_lam-ro-yeu-cau-230-231-voice-id.md` (spec phía chip: `docs/voice/commands.md` mục 3.2)

## Global Constraints

- Không commit; người dùng tự commit. Không sửa SDK `~/wb2/Ai-Thinker-WB2`. Không sửa `docs/voice/hnn_voiceboxv1.xlsx`.
- Request:
  - 230: `{"<type>":230,"u":<uint32>,"voice_id":<1..4>,"act_id":<0..255>}`
  - 231: `{"<type>":231,"u":<uint32>,"act_id":<0..255>}`
  - `<type>` là `gs_dev_type[device_type]`. Mọi key đều bắt buộc.
- Reply (đúng thứ tự key, không dấu cách):
  - 230 OK: `{"<type>":230,"u":%lu,"voice_id":%u,"act_id":%u,"result":1}`
  - 230 lỗi: `{"<type>":230,"u":%lu,"voice_id":%u,"act_id":%u,"result":0,"code":%u}`
  - 231 OK: `{"<type>":231,"u":%lu,"voice_id":%u,"act_id":%u,"result":1}`
  - 231 lỗi: `{"<type>":231,"u":%lu,"act_id":%u,"result":0,"code":%u}`
- Key trong reply:
  - `u`, `act_id`, `voice_id` (của 230) lấy từ request;
  - key nào không đọc được (thiếu, không phải số nguyên không âm, hoặc vượt `UINT32_MAX` / `UINT8_MAX` / `UINT8_MAX`) thì in 0;
  - `voice_id` 5 vẫn in 5.
- Mã lỗi:
  - 64: thiếu/sai request, hoặc chip trả `[0]`;
  - 69: đang OTA;
  - 67: đang bận;
  - 70: gửi UART lỗi;
  - 52: chip không trả trong 2 s.
- Thứ tự kiểm: 64 → 69 → 67 → gửi (70).
- Chặn hai chiều:
  - khi 230/231 đang chờ chip hoặc đang trong khoảng chờ frame muộn: 220–223, 227, 228 trả 67; 225/226 trả `result:0`; OTA 129/134 trả `result:0`;
  - 229 chỉ đọc flash của WB2, không đi qua UART, nên không bị chặn (giống 225/226 hiện nay). Đây là lựa chọn của Claude, người dùng cần xác nhận.
- UART:
  - set `{"cmd":17,"data":[v]}\r\n`, chip trả `[1]`/`[0]`;
  - get `{"cmd":18,"data":[]}\r\n`, chip trả `[1..4]`.
- Thời gian:
  - timeout chờ chip 2000 ms, khoảng chờ frame muộn 2000 ms;
  - hỏi lúc boot: lần 1 ngay sau khi nhận version, lần sau cách 1000 ms, tối đa 3 lần.
- 100/105 thêm `"voice_id":%u` ngay sau `"volume":%u`; chưa biết giọng thì in 0.
- Chạy runner:
  ```
  RUNNERS="<tên runner>" bash /home/taodc/wb2/ai-wb2-baremetal/.superpowers/sdd/002_plan-sua-227-respond-id/run-tests.sh
  ```
  - Script build `smarthome_devices/<tên>.c` bằng gcc với include sẵn, rồi in `exit=` và dòng tổng kết.
  - Baseline:
    - `test_uart_protocol_runner`: `ALL 17`;
    - `test_app_cwsl_runner`: `ALL 9`, `validation total=52 failed=0`;
    - `test_app_scene_runner`: 17 test;
    - `test_voicebox_runner`: 9 test.
- Build firmware: `cd smarthome_devices && BL60X_SDK_PATH=$HOME/wb2/Ai-Thinker-WB2/ CONFIG_CHIP_NAME=BL602 make -j16`. Kỳ vọng exit 0, không có warning mới ở file đã sửa.
- Comment trong code viết tiếng Anh. Header Doxygen theo mẫu file bên cạnh. Hằng số có hậu tố `U`.

## Review Focus

1. **Chip trả `cmd:17 [1]` sau 2 s** (sau khi phát xong welcome).
   - App nhận 52, nhưng giọng thật đã đổi. WB2 phải nhớ giọng mới để 100/105/231 nói đúng.
   - Test: Task 1 Test 18 (cache lấy giá trị đã gửi khi `[1]` tới) và Task 2 `late_set_reply_only_ends_drain`.
2. **Frame `cmd:18` của lần hỏi lúc boot tới trong khoảng chờ frame muộn của 230.**
   - Frame đó không được kết thúc khoảng chờ của 230 và không được publish.
   - Test: Task 2 `get_frame_does_not_end_set_drain`.
3. **Chip không hỗ trợ cmd 17/18** (không bao giờ trả lời).
   - Hỏi lúc boot dừng sau 3 lần.
   - 230/231 nhận 52, và sau khoảng chờ thì nhận lệnh lại được.
   - Test: Task 1 Test 19 và Task 2 `timeout_then_drain_expires`.
4. **App bấm 230 hai lần liên tiếp.**
   - Lần 2 nhận 67; chip chỉ nhận một cmd 17.
   - Test: Task 2 `second_set_while_pending_is_busy`.
5. **`voice_id` gửi dạng `"2"`, `2.5`, `300`, `true`.**
   - Trả 64 với `voice_id` 0, không gửi UART.
   - Test: Task 2 `set_rejects_invalid_requests`.

---

### Task 1: Driver UART cmd 17/18, nhớ voice_id, hỏi giọng lúc boot

**Files:**
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_config.h` (thêm sau `AIIO_CI13162_VOLUME_DRAIN_MS`)
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.h`
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.c`:
  - enum lệnh (dòng 71-88);
  - `aiio_ci13162_validate_message` (khoảng dòng 698-886);
  - dispatch trong `aiio_ci13162_uart_process` (khoảng dòng 1939-1990);
  - reset ở init (khoảng dòng 1851-1858);
  - `aiio_ci13162_uart_poll` (dòng 2110).
- Test: `smarthome_devices/test_uart_protocol_runner.c`

**Interfaces:**
- Produces (header `aiio_ci13162_uart.h`):
  ```c
  typedef void (*aiio_ci13162_voice_set_cb_t)(uint8_t applied);   /* chip cmd 17: 1 applied, 0 rejected */
  typedef void (*aiio_ci13162_voice_get_cb_t)(uint8_t voice_id);  /* chip cmd 18: 1..4 */
  int32_t aiio_ci13162_uart_send_voice_set(uint8_t voice_id);     /* 0 sent; -1 voice_id not 1..4, queue NULL, OTA held, short TX */
  int32_t aiio_ci13162_uart_query_voice(void);                    /* 0 sent; -1 queue NULL, OTA held, short TX */
  void aiio_ci13162_uart_set_voice_callbacks(aiio_ci13162_voice_set_cb_t set_cb,
                                             aiio_ci13162_voice_get_cb_t get_cb);
  uint8_t aiio_ci13162_uart_get_voice_id(void);                   /* cached 1..4, 0 unknown; never sends */
  ```
- Produces (`aiio_ci13162_config.h`): `AIIO_CI13162_VOICE_TIMEOUT_MS 2000U`, `AIIO_CI13162_VOICE_DRAIN_MS 2000U`.

- [ ] **Step 1: Write the failing tests**

  Trong runner:
  - đổi mock `xTaskGetTickCount` (dòng 113) thành `static TickType_t s_mock_tick = 0;` trả về `s_mock_tick`;
  - `reset_test_state()` đặt:
    - `s_mock_tick = 0`, `s_aiio_ci13162_voice_id = 0U`, `s_aiio_ci13162_voice_set_sent = 0U`, `s_aiio_ci13162_voice_query_scheduled = 0U`, `s_aiio_ci13162_voice_query_attempts = 0U`, `aiio_ci13162_uart_set_voice_callbacks(NULL, NULL)`;
    - `s_aiio_ci13162_voice_version_valid = 0U`, `s_aiio_ci13162_version_query_scheduled = 0U`, `s_aiio_ci13162_version_query_paused = 0U`, để lần hỏi version không chèn `cmd:3` vào số TX. Các test cũ không gọi `aiio_ci13162_uart_poll()`, nên không bị ảnh hưởng;
  - thêm helper `static aiio_ci13162_parse_result_t parse(const char *line)` (gọi `aiio_ci13162_parse_line(line, &msg)`), `static void queue_and_process(uint8_t command, uint8_t value)` (một phần tử data), `static void queue_and_process_version(uint32_t version)`;
  - thêm 2 callback test ghi số lần gọi và giá trị cuối (`s_voice_set_cb_count/last`, `s_voice_get_cb_count/last`);
  - đổi dòng tổng kết thành `ALL 19`.

  ```c
  /* TEST 18: CMD_VOICE_SET / CMD_VOICE_GET (cmd 17/18) */
  reset_test_state();
  assert(aiio_ci13162_uart_send_voice_set(2U) == 0);
  assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":17,\"data\":[2]}\r\n") == 0);
  assert(aiio_ci13162_uart_send_voice_set(0U) == -1);
  assert(aiio_ci13162_uart_send_voice_set(5U) == -1);
  assert(aiio_ci13162_uart_query_voice() == 0);
  assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":18,\"data\":[]}\r\n") == 0);
  s_aiio_ci13162_ota_held = 1U;
  assert(aiio_ci13162_uart_send_voice_set(2U) == -1);
  assert(aiio_ci13162_uart_query_voice() == -1);
  s_aiio_ci13162_ota_held = 0U;
  /* parser */
  assert(parse("{\"cmd\":17,\"data\":[1]}") == AIIO_CI13162_PARSE_OK);
  assert(parse("{\"cmd\":17,\"data\":[0]}") == AIIO_CI13162_PARSE_OK);
  assert(parse("{\"cmd\":17,\"data\":[2]}") == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(parse("{\"cmd\":17,\"data\":[]}") == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(parse("{\"cmd\":17,\"data\":[1],\"cmd_id\":1,\"cwsl\":255}") == AIIO_CI13162_PARSE_INVALID_FORMAT);
  assert(parse("{\"cmd\":18,\"data\":[1]}") == AIIO_CI13162_PARSE_OK);
  assert(parse("{\"cmd\":18,\"data\":[4]}") == AIIO_CI13162_PARSE_OK);
  assert(parse("{\"cmd\":18,\"data\":[0]}") == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(parse("{\"cmd\":18,\"data\":[5]}") == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(parse("{\"cmd\":18,\"data\":[]}") == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(parse("{\"cmd\":18,\"data\":[1,2]}") == AIIO_CI13162_PARSE_INVALID_DATA);
  /* dispatch + cache */
  aiio_ci13162_uart_set_voice_callbacks(test_voice_set_cb, test_voice_get_cb);
  assert(aiio_ci13162_uart_get_voice_id() == 0U);
  assert(aiio_ci13162_uart_send_voice_set(3U) == 0);
  queue_and_process(AIIO_CI13162_CMD_VOICE_SET, 1U);  /* reply may come any time later, even after the app timed out */
  assert(s_voice_set_cb_count == 1 && s_voice_set_cb_last == 1U);
  assert(aiio_ci13162_uart_get_voice_id() == 3U);
  queue_and_process(AIIO_CI13162_CMD_VOICE_SET, 1U);  /* unsolicited: no value was sent */
  assert(s_voice_set_cb_count == 2 && aiio_ci13162_uart_get_voice_id() == 3U);
  assert(aiio_ci13162_uart_send_voice_set(2U) == 0);
  queue_and_process(AIIO_CI13162_CMD_VOICE_SET, 0U);
  assert(s_voice_set_cb_last == 0U && aiio_ci13162_uart_get_voice_id() == 3U);
  queue_and_process(AIIO_CI13162_CMD_VOICE_GET, 4U);
  assert(s_voice_get_cb_count == 1 && s_voice_get_cb_last == 4U);
  assert(aiio_ci13162_uart_get_voice_id() == 4U);
  ```

  - `queue_and_process` dựng `aiio_ci13162_message_t` (`.command`, `.data_count = 1U`, `.data[0]`), gọi `xQueueSend(NULL, &m, 0U)` rồi `aiio_ci13162_uart_process()`, như Test 17.

  ```c
  /* TEST 19: boot voice query after version */
  reset_test_state();
  queue_and_process_version(261102U);   /* .command = AIIO_CI13162_CMD_VERSION, .data_count = 1, .version = 261102 */
  int base = s_uart_tx_count;
  aiio_ci13162_uart_poll();
  assert(s_uart_tx_count == base + 1);
  assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":18,\"data\":[]}\r\n") == 0);
  s_mock_tick = 999;  aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 1);
  s_mock_tick = 1000; aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 2);
  s_mock_tick = 2000; aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 3);
  s_mock_tick = 3000; aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 3);   /* max 3 */
  queue_and_process(AIIO_CI13162_CMD_VOICE_GET, 2U);
  assert(aiio_ci13162_uart_get_voice_id() == 2U);

  /* stops once the cache is valid */
  reset_test_state();
  queue_and_process_version(261102U);
  aiio_ci13162_uart_poll();
  queue_and_process(AIIO_CI13162_CMD_VOICE_GET, 3U);
  base = s_uart_tx_count;
  s_mock_tick = 1000; aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base);

  /* paused (CWSL session) holds the query without spending attempts */
  reset_test_state();
  aiio_ci13162_uart_set_version_query_paused(1U);
  queue_and_process_version(261102U);
  base = s_uart_tx_count;
  aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base);
  aiio_ci13162_uart_set_version_query_paused(0U);
  aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 1);

  /* no version reply -> no voice query */
  reset_test_state();
  s_mock_tick = 5000; aiio_ci13162_uart_poll();
  assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":18,\"data\":[]}\r\n") != 0);
  ```

- [ ] **Step 2: Run test to verify it fails**

  Run: `RUNNERS="test_uart_protocol_runner" bash .../run-tests.sh`
  Expected: BUILD FAILED (`aiio_ci13162_uart_send_voice_set` undeclared).

- [ ] **Step 3: Implement trong driver**

  - **Enum:** `AIIO_CI13162_CMD_VOICE_SET = 17`, `AIIO_CI13162_CMD_VOICE_GET = 18`.
  - **Validate:**
    - 17: `data_count == 1U && data[0] <= 1U`;
    - 18: `data_count == 1U && 1U <= data[0] <= 4U`;
    - ngoài ra trả `INVALID_DATA`.
    - Luật cấm `cmd_id`/`cwsl` cho lệnh không phải lệnh thoại đã có sẵn trong `parse_line`.
  - **State tĩnh:**
    - `s_aiio_ci13162_voice_id`: 0 = chưa biết;
    - `s_aiio_ci13162_voice_set_sent`: giá trị cmd 17 vừa gửi thành công, 0 = không có;
    - `s_aiio_ci13162_voice_query_scheduled`, `s_aiio_ci13162_voice_query_attempts`, `s_aiio_ci13162_voice_query_last_tick`;
    - 2 con trỏ callback.
    - Hằng trong `.c`, cạnh `AIIO_CI13162_VERSION_*`: `AIIO_CI13162_VOICE_QUERY_RETRY_MS 1000U`, `AIIO_CI13162_VOICE_QUERY_MAX_ATTEMPTS 3U`, `AIIO_CI13162_VOICE_ID_MAX 4U`.
  - **`send_voice_set` / `query_voice`:** theo mẫu `aiio_ci13162_uart_send_volume` / `aiio_ci13162_uart_query_volume`. `send_voice_set` gửi đủ byte thì gán `s_aiio_ci13162_voice_set_sent = voice_id`.
  - **Dispatch:**
    - cmd 17: nếu `data[0] == 1U && voice_set_sent != 0U` thì `voice_id = voice_set_sent`. Sau đó, với mọi frame 17, đặt `voice_set_sent = 0U` rồi gọi `set_cb(data[0])` nếu khác NULL.
    - cmd 18: `voice_id = data[0]`, rồi gọi `get_cb(data[0])`.
    - Nhánh version: nếu `voice_id == 0U` thì `voice_query_scheduled = 1U`, `voice_query_attempts = 0U`.
  - **`aiio_ci13162_process_voice_query()`** (static), gọi trong `aiio_ci13162_uart_poll()` ngay sau `aiio_ci13162_process_version_query()`:
    - bỏ qua khi `!scheduled`, `voice_id != 0U`, `attempts >= MAX`, `s_aiio_ci13162_ota_held`, hoặc `s_aiio_ci13162_version_query_paused`;
    - lần đầu gửi ngay;
    - các lần sau chỉ gửi khi `now - last_tick >= pdMS_TO_TICKS(RETRY_MS)`;
    - mỗi lần gửi: `last_tick = now`, `attempts++`, rồi `(void)aiio_ci13162_uart_query_voice()`.
  - **Init** (khối reset dòng 1851-1858): reset `voice_id`, `voice_set_sent`, `scheduled`, `attempts`, `last_tick` về 0. Không reset callback, giống volume.
  - **Header:**
    - Doxygen cho 4 hàm mới;
    - sửa note của `aiio_ci13162_uart_set_version_query_paused`: hàm này cũng dừng lần hỏi giọng lúc boot.

- [ ] **Step 4: Run test to verify it passes**

  Run: `RUNNERS="test_uart_protocol_runner" bash .../run-tests.sh`
  Expected: `exit=0`, `ALL 19 PROTOCOL & HARDWARE TESTS PASSED`.

- [ ] **Step 5: Không commit.** Chạy đủ 4 runner baseline, kỳ vọng không đổi.

---

### Task 2: Module `app_voice` (MQTT 230/231) và chặn chéo với âm lượng

**Files:**
- Create: `smarthome_devices/components/application/app_voice.h`
- Create: `smarthome_devices/components/application/app_voice.c`
- Modify: `smarthome_devices/components/application/app_volume.c`:
  - `#include "app_voice.h"`;
  - `app_volume_busy()` (dòng 166-172) thêm `|| app_voice_is_pending()`.
- Modify: `smarthome_devices/components/application/bouffalo.mk` (thêm `app_voice.c` vào `COMPONENT_SRCS`)
- Test: Create `smarthome_devices/test_app_voice_runner.c`

**Interfaces:**
- Consumes:
  - Task 1: `aiio_ci13162_uart_send_voice_set`, `aiio_ci13162_uart_query_voice`, `aiio_ci13162_uart_set_voice_callbacks`, `AIIO_CI13162_VOICE_TIMEOUT_MS`, `AIIO_CI13162_VOICE_DRAIN_MS`;
  - có sẵn: `app_cwsl_is_pending()`, `app_volume_is_pending()`, `aiio_ci13162_uart_ota_held()`, `m_ci13162_ota_is_busy()`, `m_ota_is_busy()`, `app_send_data()`, `gs_dev_type[device_type]`, `CJSON_KEY_USER_ID`, `CJSON_KEY_ACT_ID`.
- Produces (`app_voice.h`):
  ```c
  void app_voice_init(void);              /* registers the Task 1 callbacks, clears state */
  void app_voice_set(cJSON *js_root);     /* MQTT 230 */
  void app_voice_get(cJSON *js_root);     /* MQTT 231 */
  void app_voice_poll(void);              /* timeout and drain; call from the app task loop */
  bool app_voice_is_pending(void);        /* true while a request is pending or draining */
  ```

- [ ] **Step 1: Write the failing tests** trong `test_app_voice_runner.c`

  **Cách dựng runner:**
  - Theo mẫu `test_app_cwsl_runner.c`: guard header SDK, include `cJSON.c`, rồi include trực tiếp `components/application/app_voice.c` và `components/application/app_volume.c`.
  - Đặt `device_type` vào chỉ số `"voiceboxv1"` của `gs_dev_type`.
  - Mock, mỗi cái có biến điều khiển hoặc ghi lại:
    - `app_send_data` (lưu `s_last_published`, `s_publish_count`);
    - `aiio_ci13162_uart_send_voice_set` (đếm, lưu tham số, giá trị trả về chỉnh được);
    - `aiio_ci13162_uart_query_voice` (đếm, trả về chỉnh được);
    - `aiio_ci13162_uart_set_voice_callbacks` (lưu 2 con trỏ để test gọi thay chip);
    - `aiio_ci13162_uart_send_volume` (đếm), `aiio_ci13162_uart_query_volume`, `aiio_ci13162_uart_set_volume_callback`;
    - `aiio_ci13162_uart_get_volume` (trả -1);
    - `aiio_ci13162_uart_ota_held`, `m_ci13162_ota_is_busy`, `m_ota_is_busy`, `app_cwsl_is_pending` (bool chỉnh được);
    - `xTaskGetTickCount` (`s_mock_tick`).
  - Helper `call(app_voice_set, "<json>")`: parse, gọi, delete.
  - Mỗi test gọi `app_voice_init()` và `app_volume_init()` trước, reset mock và đặt tick = 0.
  - Dòng cuối: `ALL 13 VOICE TESTS PASSED`.

  | Test | Thao tác | Kỳ vọng |
  |---|---|---|
  | `set_ok` | `call(set, {"voiceboxv1":230,"u":55519,"voice_id":2,"act_id":0})`; `set_cb(1)` | Gửi 1 lần với 2; trước cb `publish_count==0`, `app_voice_is_pending()`; sau cb `s_last_published == {"voiceboxv1":230,"u":55519,"voice_id":2,"act_id":0,"result":1}`, `!app_voice_is_pending()` |
  | `set_chip_rejects` | như trên, `set_cb(0)` | `{"voiceboxv1":230,"u":55519,"voice_id":2,"act_id":0,"result":0,"code":64}` |
  | `set_rejects_invalid_requests` | `voice_id` lần lượt `0`, `5`, `"2"`, `2.5`, `300`, `true`, thiếu; thiếu `act_id`; thiếu `u` | Mỗi lần trả code 64, không gửi UART. `voice_id` in ra: 0→0, 5→5, các trường hợp còn lại→0. Thiếu `act_id` (voice_id 2) → `"voice_id":2,"act_id":0`. Thiếu `u` → `"u":0` |
  | `get_ok` | `call(get, {"voiceboxv1":231,"u":55519,"act_id":0})`; `get_cb(3)` | Gọi `query_voice` 1 lần; `{"voiceboxv1":231,"u":55519,"voice_id":3,"act_id":0,"result":1}` |
  | `get_rejects_missing_act_id` | `{"voiceboxv1":231,"u":55519}` | `{"voiceboxv1":231,"u":55519,"act_id":0,"result":0,"code":64}`, không gửi |
  | `timeout_then_drain_expires` | set voice 2 ở tick 0; poll ở 1999; poll ở 2000; 230 mới; poll ở 4000; 230 mới | 1999: chưa publish. 2000: `...,"voice_id":2,"act_id":0,"result":0,"code":52}`, vẫn `is_pending` (đang chờ frame muộn). 230 trong lúc chờ → code 67. 4000: `!is_pending`; 230 mới được gửi |
  | `late_set_reply_only_ends_drain` | sau timeout (tick 2000), `set_cb(1)` | Không publish thêm; `!is_pending` |
  | `get_frame_does_not_end_set_drain` | sau timeout của 230, `get_cb(3)` | Không publish; vẫn `is_pending` |
  | `get_pending_ignores_set_frame` | 231 đang chờ; `set_cb(1)`; rồi `get_cb(2)` | `set_cb` không publish, vẫn chờ; `get_cb` trả 231 OK voice_id 2 |
  | `second_set_while_pending_is_busy` | 230 voice 2 đang chờ; 230 voice 3 | Lần 2 → `...,"voice_id":3,...,"code":67}`; tổng số lần gửi cmd 17 = 1 |
  | `busy_with_cwsl_and_volume` | (a) `app_cwsl_is_pending=true` → 230, 231. (b) Gọi `app_volume_set` với `{"voiceboxv1":225,"vol":3,"u":1,"act_id":0}` (âm lượng đang chờ) → 230. (c) 230 đang chờ → `app_volume_set` cùng request 225 | (a)(b) code 67, không gửi. (c) `{"voiceboxv1":225,"vol":3,"u":1,"act_id":0,"result":0}`, `send_volume` không được gọi |
  | `ota_and_order` | `ota_held=true` → 230 voice 2, 231; `ota_held=true` + voice_id 5 | Code 69 cho cả hai; voice_id 5 → 64 (kiểm request trước) |
  | `send_fail_and_unsolicited` | `send_voice_set` trả -1 → 230; rồi không có gì đang chờ, gọi `set_cb(1)`, `get_cb(2)` | Code 70 và `!is_pending` (không vào khoảng chờ); frame tự phát không publish |

- [ ] **Step 2: Run test to verify it fails**

  Run: `RUNNERS="test_app_voice_runner" bash .../run-tests.sh`
  Expected: BUILD FAILED (`components/application/app_voice.c` not found).

- [ ] **Step 3: Implement `app_voice.c` / `app_voice.h`**

  - **Hằng:**
    - `APP_VOICE_CMD_SET 230U`, `APP_VOICE_CMD_GET 231U`, `APP_VOICE_ID_MIN 1U`, `APP_VOICE_ID_MAX 4U`, `APP_VOICE_KEY_VOICE_ID "voice_id"`, `APP_VOICE_RESPONSE_SIZE 128U`;
    - mã lỗi: `APP_VOICE_CODE_TIMEOUT 52U`, `_BAD_REQUEST 64U`, `_BUSY 67U`, `_OTA_HOLD 69U`, `_TX_FAIL 70U`.
  - **Đọc số:**
    - `static bool app_voice_read_number(cJSON *, const char *key, uint32_t maximum, uint32_t *value)`, cùng luật với `app_volume_read_number`;
    - đọc từng key riêng để reply in lại được key nào đọc được.
  - **State:**
    - một phiên `{active, operation SET|GET, target, user_id, act_id, start_tick}`;
    - khoảng chờ `{draining, drain_operation, drain_tick}`.
  - **Busy** = phiên đang chạy, hoặc đang trong khoảng chờ, hoặc `app_cwsl_is_pending()`, hoặc `app_volume_is_pending()`.
  - **OTA** = `aiio_ci13162_uart_ota_held() || m_ci13162_ota_is_busy() || m_ota_is_busy()`.
  - **`set_cb(applied)`:**
    - đang chờ frame muộn của SET → kết thúc khoảng chờ, return;
    - không có phiên SET → return;
    - còn lại: đóng phiên, publish OK (`applied == 1`) hoặc code 64. `voice_id` = target.
  - **`get_cb(v)`:** đối xứng với `set_cb`, cho GET. Publish OK với `v`.
  - **`app_voice_poll`:**
    - phiên hết `AIIO_CI13162_VOICE_TIMEOUT_MS` → publish 52 (SET in target, GET không có voice_id), rồi vào khoảng chờ với `operation` của phiên;
    - khoảng chờ hết `AIIO_CI13162_VOICE_DRAIN_MS` → kết thúc.
  - **Gửi lỗi:** đóng phiên, không vào khoảng chờ, publish 70.
  - **`app_volume.c`:** sửa `app_volume_busy()` như ở mục Files.
  - **`bouffalo.mk`:** thêm `app_voice.c \` sau `app_volume.c \`.

- [ ] **Step 4: Run test to verify it passes**

  Run: `RUNNERS="test_app_voice_runner" bash .../run-tests.sh`
  Expected: `exit=0`, `ALL 13 VOICE TESTS PASSED`.

- [ ] **Step 5: Không commit.** Chạy 4 runner baseline cùng runner mới, tất cả pass.

---

### Task 3: Nối vào CWSL, process_data, bản tin 100/105, tài liệu, build

**Files:**
- Modify: `smarthome_devices/components/application/app_cwsl.c`:
  - `#include "app_voice.h"`;
  - 6 chỗ `if (app_volume_is_pending())` (trong `app_cwsl_start_mutating`, `app_cwsl_start_scene_learn`, `app_cwsl_start_scene_delete`, `app_cwsl_start_scene_resp_set`, `app_cwsl_stop`, `app_cwsl_query`) đổi thành `if (app_volume_is_pending() || app_voice_is_pending())`.
- Modify: `smarthome_devices/components/application/process_data.c`:
  - `#include "app_voice.h"`;
  - bảng dispatch thêm `{app_voice_set, 230}`, `{app_voice_get, 231}` sau `{app_scene_query, 229}`;
  - `app_voice_init()` sau `app_volume_init()`;
  - `app_voice_poll()` sau `app_volume_poll()`;
  - điều kiện bận của `app_request_ota` (129) và `app_request_ci13162_ota` (134) thêm `|| app_voice_is_pending()`, sửa trace thành `"OTA, CWSL, volume, or voice operation already in progress"`;
  - format 100 (khoảng dòng 186) và 105 (khoảng dòng 935): `"\"buzz\":%u,\"volume\":%u,\"voice_id\":%u,"`, đối số thêm `(unsigned int)aiio_ci13162_uart_get_voice_id()` ngay sau `vol_val`.
- Modify: `smarthome_devices/test_app_cwsl_runner.c`:
  - stub `bool app_voice_is_pending(void)` trả `s_mock_voice_pending`;
  - Test 10;
  - dòng tổng kết `ALL 10`.
- Modify: `smarthome_devices/test_voicebox_runner.c` Test 7: bản sao format 100 thêm `"voice_id":%u`, thêm `assert(cJSON_GetObjectItem(root, "voice_id") != NULL)`.
- Modify: `docs/voice/uart_wb2_ci13162.md`:
  - mục 3: thêm hàng 17, 18 (hàm gửi, ai gọi: MQTT 230/231 và lần hỏi lúc boot, chờ 2 s);
  - mục 4: thêm hàng 17 `[0]/[1]` → `app_voice`, cache khi `[1]`; hàng 18 `[1..4]` → cache và `app_voice`;
  - mục 7: tách hàng `CMD_RESP_GET (16)` còn "Chưa cài", và hàng `CMD_VOICE_SET/GET (17/18)` "Đã cài".

**Interfaces:**
- Consumes: Task 1 `aiio_ci13162_uart_get_voice_id()`; Task 2 `app_voice_init/set/get/poll/is_pending`.

- [ ] **Step 1: Write the failing test** (`test_app_cwsl_runner.c` Test 10, sau `reset_cwsl_mocks(); reset_cwsl_sessions(); s_mock_voice_pending = true;`)

  ```c
  int sent = s_uart_send_count;
  cJSON *req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":7,\"slot\":255,\"u\":1001,\"act_id\":1}");
  app_cwsl_learn(req); cJSON_Delete(req);
  assert(strstr(s_last_published, "\"code\":67") != NULL && s_uart_send_count == sent);
  assert(app_cwsl_start_scene_learn(56U, 1U, 0U, 47U) == 67U);
  assert(app_cwsl_start_scene_delete(56U, 1U, 0U) == 67U);
  assert(app_cwsl_start_scene_resp_set(56U, 47U) == 67U);
  req = cJSON_Parse("{\"fanoem\":222,\"u\":1001,\"act_id\":1}");
  app_cwsl_stop(req); cJSON_Delete(req);
  assert(strstr(s_last_published, "\"code\":67") != NULL);
  req = cJSON_Parse("{\"fanoem\":223,\"cmd_id\":255,\"u\":1001,\"act_id\":1}");
  app_cwsl_query(req); cJSON_Delete(req);
  assert(strstr(s_last_published, "\"code\":67") != NULL && s_uart_send_count == sent);
  s_mock_voice_pending = false;
  ```

- [ ] **Step 2: Run test to verify it fails**

  Run: `RUNNERS="test_app_cwsl_runner" bash .../run-tests.sh`
  Expected: FAIL. Assertion ở dòng `"code":67` đầu tiên (chưa kiểm `app_voice_is_pending`), hoặc link lỗi nếu stub được thêm sau.

- [ ] **Step 3: Implement** các sửa đổi trong `app_cwsl.c` và `process_data.c` như mục Files, sửa Test 7 của voicebox, cập nhật `uart_wb2_ci13162.md`.

- [ ] **Step 4: Verify**

  - Run: `RUNNERS="test_uart_protocol_runner test_app_cwsl_runner test_app_scene_runner test_voicebox_runner test_app_voice_runner" bash .../run-tests.sh`
    Expected: cả 5 `exit=0`; uart `ALL 19`, cwsl `ALL 10` và `failed=0`, scene 17, voicebox 9, voice `ALL 13`.
  - Run: lệnh build firmware ở Global Constraints.
    Expected:
    - exit 0;
    - `grep -n "warning" ` trên log build không có dòng nào ở `app_voice.c`, `app_volume.c`, `app_cwsl.c`, `process_data.c`, `aiio_ci13162_uart.c`;
    - `strings` của file bin có `voice_id`.

- [ ] **Step 5: Không commit.** Báo kết quả kèm số byte của bin.

---

## Kiểm trên board (sau khi chạy xong plan, người dùng làm)

1. Khởi động:
   - log có `[CI13162] version=...`, ngay sau đó là TX `{"cmd":18,"data":[]}` và RX `{"cmd":18,"data":[v]}`;
   - gửi 105, bản tin trả về có `"voice_id":v`.
2. `{"voiceboxv1":230,"u":55519,"voice_id":2,"act_id":0}`:
   - chip phát welcome giọng nữ Bắc;
   - App nhận `result:1`.
   - Ghi lại thời gian từ TX cmd 17 tới RX `[1]` để kiểm giả định trả lời trong 2 s.
3. `{"voiceboxv1":231,"u":55519,"act_id":0}` trả `"voice_id":2`.
4. `voice_id` 5: App nhận code 64, log không có TX cmd 17.
5. Trong lúc học lệnh (220), gửi 230: App nhận code 67.
