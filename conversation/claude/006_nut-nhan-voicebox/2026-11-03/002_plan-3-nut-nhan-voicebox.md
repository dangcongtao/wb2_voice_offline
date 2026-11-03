# 3 nút nhấn VOICEBOX Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** VOICEBOX có 3 nút. Nút 1 (IO21) tăng âm lượng, nút 3 (IO0) giảm âm lượng. Nút 2 (IO20) giữ thì vào cấu hình WiFi như nút cũ, nhấn nhanh thì đổi giọng 1→2→3→4→1. Mỗi lần nhấn có kết quả thì báo App bằng 225/230.

**Architecture:**
- **Driver UART:** cmd 9 bắt buộc đủ 3 phần tử. Driver nhớ thêm cờ `muted` và có hàm gửi `cmd 8 [8]` (tắt tiếng).
- **`app_volume` / `app_voice`:** thêm thao tác "từ nút nhấn" dùng chung phiên chờ chip, timeout và khoảng chờ frame muộn với 225/230. Nếu chưa biết trạng thái thì hỏi chip trước.
- **Module mới `app_button`:**
  - một máy trạng thái thuần, đổi sự kiện nút thành hành động;
  - một hàng đợi đưa hành động từ `led_button_task` sang task ứng dụng, vì trạng thái 225/230 và UART chỉ chạy trong task ứng dụng.
- **Phần còn lại:** `m_button.c` và `button.c` đọc 3 chân mới; `process_data.c` gọi init và poll.

**Tech Stack:** C (BL602 SDK, FreeRTOS, cJSON); runner build trên máy bằng gcc với `test_mocks/`.

**Spec:** `conversation/claude/006_nut-nhan-voicebox/2026-11-03/001_lam-ro-yeu-cau-3-nut-nhan.md` (giao thức phía chip: `docs/voice/commands.md` mục 2.2 và 3.2)

## Global Constraints

- **Không commit**; người dùng tự commit. Không sửa SDK `~/wb2/Ai-Thinker-WB2`, `docs/voice/hnn_voiceboxv1.xlsx`, hay thư viện `components/third_party/huytv_button/`. Không đổi hành vi nút của các thiết bị không phải VOICEBOX.
- **Chân nút** (chỉ số trong thư viện nút = chỉ số mảng):
  - nút 1 = chỉ số 0 = IO21 (tăng âm lượng);
  - nút 2 = chỉ số 1 = IO20 (cấu hình / đổi giọng);
  - nút 3 = chỉ số 2 = IO0 (giảm âm lượng);
  - nhấn = mức 0; mức nghỉ = 1; bật pull-up bằng `gpio_set_input(pin)`;
  - nút GPIO1 cũ bỏ hẳn.
- **Nút 2:**
  - giữ đủ 5 s (sự kiện `APP_BTN_EVT_HOLD`): bật đèn smart config. Thả tay: gửi bản tin offline, chờ 100 ms, xóa db, chạy smart config. Đây đúng là chuỗi lệnh VOICEBOX hiện có;
  - giữ đủ 10 s (`APP_BTN_EVT_HOLD_SO_LONG`): bật đèn AP config. Thả tay: chạy chuỗi AP config hiện có;
  - `APP_BTN_EVT_HOLD_HONG_PHIM` (25 s): bỏ qua;
  - thả trước khi có HOLD: đổi giọng.
- **Nút 1/3:**
  - chạy ngay ở `APP_BTN_EVT_PRESSED`, ±1 mức trong 1..7;
  - HOLD, HOLD_SO_LONG, HONG_PHIM và RELEASED không làm gì: không đèn, không cấu hình.
- **Không beep khi nhấn.**
- **Tính mức đích của âm lượng** (từ `level`/`muted` WB2 đã biết, hoặc từ cmd 9 vừa hỏi):

  | Trạng thái | Nút tăng | Nút giảm |
  |---|---|---|
  | không tắt tiếng | `cmd 8 [min(level+1,7)]` | `cmd 8 [max(level-1,1)]` |
  | đang tắt tiếng | `cmd 8 [1]` | `cmd 8 [8]` (giữ tắt tiếng) |

  Ở biên vẫn gửi lại và vẫn báo App.
- **Giọng đích:** `v % 4 + 1`, với `v` là giọng hiện tại 1..4.
- **Chưa biết trạng thái thì hỏi chip trước:**
  - âm lượng: `cmd 11`, có trả lời rồi mới gửi `cmd 8`;
  - giọng: `cmd 18`, có trả lời rồi mới gửi `cmd 17`.
