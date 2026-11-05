# MQTT 232 set respond_id Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Thêm lệnh MQTT 232 để App chọn câu trả lời (`respond_id`) cho một lệnh thoại bất kỳ có trên chip, qua `cmd 15` của CI13162. Kết quả báo lại App bằng 232.

**Architecture:**
- **`app_cwsl.c`:** tách phần gửi `cmd 15` thành hàm chung `app_cwsl_start_resp_set()`, gồm kiểm OTA/bận, gửi UART và bắt đầu chờ.
  - `app_cwsl_start_scene_resp_set()` của 227 sửa chỉ còn kiểm dải 56..70 rồi gọi hàm chung.
  - Trạng thái chờ, timeout, khoảng bỏ frame muộn và đường kết quả (`app_scene_on_resp_set_result`) giữ nguyên. Vì dùng chung `s_app_cwsl_resp_set` nên mọi chỗ đang chặn bận theo `app_cwsl_is_pending()` tự chặn cả 232.
- **`app_scene.c`:** thêm handler MQTT `app_scene_resp_set()`, gồm parse, kiểm `cmd_id`/`respond_id`, gọi hàm chung và publish 232.
  - `app_scene_on_resp_set_result()` nhìn biến `s_app_scene_pending_mqtt` để trả 227 hay 232.
- **`process_data.c`:** đăng ký 232.

**Tech Stack:** C (BL602 SDK, FreeRTOS, cJSON); runner build trên máy bằng gcc với `test_mocks/`.

**Spec:** `conversation/claude/009_set-respond-id-232/2026-11-04/001_lam-ro-yeu-cau-232-set-respond-id.md` (giao thức phía chip: `docs/voice/commands.md` mục 3)

## Global Constraints

- **Repo:** chỉ `/home/taodc/wb2/rada/ai-wb2-2026`.
- **Không commit;** người dùng tự commit. Không sửa SDK `~/wb2/Ai-Thinker-WB2`, `docs/voice/hnn_voiceboxv1.xlsx`, `components/third_party/`.
- **Điều kiện bắt đầu:** người dùng đã commit phần log lỗi 227. `git status` chỉ còn file log trong `conversation/`, ngoài ra không còn thay đổi trong `smarthome_devices/`. Nếu còn thì dừng và báo.
- **Request:** `{"voiceboxv1":232,"u":55519,"cmd_id":48,"respond_id":1,"act_id":0}`. Key là `gs_dev_type[device_type]`, giá trị 232.
  - Bắt buộc có đủ `u` (0..UINT32_MAX), `act_id`, `cmd_id`, `respond_id` (cả ba 0..255, số nguyên).
  - Thiếu field, sai kiểu, không nguyên hoặc ngoài dải thì trả 64. Có field `action` cũng trả 64.
- **`cmd_id` hợp lệ:** 1..6, hoặc 48..103.
  - Ngoài dải thì trả 65.
  - Riêng 56..70 phải đã kích hoạt (`app_scene_is_cmd_id_active`); chưa kích hoạt thì trả 65.
- **`respond_id` hợp lệ:** đúng tập của `app_scene_respond_id_is_valid()` hiện có (0..3, 11..19, 40..47, 50..68, 70, 71). Ngoài tập trả 64.
- **Thứ tự kiểm** (lỗi đầu tiên gặp thì trả ngay):
  1. js_root NULL
  2. `action`
  3. key loại lệnh
  4. `u`
  5. `act_id`
  6. `cmd_id`
  7. `respond_id`
  8. tập `respond_id` (64)
  9. dải `cmd_id` (65)
  10. `app_scene_is_pending()` (67)
  11. ngữ cảnh chưa kích hoạt (65)
  12. `app_cwsl_start_resp_set()`: 69 OTA, 67 bận, 70 gửi UART lỗi
