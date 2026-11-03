# Plan: WB2 nhận lệnh nhà 71..103 qua `cmd 12`

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

Trạng thái: **đã duyệt** ngày 2026-11-03. Người dùng chọn chạy native (Claude tự làm trong phiên).

**Goal:** Frame `{"cmd":12,"data":[<id>],"cmd_id":<id>,"cwsl":..}` với `id` 71..103 không còn bị báo MQTT 299. VOICEBOX xử lý như ngữ cảnh: nháy LED 200 ms, phát BLE ADV, bắn MQTT 224.

**Architecture:**
- **Tầng UART** (`aiio_ci13162_uart.c`): luật kiểm `cmd 12` nhận thêm dạng lệnh nhà. Đường dispatch VOICEBOX đã gọi `app_cwsl_notify_voice_cmd()` cho mọi `CMD_SCENE` hợp lệ, không cần sửa.
- **Tầng CWSL** (`app_cwsl.c`): `app_cwsl_notify_voice_cmd()` cho `cmd_id` 71..103 đi tiếp. Không kiểm scene mask. Phần LED, BLE, 224 dùng lại code có sẵn.

**Tech Stack:** C (gnu11), FreeRTOS, BL602 SDK, cJSON. Host test bằng gcc với mock trong `smarthome_devices/test_mocks/`.

**Spec:**
- `conversation/claude/007_lenh-nha-cmd12/2026-11-03/001_lam-ro-yeu-cau-lenh-nha-cmd12.md` (quyết định của người dùng);
- contract chip: `docs/voice/commands.md` mục 2.1 (`CMD_SCENE`).

## Global Constraints

- Repo `/home/taodc/wb2/ai-wb2-baremetal`, branch `voiceboxv1/taodc/dev_261102`, HEAD `6fe9bffa`. **Không commit.** Không sửa SDK.
- Working tree đang có thay đổi chưa commit của việc khác, trong:
  - `test_app_cwsl_runner.c`, `test_uart_protocol_runner.c`, `test_voicebox_runner.c`, `test_app_voice_runner.c`;
  - `docs/voice/uart_wb2_ci13162.md`, `docs/voice/hnn_voiceboxv1.xlsx`.

  Giữ nguyên các thay đổi đó; chỉ thêm hoặc sửa đúng chỗ plan nêu.
- Lệnh nhà là `cmd_id` 71..103 (model 2026-11-01). Frame hợp lệ: `data_count == 1`, `data[0]` trong 71..103, `cmd_id == data[0]`, `cwsl` 0..14 hoặc 255 (luật `cwsl` chung, đã có).
- Frame ngữ cảnh giữ nguyên: `data[0]` 1..15 (`APP_SCENE_COUNT_MAX`), `cmd_id == 55 + data[0]`.
- Mọi frame `cmd 12` khác: `AIIO_CI13162_PARSE_INVALID_DATA`, vẫn đẩy MQTT 299 như hiện tại. `data_count != 1` vẫn là `INVALID_FORMAT`.
- Lệnh nhà trên VOICEBOX:
  - `m_led_stt_flash_pulse(200)`;
  - `m_ble_send_voicebox_state(cmd_id)`;
  - publish `{"<type>":224,"cmd_id":<cmd_id>,"cwsl":<cwsl>}`.

  Không gọi `app_scene_is_cmd_id_active()`. Không gọi `m_fan_need_buzz()`.
- Không đổi `APP_CWSL_CMD_ID_MAX` (70), luật 220/221/223, luật `cmd 6`, `m_ble_send_voicebox_state()`, format 224.
- Code mới theo style file đang sửa: hậu tố `U`, comment tiếng Anh, không thêm warning.
- Lệnh test host (chạy trong `smarthome_devices/`):
  ```bash
  OUT=/tmp/claude-1000/-home-taodc-wb2-ai-wb2-baremetal/8eb723e7-8ac5-43c3-867a-51747c0bdc4b/scratchpad/lenh-nha
  mkdir -p $OUT
  INC="-Itest_mocks -I. -Icomponents/app_config -Icomponents/application -Icomponents/hardware -Icomponents/third_party/cJson -Icomponents/middle/ota -Icomponents/middle/fan -Icomponents/middle/gpio -Icomponents/middle/mqtt -Icomponents/middle/database -Icomponents/middle/wifi -Icomponents/middle/ble_slave -Icomponents/middle/event_driven -Icomponents/middle/http"
  gcc -std=gnu11 -w $INC <runner>.c -o $OUT/<runner> -lm && $OUT/<runner>
  ```
  `<runner>` ∈ `test_uart_protocol_runner`, `test_app_cwsl_runner`, `test_app_scene_runner`, `test_voicebox_runner`. Đạt khi exit 0 và dòng cuối là `ALL <N> ... PASSED 100%!`. `test_app_cwsl_runner` còn phải in `[CWSL][TEST] validation total=52 failed=0`.