- **Mỗi bước UART** dùng timeout `AIIO_CI13162_VOLUME_TIMEOUT_MS` / `AIIO_CI13162_VOICE_TIMEOUT_MS` (2000) và khoảng chờ frame muộn `..._DRAIN_MS` (2000) hiện có.
- **Báo App** (`u` luôn 0, `act_id` luôn 1 = `SRC_ACTION_DEVICE`; dùng đúng hàm publish hiện có của từng module):
  - 230 OK: `{"voiceboxv1":230,"u":0,"voice_id":%u,"act_id":1,"result":1}`
  - 230 lỗi: `{"voiceboxv1":230,"u":0,"voice_id":%u,"act_id":1,"result":0,"code":%u}`. Mã: 64 chip trả `[0]`, 52 hết 2 s, 70 gửi UART lỗi.
  - 225 OK: `{"voiceboxv1":225,"vol":%u,"u":0,"act_id":1,"result":1}`. `vol` = 0 nếu chip báo `muted`=1, ngược lại là `level` chip báo.
  - 225 lỗi: `{"voiceboxv1":225,"vol":%u,"u":0,"act_id":1,"result":0}`.
  - Trong bản tin lỗi, `voice_id`/`vol` là giá trị định đặt (tắt tiếng = 0). Lỗi ngay ở bước hỏi thì là 0.
- **Thành công:**
  - giọng: chip trả `cmd 17 [1]`;
  - âm lượng: `cmd 9` có `result`=1 và đúng đích. Đích tắt tiếng thì cần `muted`=1; đích mức `n` thì cần `level`=n và `muted`=0.
  - Khác các trường hợp trên là lỗi.
- **Bận:** đang có 220–231 hoặc lần nhấn trước đang chờ chip hoặc trong khoảng chờ frame muộn, hoặc đang OTA.
  - Lúc đó bỏ qua lần nhấn, gửi `cmd 200 [1]` (beep dài) qua `aiio_ci13162_uart_send_buzz_type(1U)`.
  - Riêng khi `app_cwsl_is_learning()` (đang chạy phiên 220 hoặc 227 thêm mới, mọi giai đoạn) thì bỏ qua và im lặng.
  - Lệnh App đến trong lúc lần nhấn đang chờ nhận lỗi bận như hiện nay: 67 cho 220–223/227/228/230/231, `result:0` cho 225/226.
- **cmd 9:**
  - chỉ nhận đúng 3 phần tử `[level 0..7, result 0..1, muted 0..1]`; khác đi là `INVALID_DATA` (MQTT 299);
  - bản tin 105, 100 và các reply 225/226 của App giữ nguyên như hiện tại.
- **Lệnh chạy runner:** `RUNNERS="<tên runner>" bash /home/taodc/wb2/ai-wb2-baremetal/.superpowers/sdd/<workspace của plan này>/run-tests.sh`.
  - Đây là bản sao của `.superpowers/sdd/002_plan-them-230-231-voice-id/run-tests.sh`, không truyền `RUNNERS` thì chạy cả 5 runner.
  - Baseline: uart ALL 19, cwsl ALL 10 + `validation total=52 failed=0`, scene ALL 17, voicebox ALL 9, voice ALL 14.
- **Build firmware:** `cd smarthome_devices && BL60X_SDK_PATH=$HOME/wb2/Ai-Thinker-WB2/ CONFIG_CHIP_NAME=BL602 make -j16`. Kỳ vọng exit 0 và không có warning mới ở file đã sửa. Warning `aiio_ci13162_print_hex` không dùng là warning cũ.
- **Quy ước code:** comment tiếng Anh, header Doxygen theo file bên cạnh, hằng có hậu tố `U`.

## Review Focus

1. **Nhấn lần 2 khi lần 1 còn đang chờ chip** (bấm nhanh, hoặc nhấn 2 nút cùng lúc): chip chỉ nhận 1 lệnh, lần 2 beep dài.
   - Test: Task 3 `button_second_press_while_pending_beeps`.
2. **Giữ nút 1 hoặc 3 quá 5 s / 10 s:** không bật đèn cấu hình, không vào cấu hình, không đổi gì thêm.
   - Test: Task 3 `button_classify_volume_hold_no_config`.
3. **Giữ nút 2 quá 5 s rồi thả:** chỉ chạy cấu hình, không đổi giọng. Lần nhấn ngắn ngay sau đó lại đổi giọng.
   - Test: Task 3 `button_classify_function_hold`.
4. **Lần nhấn đầu sau khi khởi động, WB2 chưa biết mức/giọng:** hỏi chip rồi mới đặt; App chỉ nhận một bản tin.
   - Test: Task 2 `vol_unknown_queries_first`, `voice_next_unknown_queries_first`.
5. **Nhấn trong lúc đang học lệnh:** không gửi gì xuống chip, kể cả beep.
   - Test: Task 3 `button_poll_busy_learning_silent`.

---

### Task 1: Driver UART — cmd 9 đủ 3 phần tử, cờ muted, gửi tắt tiếng

**Files:**
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.h`
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.c`:
  - case `AIIO_CI13162_CMD_VOLUME_INFO` trong `aiio_ci13162_validate_message`;
  - bảng test nhúng cmd 9 (khoảng dòng 1293–1309);
  - dispatch cmd 9/13 trong `aiio_ci13162_uart_process`;
  - reset ở init (cạnh `s_aiio_ci13162_volume_valid = 0U`);
  - thêm hàm gửi cạnh `aiio_ci13162_uart_send_volume`.