- **UART:** `aiio_ci13162_uart_send_resp_set(cmd_id, respond_id)` gửi `{"cmd":15,"data":[cmd_id,respond_id]}`.
  - Chờ tối đa `AIIO_CI13162_CWSL_QUERY_TIMEOUT_MS` (2000). Chip trả `[1]` là code 0 (thành công), `[0]` là 64; hết giờ là 52, rồi bỏ frame muộn `APP_CWSL_RESP_SET_DRAIN_MS` (2000).
  - Cả ba đi qua `app_scene_on_resp_set_result(code)` như hiện tại.
- **Bản tin App** (đúng thứ tự field):
  - OK: `{"<dev>":232,"u":%lu,"cmd_id":%u,"respond_id":%u,"act_id":%u,"result":1}`
  - Lỗi: `{"<dev>":232,"u":%lu,"cmd_id":%u,"respond_id":%u,"act_id":%u,"result":0,"code":%u}`
  - Trong bản tin lỗi, field nào đọc được thì echo đúng giá trị App gửi, đọc không được thì là 0. Giống 227: đọc best-effort trước khi kiểm.
- **227 không đổi hành vi:** bản tin, mã lỗi, thứ tự kiểm, test hiện có của 227 đều giữ nguyên.
- **Log:**
  - Mỗi chỗ lỗi in `os_trace("[ERROR]: <nguyên nhân>, in[%s]\r\n", ..., __func__)`, tiếng Anh ASCII, kèm giá trị sai.
  - `os_trace` là macro `if (DEBUG_ENABLE == 1) printf`, nên luôn đặt trong khối `{ }`.
- **Định dạng C** (`/home/taodc/wb2/CLAUDE.md`), áp dụng cho code mới và khối đang sửa:
  - `{` mở khối xuống dòng riêng; không viết `} else {`;
  - tối đa 80 cột;
  - tham số xuống dòng thẳng cột sau `(`; điều kiện ngắt sau `&&`/`||`; chuỗi dài tách `"..." "..."`.
- **Quy ước:** comment tiếng Anh, Doxygen theo file bên cạnh, hằng có hậu tố `U`.
- **Lệnh chạy runner:** `RUNNERS="<tên runner>" bash <workspace>/run-tests.sh`, workspace là `/home/taodc/wb2/rada/ai-wb2-2026/.superpowers/sdd/<workspace của plan>/`. Không truyền `RUNNERS` thì chạy cả 5 runner.
  - Controller tạo script này lúc setup, bằng cách chép `/tmp/claude-1000/-home-taodc-wb2-ai-wb2-baremetal/9902f931-8544-4d1f-a8ab-0144142a38b1/scratchpad/rada-227-log/run-tests.sh` (đã trỏ sẵn vào rada) và đổi `OUT`.
  - Baseline:
    - uart ALL 21, scene ALL 17, voicebox ALL 10, voice ALL 46;
    - cwsl abort ở `test_app_cwsl_runner.c:493` (Test 6 cũ, do commit 6168b039), trước đó in `validation total=61 failed=0`.
- **Build firmware:** `cd /home/taodc/wb2/rada/ai-wb2-2026/smarthome_devices && BL60X_SDK_PATH=$HOME/wb2/Ai-Thinker-WB2/ CONFIG_CHIP_NAME=BL602 make -j16`. Kỳ vọng exit 0, `app_cwsl.o`/`app_scene.o`/`process_data.o` được build lại, không có warning ở các file đã sửa.

## Review Focus

1. **232 đến lúc đang bỏ frame cmd 15 muộn** (227 sửa hoặc 232 trước vừa timeout): trả 67, frame muộn không được tính cho lệnh mới.
   - Test: Task 1, Test 14 (d).
2. **227 sửa rồi 232 rồi 227:** mỗi kết quả ra đúng số lệnh và đúng field của lệnh đang chờ, không lẫn `u`/`cmd_id` của lần trước.
   - Test: Task 2, Test 18 (i).
3. **`cmd_id`/`respond_id` sai kiểu** (chuỗi, số lẻ, >255, âm): trả 64, echo 0, không gửi UART.
   - Test: Task 2, Test 18 (e).