- Kết quả gốc trước khi sửa (đã chạy): uart `ALL 20`, cwsl `ALL 11` (`validation total=52 failed=0`), scene `ALL 17`, voicebox `ALL 10`. Khi thêm khối test, cập nhật số trong dòng `ALL <N> ...`.
- Build firmware: `cd smarthome_devices && BL60X_SDK_PATH=$HOME/wb2/Ai-Thinker-WB2/ CONFIG_CHIP_NAME=BL602 make -j16`, đạt khi exit 0.

## Review Focus

1. Biên dải ở dạng `cmd_id == data`:
   - 70 → `INVALID_DATA`, vì 70 là ngữ cảnh và phải đi dạng `data` 15;
   - 71 và 103 → OK;
   - 104 → `INVALID_DATA`.

   Test ở Task 1.
2. Cặp lệch:
   - `data` 102, `cmd_id` 103;
   - `data` 102, `cmd_id` 157 (= 55 + 102);
   - `data` 16, `cmd_id` 71 (ca cũ, giữ).

   Đều `INVALID_DATA`. Test ở Task 1.
3. Lệnh nhà có `cwsl` 0..14 (mẫu học thêm): parse OK, 224 mang đúng `cwsl`. Test ở Task 1 và Task 2.
4. Không có ngữ cảnh nào được kích hoạt: lệnh nhà vẫn nháy LED, phát BLE, bắn 224. Test ở Task 2, dùng mock scene điều khiển được.
5. Thiết bị không phải VOICEBOX (`fanoem`): `notify(71..103)` bắn 224 nhưng không nháy LED, không phát BLE. Test ở Task 2.

---

### Task 1: UART: luật `cmd 12` nhận lệnh nhà 71..103

**Files:**
- Modify: `smarthome_devices/components/application/app_cwsl.h` — ngay dưới `APP_CWSL_CMD_ID_MAX` (`:16`), thêm 2 define kèm doc comment (lệnh nhà chip báo qua `CMD_SCENE`, model 2026-11-01).
- Modify: `smarthome_devices/components/hardware/aiio_ci13162_uart.c:843-854`, `case AIIO_CI13162_CMD_SCENE` trong `aiio_ci13162_validate_message()`:
  - `data_count != 1U` → `INVALID_FORMAT` (giữ);
  - `data[0]` trong 1..`APP_SCENE_COUNT_MAX` → OK khi `cmd_id == 55U + data[0]`, ngược lại `INVALID_DATA` (giữ);
  - `data[0]` trong `APP_CWSL_HOME_CMD_ID_MIN`..`APP_CWSL_HOME_CMD_ID_MAX` → OK khi `cmd_id == data[0]`, ngược lại `INVALID_DATA`;
  - còn lại → `INVALID_DATA`;
  - bỏ comment `/* problem here */` (`:847`).
- Test: `smarthome_devices/test_uart_protocol_runner.c`.

**Interfaces:**
- Produces: `#define APP_CWSL_HOME_CMD_ID_MIN 71U`, `#define APP_CWSL_HOME_CMD_ID_MAX 103U` trong `app_cwsl.h`. `aiio_ci13162_uart.c` đã include file này.