- Modify: `smarthome_devices/components/application/app_volume.c`: `app_volume_on_info` nhận thêm `muted`, chưa dùng tới (`(void)muted;`).
- Modify: `smarthome_devices/test_app_voice_runner.c`: chỗ nào gọi callback âm lượng thì truyền thêm `muted` 0.
- Test: `smarthome_devices/test_uart_protocol_runner.c`

**Interfaces:**
- Produces:
  ```c
  typedef void (*aiio_ci13162_volume_info_cb_t)(uint8_t level, uint8_t result, uint8_t muted); /* was (level, result) */
  int32_t aiio_ci13162_uart_send_mute(void);   /* {"cmd":8,"data":[8]}\r\n; 0 sent; -1 queue NULL, OTA held, short TX */
  int32_t aiio_ci13162_uart_get_volume_state(uint8_t *level, uint8_t *muted); /* 0 valid; -1 NULL arg or never reported */
  ```
- Cache `muted`:
  - cmd 9 đặt `muted = data[2]`;
  - cmd 13 đặt `muted = (data[0] == 0U) ? 1U : 0U`;
  - init đặt về 0;
  - hợp lệ cùng lúc với cache `level` (`s_aiio_ci13162_volume_valid`).
- Cache `level` và `aiio_ci13162_uart_get_volume()` giữ nguyên hành vi.

- [ ] **Step 1: Write the failing tests**

  Bảng test nhúng cmd 9:
  - `[3,1]`, `[7,0]`, `[0,1]` đổi kỳ vọng thành `AIIO_CI13162_PARSE_INVALID_DATA`;
  - thêm `[3,1,0]` OK (`data_count` 3, data `{3,1,0}`), `[0,1,1]` OK, `[7,0,1]` OK;
  - thêm `[3,1,2]`, `[8,1,0]`, `[3,2,0]` là `INVALID_DATA`;
  - giữ `[3]` và `[3,1,0,0]`.

  Thêm Test 20 vào runner, dòng tổng kết đổi thành `ALL 20`:
  ```c
  /* TEST 20: volume muted state + mute send */
  reset_test_state();
  assert(aiio_ci13162_uart_send_mute() == 0);
  assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":8,\"data\":[8]}\r\n") == 0);
  s_aiio_ci13162_ota_held = 1U; assert(aiio_ci13162_uart_send_mute() == -1); s_aiio_ci13162_ota_held = 0U;
  uint8_t lv = 9U, mu = 9U;
  s_aiio_ci13162_volume_valid = 0U;
  assert(aiio_ci13162_uart_get_volume_state(&lv, &mu) == -1);
  assert(aiio_ci13162_uart_get_volume_state(NULL, &mu) == -1);
  aiio_ci13162_uart_set_volume_callback(test_volume_cb);          /* records count, level, result, muted */
  queue_and_process_volume_info(3U, 1U, 1U);                      /* cmd 9 [3,1,1] */
  assert(s_volume_cb_count == 1 && s_volume_cb_level == 3U && s_volume_cb_result == 1U && s_volume_cb_muted == 1U);
  assert(aiio_ci13162_uart_get_volume_state(&lv, &mu) == 0 && lv == 3U && mu == 1U);
  queue_and_process_volume_change(4U, 49U, 255U);                 /* cmd 13 [4], cmd_id 49 cwsl 255 */
  assert(aiio_ci13162_uart_get_volume_state(&lv, &mu) == 0 && lv == 4U && mu == 0U);
  queue_and_process_volume_change(0U, 54U, 255U);                 /* cmd 13 [0] (mute) */
  assert(aiio_ci13162_uart_get_volume_state(&lv, &mu) == 0 && lv == 0U && mu == 1U);
  aiio_ci13162_uart_set_volume_callback(NULL);
  ```
  - Helper dựng `aiio_ci13162_message_t` theo cách các test 7/11 đang làm.
  - `reset_test_state()` reset thêm `s_aiio_ci13162_volume_muted = 0U`, `s_aiio_ci13162_volume_valid = 0U`.
  - cmd 13 kèm `app_volume_report_voice`/`app_cwsl_notify_voice_cmd`, đã có stub trong runner.

- [ ] **Step 2: Chạy để thấy fail**
  - Run: `RUNNERS="test_uart_protocol_runner" bash …/run-tests.sh`
  - Expected: BUILD FAILED (`aiio_ci13162_uart_send_mute` undeclared).

- [ ] **Step 3: Implement**
  - Thêm biến tĩnh `s_aiio_ci13162_volume_muted`.
  - Validate cmd 9: `data_count == 3U && data[0] <= 7U && data[1] <= 1U && data[2] <= 1U`.
  - Dispatch cmd 9 cập nhật cache (level, muted, valid) **trước** khi gọi callback `(data[0], data[1], data[2])`, để callback đọc `get_volume_state` thấy giá trị mới.
  - Dispatch cmd 13 cập nhật `muted` như mục Interfaces.
  - `send_mute` theo mẫu `aiio_ci13162_uart_query_volume`, dùng chuỗi tĩnh.
  - Doxygen cho 2 hàm mới; cập nhật mô tả typedef callback.