4. **Biên dải `cmd_id`** (0/1, 6/7, 47/48, 103/104, 255): đúng nhận/từ chối.
   - Test: Task 2, Test 18 (c).
5. **Ngữ cảnh 56..70 theo mask hiện tại:** đã kích hoạt thì nhận, chưa kích hoạt thì 65.
   - Test: Task 2, Test 18 (f).

---

### Task 1: Hàm chung gửi cmd 15 trong `app_cwsl`, sửa Test 6

**Files:**
- Modify: `smarthome_devices/components/application/app_cwsl.c`:
  - tách `app_cwsl_start_scene_resp_set` thành hàm chung cộng wrapper;
  - giữ nguyên `app_cwsl_trace_scene_busy` và các log đã có, chỉ chuyển sang hàm chung.
- Modify: `smarthome_devices/components/application/app_cwsl.h`: khai báo và Doxygen hàm chung; sửa Doxygen của `app_cwsl_start_scene_resp_set`.
- Test: `smarthome_devices/test_app_cwsl_runner.c`:
  - Test 6, khối "Learn fan command (cmd_id 7)" và "Delete fan command (cmd_id 7)";
  - mock `m_ota_is_busy`;
  - thêm Test 14;
  - dòng tổng kết.

**Interfaces:**
- Produces:
  ```c
  /* Send {"cmd":15,"data":[cmd_id,respond_id]} and wait for the chip reply.
   * The caller validates cmd_id/respond_id. Result arrives through
   * app_scene_on_resp_set_result(): 0 applied, 64 chip [0], 52 timeout.
   * Returns 0 sent, 69 OTA, 67 busy (draining, volume/voice pending,
   * mutating/query/resp_set active), 70 UART TX fail. */
  uint8_t app_cwsl_start_resp_set(uint8_t cmd_id, uint8_t respond_id);
  ```
- `app_cwsl_start_scene_resp_set(cmd_id, respond_id)`: giữ chữ ký và mã trả về.
  - Ngoài 56..70 thì log như hiện tại và trả 65.
  - Trong dải thì `return app_cwsl_start_resp_set(cmd_id, respond_id);`.
- Thứ tự kiểm trong hàm chung giữ đúng như bản scene hiện tại, chỉ bỏ kiểm dải: OTA → draining → volume/voice → phiên bận → gửi UART.

- [ ] **Step 1: Sửa Test 6 theo luật VOICEBOX mới (`cmd_id` ≤ 103)**

  - **Khối "Learn fan command (cmd_id 7)":**
    - đổi comment thành "Learn command cmd_id 7 is allowed on voicebox";
    - thay các assert sau `app_cwsl_learn(req)` bằng:
    ```c
    assert(s_publish_count == 0);
    assert(s_uart_send_count == 1);
    assert(s_mock_uart_action == APP_CWSL_ACTION_LEARN);
    assert(s_mock_uart_data[0] == 7 && s_mock_uart_data[1] == 0 && s_mock_uart_data[2] == 47);
    reset_cwsl_sessions();
    ```
  - **Ngay sau khối đó, thêm case 220 `cmd_id` 104:**
    ```c
    reset_cwsl_mocks();
    req = cJSON_Parse("{\"voiceboxv1\":220,\"cmd_id\":104,\"slot\":0,\"u\":5006,\"act_id\":6,\"respond_id\":47}");
    app_cwsl_learn(req);
    cJSON_Delete(req);
    assert(s_publish_count == 1 && s_uart_send_count == 0);
    assert(strstr(s_last_published, "\"voiceboxv1\":220") != NULL);
    assert(strstr(s_last_published, "\"code\":65") != NULL);
    ```
  - **Khối "Delete fan command (cmd_id 7)":**
    - đổi comment thành "Delete command cmd_id 7 is allowed on voicebox";
    - thay assert bằng:
    ```c
    assert(s_publish_count == 0);
    assert(s_uart_send_count == 1);
    assert(s_mock_uart_action == APP_CWSL_ACTION_DELETE);
    assert(s_mock_uart_data[0] == 7);
    reset_cwsl_sessions();
    ```