- [ ] **Step 1: Viết test (fail).** Khối mới "Test 21 - Home command 71..103 (cmd 12)", đặt sau Test 20, trước dòng `ALL`:
  ```c
  aiio_ci13162_message_t msg;
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[71],\"cmd_id\":71,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_OK);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[102],\"cmd_id\":102,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_OK);
  assert(msg.command == AIIO_CI13162_CMD_SCENE && msg.data[0] == 102U && msg.cmd_id == 102U && msg.cwsl == 255U);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[103],\"cmd_id\":103,\"cwsl\":3}", &msg) == AIIO_CI13162_PARSE_OK);
  assert(msg.cmd_id == 103U && msg.cwsl == 3U);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[104],\"cmd_id\":104,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[70],\"cmd_id\":70,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[16],\"cmd_id\":16,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[0],\"cmd_id\":0,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[102],\"cmd_id\":103,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[102],\"cmd_id\":157,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
  assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[15],\"cmd_id\":70,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_OK);
  ```
  Thêm 2 ca đi qua đường byte → process trên VOICEBOX (theo mẫu Test 13):
  - `reset_test_state(); device_type = VOICEBOX;`
    - `aiio_ci13162_handle_line` dòng `{"cmd":12,"data":[102],"cmd_id":102,"cwsl":255}`, rồi `aiio_ci13162_uart_process()`;
    - kỳ vọng `s_app_send_data_count == 0`, `s_cwsl_notify_count == 1`, `s_last_cwsl_notify_cmd_id == 102U`, `s_last_cwsl_notify_cwsl == 255U`.
  - `reset_test_state();`
    - dòng `{"cmd":12,"data":[104],"cmd_id":104,"cwsl":255}`, rồi process;
    - kỳ vọng `s_app_send_data_count == 1`, `strstr(s_last_app_send_data, "\"voiceboxv1\":299")`, `strstr(..., "\"reason\":\"INVALID_DATA\"")`, `s_cwsl_notify_count == 0`.

    Cuối khối trả `device_type` về giá trị trước khối.

  Đổi dòng cuối thành `ALL 21 PROTOCOL & HARDWARE TESTS PASSED 100%!`. Test 15 giữ nguyên.
- [ ] **Step 2: Chạy `test_uart_protocol_runner`, xác nhận fail** (lỗi compile vì chưa có define, hoặc assert ca 71/102 fail).
- [ ] **Step 3: Sửa code theo phần Files ở trên.**
- [ ] **Step 4: Chạy cả 4 runner, xác nhận đạt.** uart `ALL 21`, các runner khác giữ số cũ.
- [ ] **Step 5: Không commit.** Báo danh sách file đã sửa.

---

### Task 2: CWSL: `app_cwsl_notify_voice_cmd()` nhận lệnh nhà

**Files:**
- Modify: `smarthome_devices/components/application/app_cwsl.c:1451`. Điều kiện bỏ qua đổi thành: bỏ khi `cmd_id == 0U`, hoặc khi `cmd_id > APP_CWSL_CMD_ID_MAX` mà không nằm trong `APP_CWSL_HOME_CMD_ID_MIN`..`APP_CWSL_HOME_CMD_ID_MAX`. Phần còn lại giữ nguyên:
  - gộp 1..6 thành 1;
  - kiểm scene chỉ cho 56..70;
  - `cmd_id >= 7U` trên VOICEBOX thì LED + BLE;
  - publish 224.
- Test: `smarthome_devices/test_app_cwsl_runner.c`:
  - mock `app_scene_is_cmd_id_active` (`:136`) đổi thành trả `s_mock_scene_active` (`static bool`, mặc định `true`);
  - `reset_cwsl_mocks()` đặt lại `s_mock_scene_active = true`.

**Interfaces:**
- Consumes: `APP_CWSL_HOME_CMD_ID_MIN`, `APP_CWSL_HOME_CMD_ID_MAX` (Task 1).
- Produces: không có hàm mới.