- [ ] **Step 4: Chạy để thấy pass**
  - Run: `bash …/run-tests.sh` (cả 5 runner).
  - Expected: uart `ALL 20`; cwsl, scene, voicebox, voice như baseline.

- [ ] **Step 5: Không commit.**

---

### Task 2: Thao tác "từ nút nhấn" trong `app_volume` và `app_voice`

**Files:**
- Modify: `smarthome_devices/components/application/app_volume.h`, `app_volume.c`
- Modify: `smarthome_devices/components/application/app_voice.h`, `app_voice.c`
- Test: `smarthome_devices/test_app_voice_runner.c`

**Interfaces:**
- Consumes (Task 1): `aiio_ci13162_uart_get_volume_state`, `aiio_ci13162_uart_send_mute`, callback âm lượng `(level, result, muted)`. Có sẵn: `aiio_ci13162_uart_get_voice_id()` (0 = chưa biết), `aiio_ci13162_uart_send_volume`, `aiio_ci13162_uart_query_volume`, `aiio_ci13162_uart_send_voice_set`, `aiio_ci13162_uart_query_voice`.
- Produces:
  ```c
  /* app_volume.h */
  bool app_volume_button_step(bool up);  /* false = busy or OTA (nothing sent); true = handled (sent, or TX failure already reported) */
  /* app_voice.h */
  bool app_voice_button_next(void);      /* same contract */
  ```
- **Hành vi:**
  - **Bận và OTA:** dùng lại `app_volume_busy()` / `app_voice_busy()` và hàm OTA của từng module.
  - **Thao tác mới của âm lượng:**
    - `APP_VOLUME_OPERATION_BUTTON_QUERY`: chờ cmd 9 của `cmd 11`;
    - `APP_VOLUME_OPERATION_BUTTON_SET`: chờ cmd 9 của `cmd 8`; lưu đích, kèm cờ "đích là tắt tiếng".
  - **Thao tác mới của giọng:**
    - `APP_VOICE_OPERATION_BUTTON_QUERY`: chờ cmd 18;
    - phần đặt giọng dùng lại `APP_VOICE_OPERATION_SET` với `user_id` 0, `act_id` 1.
  - **Khoảng chờ frame muộn của giọng:** `BUTTON_QUERY` nuốt frame cmd 18 như GET; SET nuốt cmd 17.
  - **Bước hỏi có trả lời:**
    - tính đích theo Global Constraints, gửi lệnh đặt, chuyển phiên sang thao tác đặt và đặt lại `start_tick`;
    - gửi lỗi: đóng phiên, không vào khoảng chờ, báo App lỗi với giá trị đích (230 kèm code 70).
  - **Timeout:**
    - báo App lỗi (225 `result:0`; 230 code 52), giá trị = đích, ở bước hỏi thì 0;
    - sau đó vào khoảng chờ frame muộn, như hiện tại.
  - **cmd 9 `result`=0 ở bước hỏi:** báo 225 `vol` 0 `result:0`.