- [ ] **Step 2: Thêm mock OTA và Test 14 (failing)**

  - **Mock OTA:**
    - thêm `static bool s_mock_ota_busy = false;`;
    - `m_ota_is_busy()` trả `s_mock_ota_busy`;
    - `reset_cwsl_mocks()` đặt nó về `false`.
  - **Test 14** đặt ngay trước dòng tổng kết; dòng tổng kết đổi thành `ALL 14 CWSL TEST SCENARIOS PASSED 100%!`:
  ```c
  /* TEST 14: generic cmd:15 start used by MQTT 232 */
  {
      printf("--- Test 14: generic respond_id set ---\n");
      device_type = 10;
      cJSON *req;

      /* (a) any chip cmd_id is sent, result goes to app_scene */
      reset_cwsl_mocks();
      reset_cwsl_sessions();
      assert(app_cwsl_start_resp_set(48U, 1U) == 0U);
      assert(s_resp_set_send_count == 1 && s_resp_set_last_cmd_id == 48U && s_resp_set_last_value == 1U);
      assert(app_cwsl_is_pending() == true);

      /* (b) busy both ways while waiting */
      assert(app_cwsl_start_resp_set(1U, 0U) == 67U);
      assert(s_resp_set_send_count == 1);
      req = cJSON_Parse("{\"voiceboxv1\":220,\"cmd_id\":1,\"slot\":0,\"u\":1,\"act_id\":1,\"respond_id\":47}");
      app_cwsl_learn(req);
      cJSON_Delete(req);
      assert(s_uart_send_count == 0);
      assert(strstr(s_last_published, "\"code\":67") != NULL);
      s_registered_resp_set_cb(1U);
      assert(s_scene_resp_set_result_count == 1 && s_scene_resp_set_result_code == 0U);
      assert(app_cwsl_is_pending() == false);

      /* (c) chip rejects */
      reset_cwsl_mocks();
      reset_cwsl_sessions();
      assert(app_cwsl_start_resp_set(103U, 71U) == 0U);
      s_registered_resp_set_cb(0U);
      assert(s_scene_resp_set_result_count == 1 && s_scene_resp_set_result_code == 64U);

      /* (d) timeout, start during drain is busy, late reply dropped */
      reset_cwsl_mocks();
      reset_cwsl_sessions();
      assert(app_cwsl_start_resp_set(6U, 2U) == 0U);
      s_mock_tick = 2000;
      app_cwsl_poll();
      assert(s_scene_resp_set_result_count == 1 && s_scene_resp_set_result_code == 52U);
      assert(app_cwsl_start_resp_set(48U, 1U) == 67U);
      assert(s_resp_set_send_count == 1);
      s_registered_resp_set_cb(1U);
      assert(s_scene_resp_set_result_count == 1);
      assert(app_cwsl_start_resp_set(48U, 1U) == 0U);

      /* (e) voice pending 67, OTA 69, TX fail 70 */
      reset_cwsl_mocks();
      reset_cwsl_sessions();
      s_mock_voice_pending = true;
      assert(app_cwsl_start_resp_set(48U, 1U) == 67U);
      s_mock_voice_pending = false;
      s_mock_ota_busy = true;
      assert(app_cwsl_start_resp_set(48U, 1U) == 69U);
      s_mock_ota_busy = false;
      s_mock_resp_set_ret = -1;
      assert(app_cwsl_start_resp_set(48U, 1U) == 70U);
      assert(app_cwsl_is_pending() == false);
      assert(s_resp_set_send_count == 1);

      /* (f) the 227 wrapper keeps its 56..70 guard */
      reset_cwsl_mocks();
      reset_cwsl_sessions();
      assert(app_cwsl_start_scene_resp_set(48U, 1U) == 65U);
      assert(s_resp_set_send_count == 0);

      printf("  PASS: generic respond_id set verified\n");
  }
  ```

