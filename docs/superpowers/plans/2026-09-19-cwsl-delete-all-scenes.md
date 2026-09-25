# Xóa Ngữ Cảnh Khi Nhận Lệnh CWSL Delete All (221, cmd_id: 255, slot: 255) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Khi nhận lệnh MQTT 221 xóa toàn bộ (`{"cmd_id":255,"slot":255}`), sau khi chip giọng nói CI13162 xác nhận xóa thành công (`code: 33`), firmware xóa sạch toàn bộ cấu hình ngữ cảnh (`scene_mask = 0`) trong RAM và Flash, đồng bộ trạng thái giữa giọng nói và ngữ cảnh.

**Architecture:** Bổ sung hàm API `app_scene_clear_all()` trong `app_scene.h`/`app_scene.c` để đặt `s_app_scene_mask = 0` và lưu vào EasyFlash. Trong `app_cwsl.c`, tại callback `app_cwsl_on_status()`, khi nhận mã thành công `APP_CWSL_CODE_DELETE_OK` (33) cho yêu cầu `cmd_id == APP_CWSL_ANY && slot == APP_CWSL_ANY`, tiến hành gọi `app_scene_clear_all()`. Cập nhật unit test và tài liệu MQTT spec.

**Tech Stack:** C (Embedded C - Bouffalo SDK / BL602 / FreeRTOS / EasyFlash / cJSON), GCC test runner.