- [ ] **Step 1: Write the failing tests**

  Thêm mock vào runner, mỗi mock có biến điều khiển:
  - `aiio_ci13162_uart_get_volume_state` (valid, level, muted);
  - `aiio_ci13162_uart_send_mute` (đếm, giá trị trả về);
  - `aiio_ci13162_uart_get_voice_id` (giá trị trả về);
  - `send_volume` ghi lại tham số cuối.

  Lưu callback âm lượng để gọi thay chip (`vol_cb(level, result, muted)`). Viết tắt: `B225(v,r)` = `{"voiceboxv1":225,"vol":v,"u":0,"act_id":1,"result":r}`; `B230ok(v)` / `B230err(v,c)` theo Global Constraints.

  | Test | Thao tác | Kỳ vọng |
  |---|---|---|
  | `vol_up_known` | state (4,0); `step(true)`; `vol_cb(5,1,0)` | trả true; gửi `send_volume(5)`; publish `B225(5,1)`; `!app_volume_is_pending()` |
  | `vol_down_known` | (4,0); `step(false)`; `vol_cb(3,1,0)` | `send_volume(3)`; `B225(3,1)` |
  | `vol_up_at_max` / `vol_down_at_min` | (7,0) up / (1,0) down; cb trả cùng mức | `send_volume(7)` / `(1)`; `B225(7,1)` / `B225(1,1)` |
  | `vol_up_muted` | (4,1) up; `vol_cb(1,1,0)` | `send_volume(1)`; `B225(1,1)` |
  | `vol_down_muted` | (4,1) down; `vol_cb(4,1,1)` | `send_mute` 1 lần, `send_volume` 0 lần; `B225(0,1)` |
  | `vol_unknown_queries_first` | state invalid; up; `vol_cb(3,1,0)`; `vol_cb(4,1,0)` | `query_volume` 1 lần; sau cb đầu: `send_volume(4)`, chưa publish, vẫn pending; sau cb 2: `B225(4,1)`, tổng publish = 1 |
  | `vol_query_timeout` | invalid; up; poll ở tick 2000 | `B225(0,0)`; vẫn `is_pending` (khoảng chờ frame muộn) |
  | `vol_set_rejected` | (4,0) up; `vol_cb(4,0,0)` | `B225(5,0)` |
  | `vol_set_timeout` | (4,0) up; poll ở tick 2000 | `B225(5,0)` |
  | `vol_busy` | lần lượt: cwsl pending; voice pending (230 App đang chờ); 225 App đang chờ; `ota_held` | mỗi lần `step` trả false, không gửi gì, không publish |
  | `vol_tx_fail` | (4,0); `send_volume` trả -1; up | trả true; `B225(5,0)`; `!is_pending` |
  | `voice_next_known` | id 2; `next()`; `set_cb(1)` | trả true; `send_voice_set(3)`; `B230ok(3)` |
  | `voice_next_wraps` | id 4; `set_cb(1)` | `send_voice_set(1)`; `B230ok(1)` |
  | `voice_next_unknown_queries_first` | id 0; `get_cb(2)`; `set_cb(1)` | `query_voice` 1 lần; sau `get_cb`: `send_voice_set(3)`, chưa publish; sau `set_cb`: `B230ok(3)`, tổng publish = 1 |
  | `voice_next_rejected` | id 1; `set_cb(0)` | `B230err(2,64)` |
  | `voice_query_timeout` | id 0; tick 2000 | `B230err(0,52)`; trong khoảng chờ, `get_cb(3)` không publish và kết thúc khoảng chờ |
  | `voice_set_timeout` | id 1; tick 2000 | `B230err(2,52)` |
  | `voice_busy` | cwsl pending; 225 App đang chờ; `ota_held` | `next()` trả false, không gửi, không publish |
  | `voice_tx_fail` | id 1, `send_voice_set` trả -1; rồi id 0, `query_voice` trả -1 | `B230err(2,70)`; rồi `B230err(0,70)`; cả hai `!is_pending` |
  | `app_busy_during_button` | id 1 `next()` đang chờ → App 230 `{"voiceboxv1":230,"u":5,"voice_id":2,"act_id":0}`; (4,0) `step(true)` đang chờ → App 225 `{"voiceboxv1":225,"vol":3,"u":5,"act_id":0}` | 230 App nhận `...,"code":67}`; 225 App nhận `{"voiceboxv1":225,"vol":3,"u":5,"act_id":0,"result":0}` |

  Dòng tổng kết: `ALL 35 VOICE TESTS PASSED` (14 cũ + 21 mới).

- [ ] **Step 2: Chạy để thấy fail**
  - Run: `RUNNERS="test_app_voice_runner" bash …/run-tests.sh`
  - Expected: BUILD FAILED (`app_volume_button_step` undeclared).

- [ ] **Step 3: Implement** theo mục Interfaces.
  - Trong `app_volume_on_info`:
    - giữ nguyên nhánh `MQTT_SET`/`MQTT_GET`;
    - thêm nhánh `BUTTON_QUERY` và `BUTTON_SET`.
  - Báo App:
    - âm lượng dùng `app_volume_publish(APP_VOLUME_CMD_SET, vol, 0U, 1U, result)`;
    - giọng dùng `app_voice_publish(APP_VOICE_CMD_SET, v, 0U, 1U, result, code)`.
  - `act_id` 1 là `SRC_ACTION_DEVICE` trong `system_typedef.h`; dùng enum đó, không viết số trần.

- [ ] **Step 4: Chạy để thấy pass**
  - Run: `bash …/run-tests.sh`
  - Expected: voice `ALL 35`; uart 20; còn lại như baseline.

- [ ] **Step 5: Không commit.**

---

### Task 3: Module `app_button` và `app_cwsl_is_learning`

**Files:**
- Create: `smarthome_devices/components/application/app_button.h`, `app_button.c`
- Modify: `smarthome_devices/components/application/app_cwsl.h`, `app_cwsl.c`: thêm `app_cwsl_is_learning`.
- Modify: `smarthome_devices/components/application/bouffalo.mk`: thêm `app_button.c`.
- Test:
  - `smarthome_devices/test_app_voice_runner.c`: include thêm `app_button.c` và `components/third_party/huytv_button/app_btn.h`; thêm mock queue, `app_cwsl_is_learning`, `aiio_ci13162_uart_send_buzz_type`;
  - `smarthome_devices/test_app_cwsl_runner.c`: Test 11.