- [ ] **Step 3: Chạy cwsl runner, kỳ vọng build fail**

  Run: `RUNNERS="test_app_cwsl_runner" bash <workspace>/run-tests.sh`
  Expected: `BUILD FAILED` (implicit declaration / undefined `app_cwsl_start_resp_set`).

- [ ] **Step 4: Cài `app_cwsl_start_resp_set` và wrapper, theo Interfaces**

  Thứ tự kiểm và log lấy từ thân `app_cwsl_start_scene_resp_set` hiện tại, bỏ kiểm dải. Đặt hàm chung ngay trước wrapper. Khai báo trong `app_cwsl.h` cạnh `app_cwsl_start_scene_resp_set`.

- [ ] **Step 5: Chạy lại cả 5 runner**

  Run: `bash <workspace>/run-tests.sh`
  Expected:
  - `test_app_cwsl_runner exit=0`, `ALL 14 CWSL TEST SCENARIOS PASSED 100%!`, `validation total=61 failed=0`;
  - 4 runner còn lại đúng baseline.

- [ ] **Step 6: Không commit.** Ghi vào report danh sách file đã sửa.

---

### Task 2: Handler MQTT 232 trong `app_scene`, đăng ký lệnh, tài liệu, build

**Files:**
- Modify: `smarthome_devices/components/application/app_scene.h`: hằng và khai báo `app_scene_resp_set`.
- Modify: `smarthome_devices/components/application/app_scene.c`:
  - biến `static uint32_t s_app_scene_pending_mqtt;` (in log bằng `(unsigned long)`);
  - 2 hàm publish 232;
  - hàm `app_scene_resp_set`;
  - `app_scene_add` ghi `s_app_scene_pending_mqtt = APP_SCENE_CMD_ADD` ở chỗ đang lưu `s_app_scene_pending_uid`;
  - `app_scene_on_resp_set_result` rẽ nhánh theo `s_app_scene_pending_mqtt`, sửa log cho chung.
- Modify: `smarthome_devices/components/application/process_data.c`: thêm `{app_scene_resp_set, 232},` ngay sau `{app_voice_get, 231},`.
- Modify: `docs/voice/uart_wb2_ci13162.md` (xem Step 6).
- Test: `smarthome_devices/test_app_scene_runner.c`: mock `app_cwsl_start_resp_set`, reset trong `reset_mocks()`, Test 18, dòng tổng kết.

**Interfaces:**
- Consumes (Task 1): `uint8_t app_cwsl_start_resp_set(uint8_t cmd_id, uint8_t respond_id);`
- Produces:
  ```c
  /* app_scene.h */
  #define APP_SCENE_CMD_RESP_SET      232U
  #define APP_SCENE_RESP_WAKE_ID_MAX  6U    /* wake words 1..6 */
  #define APP_SCENE_RESP_CMD_ID_MIN   48U   /* volume 48..55, scenes 56..70, home 71..103 */
  #define APP_SCENE_RESP_CMD_ID_MAX   103U
  /** Handle one MQTT 232 request: set the response phrase of one chip command. */
  void app_scene_resp_set(cJSON *js_root);
  ```
- `app_scene_on_resp_set_result(uint8_t code)`: chữ ký giữ nguyên.
  - Nếu `s_app_scene_pending_mqtt == APP_SCENE_CMD_RESP_SET` thì publish 232, OK khi `code == 0`, còn lại là lỗi với `code`.
  - Ngược lại thì giữ đúng hành vi 227 hiện tại.
  - Log lỗi: `"[ERROR]: cmd %lu cmd_id=%u respond_id=%u failed, code=%u, in[%s]\r\n"`.