**Spec:** [CWSL_MQTT_SPEC.md](file:///home/huy/WB2/lqh_fangavit/CWSL_MQTT_SPEC.md#L805-L820)

## Global Constraints

- Tuân thủ quy tắc ngôn ngữ: Trả lời bằng tiếng Việt, từ ngữ chuyên ngành dùng tiếng Anh có ( tiếng Việt ) đi kèm.
- Giữ nguyên luồng UART xóa toàn bộ tới chip CI13162: `{"cmd":4,"data":[2,255,255]}\r\n`.
- Giữ nguyên định dạng phản hồi MQTT 221: `{"<dev>":221,"cmd_id":255,"slot":255,"code":33,"u":...,"act_id":...}`.
- Chỉ xóa `scene_mask` khi chip CI13162 phản hồi thành công `code: 33` (DELETE_OK); nếu lỗi hoặc timeout (hết thời gian chờ) thì giữ nguyên dữ liệu (fail-safe).
- Lệnh 221 chỉ xóa ngữ cảnh khi là lệnh xóa toàn bộ (`cmd_id=255, slot=255`). Xóa từng ngữ cảnh đơn lẻ (56..65) vẫn xử lý qua lệnh 228 (`app_scene_delete`).

---

### Task 1: Bổ sung API `app_scene_clear_all()` và kiểm thử trong `app_scene`

**Files:**
- Modify: `smarthome_devices/components/application/app_scene.h`
- Modify: `smarthome_devices/components/application/app_scene.c`
- Test: `smarthome_devices/test_app_scene_runner.c`

**Interfaces:**
- Produces: `void app_scene_clear_all(void);`

- [ ] **Step 1: Viết test case kiểm thử hàm `app_scene_clear_all()` trong `test_app_scene_runner.c`**
  Thêm Test 12 vào `test_app_scene_runner.c` kiểm tra:
  - Khởi tạo mock `scene_mask` có chứa ngữ cảnh (ví dụ: bit 0 và 2 được bật).
  - Gọi `app_scene_clear_all()`.
  - Assert `app_scene_is_active(1) == false`, `app_scene_is_active(3) == false`.
  - Assert mock flash lưu key `"scene_mask"` với giá trị 0.

- [ ] **Step 2: Chạy test để xác nhận test lỗi biên dịch (do chưa có prototype `app_scene_clear_all`)**
  ```bash
  gcc -I smarthome_devices/test_mocks -I smarthome_devices -I smarthome_devices/components/third_party/cJson -I smarthome_devices/components/hardware -I smarthome_devices/components/application -I smarthome_devices/components/app_config -I smarthome_devices/components/middle/ota -I smarthome_devices/components/middle/fan smarthome_devices/test_app_scene_runner.c -o /tmp/test_app_scene_runner
  ```

- [ ] **Step 3: Khai báo prototype trong `app_scene.h` và hiện thực trong `app_scene.c`**
  Trong `app_scene.h`:
  ```c
  /** @brief Clear all active scenes from RAM and Flash immediately. */
  void app_scene_clear_all(void);
  ```
  Trong `app_scene.c`:
  ```c
  void app_scene_clear_all(void)
  {
      s_app_scene_delete_all_active = 0U;
      s_app_scene_mask = 0U;
      (void)app_scene_save_mask(0U);
  }
  ```

- [ ] **Step 4: Chạy lại `test_app_scene_runner` để đảm bảo 12 kịch bản kiểm thử đều đạt 100%**
  ```bash
  gcc -I smarthome_devices/test_mocks -I smarthome_devices -I smarthome_devices/components/third_party/cJson -I smarthome_devices/components/hardware -I smarthome_devices/components/application -I smarthome_devices/components/app_config -I smarthome_devices/components/middle/ota -I smarthome_devices/components/middle/fan smarthome_devices/test_app_scene_runner.c -o /tmp/test_app_scene_runner && /tmp/test_app_scene_runner
  ```

---

### Task 2: Tích hợp gọi `app_scene_clear_all()` trong `app_cwsl.c` khi Delete All thành công

**Files:**
- Modify: `smarthome_devices/components/application/app_cwsl.c`
- Test: `smarthome_devices/test_app_cwsl_runner.c`

**Interfaces:**
- Consumes: `void app_scene_clear_all(void);` từ `app_scene.h`

- [ ] **Step 1: Viết test case trong `test_app_cwsl_runner.c` kiểm tra Delete All kích hoạt `app_scene_clear_all()`**
  Trong `test_app_cwsl_runner.c`:
  - Thêm biến đếm mock `static int s_scene_clear_all_count = 0;`.
  - Định nghĩa mock `void app_scene_clear_all(void) { s_scene_clear_all_count++; }`.
  - Trong kịch bản Test 3 (DELETE Requests - Delete All 255, 255):
    - Kiểm tra trước khi nhận code 33: `s_scene_clear_all_count == 0`.
    - Sau khi `s_registered_status_cb(33U, 255U, 255U)`:
      - Assert `s_scene_clear_all_count == 1`.
    - Thêm trường hợp Delete All bị timeout (`code = 51`) hoặc lỗi:
      - Assert `s_scene_clear_all_count` không tăng (giữ nguyên `scene_mask`).

- [ ] **Step 2: Chạy test runner để xác nhận fail do chưa tích hợp trong `app_cwsl.c`**
  ```bash
  gcc -I smarthome_devices/test_mocks -I smarthome_devices -I smarthome_devices/components/third_party/cJson -I smarthome_devices/components/hardware -I smarthome_devices/components/application -I smarthome_devices/components/app_config -I smarthome_devices/components/middle/ota -I smarthome_devices/components/middle/fan smarthome_devices/test_app_cwsl_runner.c -o /tmp/test_app_cwsl_runner && /tmp/test_app_cwsl_runner
  ```

- [ ] **Step 3: Cập nhật `app_cwsl.c` để gọi `app_scene_clear_all()`**
  Trong `app_cwsl.c`:
  Thêm include `#include "app_scene.h"` (nếu chưa có).
  Trong `app_cwsl_on_status()`:
  Tại nhánh xử lý kết quả xóa thông thường:
  ```c
      if (pending.request.mqtt_cmd == APP_SCENE_CMD_ADD) {
          app_scene_on_learn_result(code, pending.request.cmd_id);
      } else if (pending.request.mqtt_cmd == APP_SCENE_CMD_DELETE) {
          app_scene_on_delete_result(code, pending.request.cmd_id);
      } else {
          if ((pending.request.mqtt_cmd == APP_CWSL_CMD_DELETE) &&
              (pending.request.cmd_id == APP_CWSL_ANY) &&
              (pending.request.slot == APP_CWSL_ANY) &&
              (code == APP_CWSL_CODE_DELETE_OK)) {
              app_scene_clear_all();
          }
          app_cwsl_publish_mutating_final(&pending, cmd_id, slot, code);
      }
  ```

- [ ] **Step 4: Chạy lại `test_app_cwsl_runner` để đảm bảo toàn bộ kiểm thử pass 100%**
  ```bash
  gcc -I smarthome_devices/test_mocks -I smarthome_devices -I smarthome_devices/components/third_party/cJson -I smarthome_devices/components/hardware -I smarthome_devices/components/application -I smarthome_devices/components/app_config -I smarthome_devices/components/middle/ota -I smarthome_devices/components/middle/fan smarthome_devices/test_app_cwsl_runner.c -o /tmp/test_app_cwsl_runner && /tmp/test_app_cwsl_runner
  ```

---

### Task 3: Cập nhật tài liệu kỹ thuật trong `CWSL_MQTT_SPEC.md`

**Files:**
- Modify: `CWSL_MQTT_SPEC.md`

- [ ] **Step 1: Cập nhật mục 12.7 (Delete all) trong `CWSL_MQTT_SPEC.md`**
  Ghi chú rõ ràng:
  - Khi lệnh `{"fanoem":221,"cmd_id":255,"slot":255,...}` thực hiện thành công (`code: 33`), firmware không chỉ xóa toàn bộ mẫu giọng nói trên chip CI13162 mà còn tự động reset `scene_mask = 0` (xóa toàn bộ danh sách ngữ cảnh đã thêm trong Flash và RAM).
  - Không phát sinh thêm bản tin MQTT 228 riêng biệt; App nhận biết trạng thái qua phản hồi kết quả của lệnh 221.

---

### Task 4: Kiểm tra toàn diện (Verification & Sanity Check)

- [ ] **Step 1: Chạy toàn bộ các test runner native**
  - Chạy `test_app_cwsl_runner`
  - Chạy `test_app_scene_runner`
  - Chạy `test_uart_protocol_runner` (nếu có liên quan)
- [ ] **Step 2: Kiểm tra `git diff` đảm bảo mã nguồn gọn gàng, không thừa thãi, tuân thủ coding standard**