**Interfaces:**
- Consumes (Task 2): `app_volume_button_step(bool)`, `app_voice_button_next(void)`.
- Produces:
  ```c
  /* app_cwsl.h */
  bool app_cwsl_is_learning(void);  /* true while a 220 learn or a 227 add session is active, any phase */

  /* app_button.h */
  typedef enum {
      APP_BUTTON_ACTION_NONE = 0,
      APP_BUTTON_ACTION_VOLUME_UP,
      APP_BUTTON_ACTION_VOLUME_DOWN,
      APP_BUTTON_ACTION_VOICE_NEXT,
      APP_BUTTON_ACTION_LED_SMART_CONFIG,
      APP_BUTTON_ACTION_LED_AP_CONFIG,
      APP_BUTTON_ACTION_SMART_CONFIG,
      APP_BUTTON_ACTION_AP_CONFIG,
  } app_button_action_t;
  #define APP_BUTTON_INDEX_VOLUME_UP   0U  /* IO21 */
  #define APP_BUTTON_INDEX_FUNCTION    1U  /* IO20 */
  #define APP_BUTTON_INDEX_VOLUME_DOWN 2U  /* IO0  */
  app_button_action_t app_button_voicebox_event(uint32_t index, int event); /* button task; pure state machine over APP_BTN_EVT_* */
  void app_button_init(void);                       /* creates a static queue of 4 actions */
  bool app_button_post(app_button_action_t action); /* button task; accepts only VOLUME_UP/DOWN/VOICE_NEXT; false if not init, full, or other action */
  void app_button_poll(void);                       /* app task; runs queued actions */
  ```
- **`app_button_voicebox_event`:**
  - Mỗi chỉ số có một trạng thái giữ 0 / HOLD / SO_LONG.
  - Nút chỉ số 0 / 2:
    - `PRESSED` → UP / DOWN;
    - mọi sự kiện khác → NONE.
  - Nút chỉ số 1:
    - `HOLD` → đặt trạng thái HOLD, trả `LED_SMART_CONFIG`;
    - `HOLD_SO_LONG` → đặt SO_LONG, trả `LED_AP_CONFIG`;
    - `RELEASED` → trạng thái HOLD trả `SMART_CONFIG`, SO_LONG trả `AP_CONFIG`, còn lại trả `VOICE_NEXT`; sau đó xóa trạng thái;
    - `PRESSED` / `HOLD_HONG_PHIM` / sự kiện khác → NONE.
  - Chỉ số từ 3 trở lên → NONE.
- **`app_button_poll`:**
  - Lấy hết hàng đợi; với mỗi hành động gọi hàm Task 2 tương ứng.
  - Hàm trả false thì: `app_cwsl_is_learning()` → im lặng; ngược lại → `aiio_ci13162_uart_send_buzz_type(1U)`.

- [ ] **Step 1: Write the failing tests**

  Test 11 trong cwsl runner (dòng tổng kết `ALL 11`):
  - `assert(!app_cwsl_is_learning())` khi rảnh;
  - sau `app_cwsl_learn({"fanoem":220,"cmd_id":7,"slot":255,"u":1001,"act_id":1})` → true;
  - reset; sau `app_cwsl_start_scene_learn(56U,1U,0U,47U)` → true;
  - reset; sau `app_cwsl_start_scene_delete(56U,1U,0U)` → false;
  - reset; sau `app_cwsl_query({"fanoem":223,"cmd_id":255,"u":1001,"act_id":1})` → false;
  - reset; sau `app_cwsl_start_scene_resp_set(56U,47U)` → false.

  Thêm test vào voice runner (dòng tổng kết `ALL 45 VOICE TESTS PASSED`):

  | Test | Kỳ vọng |
  |---|---|
  | `button_classify_volume_press` | `(0,PRESSED)`=UP; `(2,PRESSED)`=DOWN; `(1,PRESSED)`=NONE; `(0,RELEASED)`=NONE; `(3,PRESSED)`=NONE |
  | `button_classify_volume_hold_no_config` | lần lượt `(0,PRESSED)`, `(0,HOLD)`, `(0,HOLD_SO_LONG)`, `(0,HOLD_HONG_PHIM)`, `(0,RELEASED)` → UP, NONE, NONE, NONE, NONE; chỉ số 2 (IO0) cũng vậy với DOWN |
  | `button_classify_function_short` | `(1,PRESSED)`=NONE, `(1,RELEASED)`=VOICE_NEXT |
  | `button_classify_function_hold` | `(1,HOLD)`=LED_SMART_CONFIG, `(1,RELEASED)`=SMART_CONFIG; rồi `(1,PRESSED)`, `(1,RELEASED)` = NONE, VOICE_NEXT |
  | `button_classify_function_hold_long` | `(1,HOLD)`=LED_SMART, `(1,HOLD_SO_LONG)`=LED_AP, `(1,HOLD_HONG_PHIM)`=NONE, `(1,RELEASED)`=AP_CONFIG |
  | `button_post_rules` | trước `app_button_init()`: `post(UP)` = false; sau init: `post(SMART_CONFIG)` = false, `post(LED_AP_CONFIG)` = false, `post(UP)` = true |
  | `button_poll_runs_actions` | state âm lượng (4,0), id giọng 2. (1) post UP → poll → `send_volume(5)`; `vol_cb(5,1,0)`. (2) đổi mock state thành (5,0); post DOWN → poll → `send_volume(4)`; `vol_cb(4,1,0)`. (3) post VOICE_NEXT → poll → `send_voice_set(3)`; `set_cb(1)`. Cả quá trình `buzz` 0 lần |
  | `button_poll_busy_beeps` | cwsl pending, learning false; post UP; poll → `buzz_type` 1 lần với tham số 1; `send_volume` 0 lần |
  | `button_poll_busy_learning_silent` | cwsl pending, learning true; post DOWN; poll → `buzz` 0 lần, không gửi UART |
  | `button_second_press_while_pending_beeps` | (4,0); post UP 2 lần; poll → `send_volume` 1 lần, `buzz_type(1)` 1 lần |

  Mock queue trong runner: mảng vòng chứa `uint8_t`. `xQueueCreateStatic` trả handle khác NULL.