- 232 lưu vào các biến pending sẵn có (`s_app_scene_pending_cmd_id`, `_respond_id`, `_uid`, `_act_id`) và `s_app_scene_pending_mqtt = APP_SCENE_CMD_RESP_SET`, chỉ sau khi `app_cwsl_start_resp_set` trả 0.
- Format publish 232 đúng như Global Constraints, hard-code `232` trong chuỗi như các helper 227. Dùng `app_scene_publish()` hiện có.

- [ ] **Step 1: Thêm mock và Test 18 (failing)**

  - **Mock**, đặt cạnh mock `app_cwsl_start_scene_resp_set`:
    - `mock_resp_any_ret`, `mock_resp_any_call_count`, `mock_resp_any_last_cmd_id`, `mock_resp_any_last_respond_id`;
    - `uint8_t app_cwsl_start_resp_set(uint8_t cmd_id, uint8_t respond_id)` ghi lại tham số và trả `mock_resp_any_ret`;
    - `reset_mocks()` đặt cả 4 biến về 0.
  - **Test 18** đặt trước dòng tổng kết; dòng tổng kết đổi thành `ALL 18 UNIT TEST SCENARIOS PASSED 100%!`. Mọi case dùng `device_type = 10`, cuối test trả `device_type = 0`.
  - Helper cục bộ `send232(const char *json)`: `cJSON_Parse` → `app_scene_resp_set` → `cJSON_Delete`.

  ```c
  /* (a) success */
  reset_mocks();
  send232("{\"voiceboxv1\":232,\"u\":55519,\"cmd_id\":48,\"respond_id\":1,\"act_id\":0}");
  assert(mock_resp_any_call_count == 1 && mock_resp_any_last_cmd_id == 48 && mock_resp_any_last_respond_id == 1);
  assert(publish_count == 0);
  app_scene_on_resp_set_result(0U);
  assert(strcmp(last_published, "{\"voiceboxv1\":232,\"u\":55519,\"cmd_id\":48,\"respond_id\":1,\"act_id\":0,\"result\":1}") == 0);

  /* (b) chip [0] -> 64, timeout -> 52 */
  reset_mocks();
  send232("{\"voiceboxv1\":232,\"u\":55519,\"cmd_id\":48,\"respond_id\":1,\"act_id\":0}");
  app_scene_on_resp_set_result(64U);
  assert(strcmp(last_published, "{\"voiceboxv1\":232,\"u\":55519,\"cmd_id\":48,\"respond_id\":1,\"act_id\":0,\"result\":0,\"code\":64}") == 0);
  reset_mocks();
  send232("{\"voiceboxv1\":232,\"u\":55519,\"cmd_id\":48,\"respond_id\":1,\"act_id\":0}");
  app_scene_on_resp_set_result(52U);
  assert(strstr(last_published, "\"result\":0,\"code\":52}") != NULL);
  ```

  - **(c) Biên `cmd_id`:**
    - với `respond_id` 1 và `u` 9, mỗi `cmd_id` trong {1, 6, 48, 55, 71, 103}: sau mỗi lần thì `mock_resp_any_call_count` tăng 1 và `publish_count` vẫn 0;
    - mỗi `cmd_id` trong {0, 7, 47, 104, 255}: bản tin đúng bằng `{"voiceboxv1":232,"u":9,"cmd_id":<id>,"respond_id":1,"act_id":0,"result":0,"code":65}`, call count không đổi.
  - **(d) Tập `respond_id`**, với `cmd_id` 48:
    - {0, 3, 11, 19, 40, 47, 50, 68, 70, 71} được gửi;
    - {4, 10, 39, 48, 69, 72, 99} trả `...,"respond_id":<id>,"act_id":0,"result":0,"code":64}`, không gọi mock.
  - **(e) Request sai, đều 64 và không gọi mock.** So khớp đúng chuỗi:
    - NULL (`app_scene_resp_set(NULL)`): `{"voiceboxv1":232,"u":0,"cmd_id":0,"respond_id":0,"act_id":0,"result":0,"code":64}`
    - thiếu `u`: `{"voiceboxv1":232,"cmd_id":48,"respond_id":1,"act_id":3}` → `{"voiceboxv1":232,"u":0,"cmd_id":48,"respond_id":1,"act_id":3,"result":0,"code":64}`
    - thiếu `act_id`: `{"voiceboxv1":232,"u":5,"cmd_id":48,"respond_id":1}` → `{"voiceboxv1":232,"u":5,"cmd_id":48,"respond_id":1,"act_id":0,"result":0,"code":64}`
    - các dạng sau đều echo `"cmd_id":0`, các field khác giữ giá trị gửi:
      - `cmd_id` là `"48"`, `48.5`, `300`, `-1`;
      - thiếu `cmd_id`.
    - `respond_id` là `"1"`, hoặc thiếu `respond_id`: echo `"respond_id":0`.
    - có `"action":1`: echo đủ field.
    - giá trị lệnh sai `{"voiceboxv1":231,...}`: echo đủ field.
  - **(f) Ngữ cảnh:**
    - `mock_flash_mask = 0x0000; app_scene_init();` → `cmd_id` 56 trả 65;
    - `mock_flash_mask = 0x0001; app_scene_init();` → `cmd_id` 56 được gửi, `cmd_id` 57 trả 65.
  - **(g) Đang xóa tất cả:** `s_app_scene_delete_all_active = 1U;` → `cmd_id` 48 trả `"code":67`, không gọi mock. Sau đó đặt lại 0.
  - **(h) Lỗi từ hàm chung:** lần lượt `mock_resp_any_ret` = 67, 69, 70 → bản tin `{"voiceboxv1":232,"u":55519,"cmd_id":48,"respond_id":1,"act_id":0,"result":0,"code":<N>}`.
  - **(i) Rẽ nhánh kết quả:**
    ```c
    reset_mocks();
    mock_flash_mask = 0x0001;
    app_scene_init();
    /* 227 edit */
    cJSON *r = cJSON_Parse("{\"voiceboxv1\":227,\"u\":7,\"cmd_id\":56,\"act_id\":2,\"respond_id\":50}");
    app_scene_add(r);
    cJSON_Delete(r);
    app_scene_on_resp_set_result(0U);
    assert(strcmp(last_published, "{\"voiceboxv1\":227,\"cmd_id\":56,\"respond_id\":50,\"u\":7,\"act_id\":2,\"result\":1}") == 0);
    /* then 232 */
    send232("{\"voiceboxv1\":232,\"u\":8,\"cmd_id\":71,\"respond_id\":0,\"act_id\":4}");
    app_scene_on_resp_set_result(0U);
    assert(strcmp(last_published, "{\"voiceboxv1\":232,\"u\":8,\"cmd_id\":71,\"respond_id\":0,\"act_id\":4,\"result\":1}") == 0);
    /* then 227 edit again */
    r = cJSON_Parse("{\"voiceboxv1\":227,\"u\":9,\"cmd_id\":56,\"act_id\":5,\"respond_id\":47}");
    app_scene_add(r);
    cJSON_Delete(r);
    app_scene_on_resp_set_result(64U);
    assert(strcmp(last_published, "{\"voiceboxv1\":227,\"cmd_id\":56,\"respond_id\":47,\"u\":9,\"act_id\":5,\"result\":0,\"code\":64}") == 0);
    ```