- [ ] **Step 1: Viết test (fail).**
  - Test 5 (`:452-456`): ca `app_cwsl_notify_voice_cmd(71U, 255U)` đổi kỳ vọng thành `s_publish_count == 1` và `strcmp(s_last_published, "{\"fanoem\":224,\"cmd_id\":71,\"cwsl\":255}") == 0`. Comment `/* Invalid command IDs (0, 71) */` sửa cho khớp. Ca `0U` giữ kỳ vọng không publish.
  - Khối mới "Test 12 - Home command notify 71..103", sau Test 11, trước dòng `ALL`:
    1. `reset_cwsl_mocks(); device_type = 10; s_mock_scene_active = false;`
       - `app_cwsl_notify_voice_cmd(102U, 255U)`;
       - kỳ vọng `s_publish_count == 1`, `strcmp(s_last_published, "{\"voiceboxv1\":224,\"cmd_id\":102,\"cwsl\":255}") == 0`;
       - kỳ vọng `s_mock_led_pulse_count == 1`, `s_mock_led_pulse_duration == 200`, `s_mock_ble_beacon_count == 1`, `s_mock_ble_beacon_cmd_id == 102U` (Review Focus 4).
    2. `reset_cwsl_mocks(); device_type = 10;` `notify(71U, 255U)` và `notify(103U, 3U)`:
       - `s_publish_count == 2`;
       - `s_last_published` là `{"voiceboxv1":224,"cmd_id":103,"cwsl":3}`;
       - `s_mock_ble_beacon_cmd_id == 103U` (Review Focus 3).
    3. `reset_cwsl_mocks(); device_type = 10;` `notify(104U, 255U)` và `notify(255U, 255U)`: `s_publish_count == 0`, `s_mock_led_pulse_count == 0`, `s_mock_ble_beacon_count == 0`.
    4. `reset_cwsl_mocks(); device_type = 0;` (`fanoem`) `notify(102U, 255U)` → `{"fanoem":224,"cmd_id":102,"cwsl":255}`, `s_mock_led_pulse_count == 0`, `s_mock_ble_beacon_count == 0` (Review Focus 5).
    5. Hồi quy ngữ cảnh: `reset_cwsl_mocks(); device_type = 10; s_mock_scene_active = false;` `notify(56U, 255U)` → `s_publish_count == 0`, `s_mock_ble_beacon_count == 0`.

    Cuối khối: `device_type = 0;`.
  - Đổi dòng cuối thành `ALL 12 CWSL TEST SCENARIOS PASSED 100%!`.
- [ ] **Step 2: Chạy `test_app_cwsl_runner`, xác nhận fail** (ca 71 ở Test 5 và Test 12.1).
- [ ] **Step 3: Sửa code theo phần Files ở trên.**
- [ ] **Step 4: Chạy cả 4 runner, xác nhận đạt.** cwsl `ALL 12` và `validation total=52 failed=0`; uart `ALL 21`; scene `ALL 17`; voicebox `ALL 10`.
- [ ] **Step 5: Không commit.** Báo danh sách file đã sửa.

---

### Task 3: Tài liệu và kiểm tra cuối

**Files:**
- Modify: `docs/voice/uart_wb2_ci13162.md`:
  - mục 4, dòng `| 12 SCENE |` (`:97`): `data` hợp lệ `[1..15]` hoặc `[71..103]`; `cmd_id` là `55 + data` (56..70) hoặc `= data` (71..103). Xử lý: báo MQTT 224, nháy LED, BLE ADV; ngữ cảnh chỉ khi đang được cấu hình, lệnh nhà luôn báo;
  - mục 4, câu về MQTT 224 (`:122`): hàm bỏ `cmd_id` 0, và bỏ `cmd_id` > 70 trừ dải lệnh nhà 71..103;
  - mục 7, bỏ dòng `CMD_SCENE (12)` (`:197`). Dòng `CMD_CWSL_INFO (6)` (`:198`) giữ nguyên.
- Không sửa `commands.md`. Đó là spec phía chip, vốn đã đúng.

**Interfaces:** không có.

- [ ] **Step 1: Sửa tài liệu như trên.** Mọi câu phải khớp code sau Task 1–2.
- [ ] **Step 2: Chạy cả 4 runner, xác nhận đạt** (uart 21, cwsl 12 `failed=0`, scene 17, voicebox 10). Build firmware: exit 0, không có warning ở `aiio_ci13162_uart.c` và `app_cwsl.c`. Báo kích thước `build_out/switchv3.bin`.
- [ ] **Step 3: Không commit.** Liệt kê việc kiểm trên board:
  - nói "BẬT-ĐÈN": log có `[CI13162] voicebox cmd=0x0C cmd_id=102`; không có MQTT 299; có `{"voiceboxv1":224,"cmd_id":102,"cwsl":255}`; LED nháy; log `m_ble_send_voicebox_state cmd_id: 102`;
  - nói "KÍCH-HOẠT-NGỮ-CẢNH-MỘT" khi chưa thêm ngữ cảnh 56: không có 224 (như cũ);
  - nói lệnh âm lượng: vẫn chạy như cũ.

## Không làm

- Không mở rộng 220/221/223 và frame `cmd 6` cho `cmd_id` 71..103.
- Không đổi `APP_CWSL_CMD_ID_MAX`.
- Không ánh xạ lệnh nhà sang hành động trên WB2 (WB2 chỉ báo 224 và BLE ADV).
- Không sửa firmware chip, `commands.md`, `hnn_voiceboxv1.xlsx`.