- [ ] **Step 2: Chạy để thấy fail**
  - Run: `RUNNERS="test_app_voice_runner test_app_cwsl_runner" bash …/run-tests.sh`
  - Expected: BUILD FAILED (`app_button.c` not found; `app_cwsl_is_learning` undeclared).

- [ ] **Step 3: Implement** theo mục Interfaces.
  - Queue dùng `xQueueCreateStatic` với bộ nhớ tĩnh, phần tử `uint8_t`.
  - `post` dùng `xQueueSend(q, &a, 0)`; poll dùng `xQueueReceive(q, &a, 0)` trong vòng lặp.

- [ ] **Step 4: Chạy để thấy pass**
  - Run: `bash …/run-tests.sh`
  - Expected: voice `ALL 45`, cwsl `ALL 11` và `failed=0`, uart 20, scene 17, voicebox 9.

- [ ] **Step 5: Không commit.**

---

### Task 4: Nối phần cứng — chân nút, `m_button.c`, `process_data.c`, tài liệu, build

**Files:**
- Modify: `smarthome_devices/components/hardware/hardware.h`:
  - bỏ `#define BTN_CFG_VOICEBOX 1`;
  - thêm `BTN_VOICEBOX_VOLUME_UP_PIN 21`, `BTN_VOICEBOX_FUNCTION_PIN 20`, `BTN_VOICEBOX_VOLUME_DOWN_PIN 0`, `BTN_VOICEBOX_IDLE_LEVEL 1`.
- Modify: `smarthome_devices/components/hardware/button.c`:
  - **VOICEBOX:**
    - `hw_button_config(i)` với `i` từ 0 đến 2 gọi `gpio_set_input(<chân của i>)`;
    - `hw_btn_read(i)` trả `gpio_get_level(<chân của i>)`; `i` từ 3 trở lên trả `BTN_VOICEBOX_IDLE_LEVEL`;
    - bỏ phần đọc GPIO1 và phần OR với giá trị touch.
  - Nhánh của thiết bị khác giữ nguyên.
  - Bỏ `#include "hosal_adc.h"` nếu không còn dùng; kiểm bằng grep `hosal_adc_` trong file.
- Modify: `smarthome_devices/components/middle/gpio/m_button.c`:
  - **`m_button_config`:** VOICEBOX dùng `BTN_VOICEBOX_IDLE_LEVEL` cho `idle_level` và `last_state`; thiết bị khác giữ `BTN_IDLE_LEVEL`.
  - **`on_button_event_cb`:** ngay đầu hàm, nếu `device_type == VOICEBOX` thì gọi hàm tĩnh mới `m_button_voicebox_event(button_pin, event)` rồi return. Hàm này:
    - lấy `app_button_voicebox_event()`;
    - UP / DOWN / VOICE_NEXT → `app_button_post()`;
    - `LED_SMART_CONFIG` / `LED_AP_CONFIG` → 2 lệnh đèn hiện có;
    - `SMART_CONFIG` / `AP_CONFIG` → đúng 2 chuỗi lệnh VOICEBOX hiện có ở nhánh RELEASED (offline message, `vTaskDelay(100)`, reset db, start smart/AP config).
  - Xóa các nhánh `device_type == VOICEBOX` cũ trong PRESSED (beep), RELEASED và HOLD_HONG_PHIM, vì đã bị thay. Nhánh của thiết bị khác giữ nguyên từng dòng.
- Modify: `smarthome_devices/components/application/process_data.c`:
  - `#include "app_button.h"`;
  - `app_button_init()` sau `app_voice_init()`;
  - `app_button_poll()` sau `app_voice_poll()`.
- Modify: `smarthome_devices/test_voicebox_runner.c`:
  - Test 2: thay `assert(BTN_CFG_VOICEBOX == 1)` bằng assert 3 chân 21/20/0 và mức nghỉ 1;
  - Test 10 mới: dùng `button.c` thật;
  - dòng tổng kết `ALL 10`.