- [ ] **Step 2: Chạy scene runner, kỳ vọng build fail**

  Run: `RUNNERS="test_app_scene_runner" bash <workspace>/run-tests.sh`
  Expected: `BUILD FAILED` (thiếu `app_scene_resp_set`).

- [ ] **Step 3: Cài phần `app_scene` theo Interfaces và Global Constraints**

  - Kiểm dải `cmd_id` bằng `static bool app_scene_resp_cmd_id_exists(uint32_t cmd_id)`, dùng 3 hằng mới.
  - Log từng nhánh lỗi theo kiểu `app_scene_add`. Lỗi do `app_cwsl_start_resp_set` trả về thì không log lại, vì hàm chung đã log.

- [ ] **Step 4: Đăng ký 232 trong `process_data.c`**

  Thêm `{app_scene_resp_set, 232},` ngay sau `{app_voice_get, 231},`. Kiểm: `grep -n "app_scene_resp_set, 232" smarthome_devices/components/application/process_data.c` ra đúng 1 dòng.

- [ ] **Step 5: Chạy cả 5 runner**

  Run: `bash <workspace>/run-tests.sh`
  Expected:
  - scene `ALL 18 UNIT TEST SCENARIOS PASSED 100%!`;
  - cwsl `ALL 14`, `validation total=61 failed=0`;
  - uart ALL 21, voicebox ALL 10, voice ALL 46.