- Modify: `docs/voice/uart_wb2_ci13162.md` (tiếng Việt, theo kiểu bảng của file, sửa đè lên nội dung hiện tại):
  - hàng cmd 9: đúng 3 phần tử, `muted` được nhớ;
  - hàng cmd 8: thêm `[8]` do nút giảm gửi khi đang tắt tiếng;
  - hàng cmd 11/17/18: thêm "nút nhấn" vào cột người gọi;
  - thêm một đoạn ngắn "Nút nhấn VOICEBOX": bảng 3 nút, hành động, báo App.

**Interfaces:**
- Consumes (Task 3): `app_button_voicebox_event`, `app_button_post`, `app_button_init`, `app_button_poll`, các giá trị `APP_BUTTON_ACTION_*`.

- [ ] **Step 1: Write the failing test** — Test 10 trong voicebox runner.
  - Đổi mock `bl_gpio_enable_input` để ghi lại chân và pull-up: `s_mock_gpio_input_pins[pin]=1`, `s_mock_gpio_input_pullup[pin]=pullup`.
  - Đổi mock `bl_gpio_input_get_value` để trả `s_mock_gpio_input_levels[pin]`, mặc định 1.
  - Thêm stub `void gpio_set_input(int pin) { bl_gpio_enable_input(pin, 1, 0); }`.
  - Include `components/hardware/button.c`.

  ```c
  device_type = VOICEBOX;
  hw_button_config(0U); hw_button_config(1U); hw_button_config(2U);
  assert(s_mock_gpio_input_pins[21] && s_mock_gpio_input_pullup[21] == 1);
  assert(s_mock_gpio_input_pins[20] && s_mock_gpio_input_pullup[20] == 1);
  assert(s_mock_gpio_input_pins[0]  && s_mock_gpio_input_pullup[0]  == 1);
  assert(s_mock_gpio_input_pins[1] == 0);                      /* old GPIO1 no longer configured */
  assert(hw_btn_read(0U) == 1U && hw_btn_read(1U) == 1U && hw_btn_read(2U) == 1U);   /* idle */
  s_mock_gpio_input_levels[21] = 0; assert(hw_btn_read(0U) == 0U);
  s_mock_gpio_input_levels[20] = 0; assert(hw_btn_read(1U) == 0U);
  s_mock_gpio_input_levels[0]  = 0; assert(hw_btn_read(2U) == 0U);
  hw_btn_set_val_button(0xFF); s_mock_gpio_input_levels[21] = 1;
  assert(hw_btn_read(0U) == 1U);                               /* touch value ignored on VOICEBOX */
  assert(hw_btn_read(3U) == BTN_VOICEBOX_IDLE_LEVEL);
  ```

- [ ] **Step 2: Chạy để thấy fail**
  - Run: `RUNNERS="test_voicebox_runner" bash …/run-tests.sh`
  - Expected: BUILD FAILED (`BTN_VOICEBOX_VOLUME_UP_PIN` undeclared), hoặc assert fail ở chân 21.

- [ ] **Step 3: Implement** các thay đổi trong mục Files.

- [ ] **Step 4: Verify**
  - Run: `bash …/run-tests.sh` — cả 5 runner pass: uart 20, cwsl 11 + `failed=0`, scene 17, voicebox 10, voice 45.
  - Run build firmware (Global Constraints). Kỳ vọng:
    - exit 0;
    - `grep -i warning` trên log, lọc `button.c|m_button.c|app_button|app_volume|app_voice|app_cwsl|process_data|aiio_ci13162_uart`, chỉ còn warning cũ đã biết;
    - báo lại số byte của `build_out/switchv3.bin`.

- [ ] **Step 5: Không commit.**

---

## Kiểm trên board (người dùng làm sau khi chạy xong plan)

1. **Nhấn nút 1 (IO21):** chip beep, App nhận 225 với `act_id` 1. Log có TX `{"cmd":8,"data":[n]}`, hoặc `cmd 11` trước nếu đây là lần nhấn đầu.
2. **Nhấn nút 3 (IO0)** đến mức 1, nhấn thêm lần nữa: vẫn nhận 225 `vol` 1.
3. **Tắt tiếng bằng giọng nói**, rồi:
   - nhấn nút 3: TX `cmd 8 [8]`, 225 `vol` 0;
   - nhấn nút 1: lên mức 1.
4. **Nhấn nhanh nút 2:** chip phát welcome giọng kế tiếp, App nhận 230 với `act_id` 1.
5. **Giữ nút 2:**
   - 5 s: đèn smart config; thả tay thì vào smart config;
   - 10 s: đèn AP config; thả tay thì vào AP config.
6. **Giữ nút 1 quá 10 s:** không đổi đèn, không vào cấu hình.
7. **Trong lúc học lệnh (220), nhấn nút:** không có tiếng gì, mức âm lượng không đổi.
8. **Đo:** từ lúc nhấn đến lúc nghe beep. Nếu thấy chậm thì báo lại, vì lệnh đi qua hàng đợi và task ứng dụng chạy chu kỳ 50 ms.