- [ ] **Step 6: Sửa `docs/voice/uart_wb2_ci13162.md`** (văn phong như các dòng bên cạnh, mọi câu phải khớp code)

  - **Bảng file đầu tài liệu:** thêm dòng `application/app_scene.c`: "MQTT 227/228/229 (ngữ cảnh) và 232 (đặt câu trả lời qua cmd 15)".
  - **Dòng cmd 15 ở mục 3** ("Chỉ dùng cho MQTT 227 sửa câu phản hồi"):
    - nói rõ có hai đường: 227 sửa (`cmd_id` 56..70 đã kích hoạt) và 232;
    - 232 nhận `cmd_id` 1..6, 48..103; 56..70 phải đã kích hoạt; `respond_id` theo tập lọc của 227;
    - giữ câu chờ 2 s.
  - **Dòng cmd 15 ở mục 4** (`[1]` → 227 thành công): `[1]`/`[0]` trả cho lệnh đang chờ, 227 hoặc 232; `[0]` là code 64.
  - **Câu bận ở mục 3** (dòng "Chỉ chạy một thao tác CWSL..."): thêm 232 đang chờ cmd 15.
  - **Câu bận của nút nhấn** ("có 220..231"): đổi thành "có 220..232".
  - **Câu timeout ở mục 5:** thêm "đặt câu trả lời (227 sửa, 232) chờ chip 2 s và khoảng chờ frame muộn 2 s".
  - **Bảng mục 7, dòng `CMD_RESP_SET` (15):** "**Đã cài**, cho MQTT 227 sửa câu phản hồi và MQTT 232".

- [ ] **Step 7: Build firmware**

  Run: lệnh build trong Global Constraints.
  Expected:
  - exit 0;
  - log có `CC build_out/application/app_cwsl.o`, `app_scene.o`, `process_data.o`;
  - `grep -n "app_scene\|app_cwsl\|process_data" <make log> | grep -i warning` không ra dòng nào;
  - ghi kích thước `build_out/switchv3.bin` vào report.

- [ ] **Step 8: Không commit.** Ghi vào report danh sách file đã sửa.

## Kiểm trên board (người dùng làm sau khi chạy xong plan)

- `{"voiceboxv1":232,"u":55519,"cmd_id":48,"respond_id":1,"act_id":0}`:
  - UART TX đúng `{"cmd":15,"data":[48,1]}`;
  - chip trả `[1]`, App nhận `result:1`;
  - nói lệnh tăng âm lượng thì nghe câu mặc định.
- Đặt `cmd_id` 89 (bật ti vi) với `respond_id` 50, nói "bật ti vi": nghe câu "đèn sáng". Thử thêm `respond_id` 0 (ngẫu nhiên).
- `cmd_id` 1 (wake): đổi câu đáp wake word.
- Tắt nguồn bật lại: câu đã đặt vẫn còn (chip lưu NV).
- Gửi 232 hai lần liền nhau: lần 2 nhận 67 nếu lần 1 còn chờ chip.
- `respond_id` thuộc tập lọc nhưng không giọng nào có câu đó: chip trả `[0]`, App nhận 64.
