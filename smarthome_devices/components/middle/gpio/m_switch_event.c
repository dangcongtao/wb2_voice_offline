#include "m_switch.h"
#include "hardware.h"
#include "m_database.h"

static cyclic_event_switch_t cyclic_timer_arr[MAX_NUM_SWITCH] = {0};
static uint32_t previous_cyclic_timer_active[MAX_NUM_SWITCH] = {0};
static uint32_t step_toggle[MAX_NUM_SWITCH] = {0};
static uint32_t time_diff = 0;

/************** START CONTROL SWITCH WITH CYCLIC TIMER **************/
uint8_t start_cylcic_timer(switch_action_t action, uint32_t time_act1, uint32_t time_act2, uint32_t time_stamp)
{
    uint8_t channel_index = (action - 1) / 2;

    if (channel_index >= m_switch_get_instance().max_number_channel) {
        os_trace("%s out of range number channel\r\n", __func__);
        return 0;
    }

    if (cyclic_timer_arr[channel_index].time_stamp && cyclic_timer_arr[channel_index].time_stamp == time_stamp) {
        os_trace("%s same timestamp\r\n", __func__);
        return 0;
    }
    cyclic_timer_arr[channel_index].time_stamp = time_stamp;

    if (action % 2 == 0)
    { /* off */
        cyclic_timer_arr[channel_index].time_on = time_act2;
        cyclic_timer_arr[channel_index].time_off = time_act1;
    }
    else
    { /* on */
        cyclic_timer_arr[channel_index].time_on = time_act1;
        cyclic_timer_arr[channel_index].time_off = time_act2;
    }
    m_ble_send_t ble_send;
    ble_send.send_ble = true;
    ble_send.need_create_new_id = true;
    ble_send.send_self = true;
    m_switch_get_instance()._control_switch_cb(action, SRC_ACTION_CYCLIC, 0, true, false, ble_send);
    previous_cyclic_timer_active[channel_index] = sys_get_tick_ms();

    step_toggle[channel_index] = 0;

    cyclic_timer_arr[channel_index].enable = 1;
    return 1;
}

void stop_cylcic_timer(uint32_t channel_index)
{
    uint8_t index = channel_index - 1;
    os_trace("stop_cylcic_timer %d\r\n", index);

    cyclic_timer_arr[index].enable = 0;
    cyclic_timer_arr[index].time_on = 0;
    cyclic_timer_arr[index].time_off = 0;
    cyclic_timer_arr[index].time_stamp = 0;
}

/* just use for reset factory database */
void clear_cyclic_timer(void)
{
    os_trace("clear_cyclic_timer - number channel: %d\r\n", m_switch_get_instance().max_number_channel);

    /* start from 1 because mobie app also start from at this point */
    for (uint8_t i = 1; i <= m_switch_get_instance().max_number_channel; i++)
        stop_cylcic_timer(i);
}

void m_switch_cyclic_timer_loop(void)
{
    uint32_t current_tick = 0;

    /* loop for checking event switch controlled by user */
    for (uint8_t i = 0; i < m_switch_get_instance().max_number_channel; i++)
    {
        if (cyclic_timer_arr[i].enable)
        {
            current_tick = sys_get_tick_ms();
            if (previous_cyclic_timer_active[i] <= current_tick)
                time_diff = current_tick - previous_cyclic_timer_active[i];
            else
                time_diff = UINT32_MAX - previous_cyclic_timer_active[i] + current_tick;

            m_ble_send_t ble_send;
            ble_send.send_ble = true;
            ble_send.need_create_new_id = true;
            ble_send.send_self = true;
            switch (step_toggle[i])
            {
            case 0:
            {
                if (time_diff >= cyclic_timer_arr[i].time_on * 1000)
                {
                    m_switch_get_instance()._control_switch_cb(CH1_OFF + 2 * i, SRC_ACTION_CYCLIC, 0, true, false, ble_send);
                    step_toggle[i] = !step_toggle[i]; /* 1 -> 0 , 0 -> 1 */
                    previous_cyclic_timer_active[i] = sys_get_tick_ms();
                }
            }
            break;

            case 1:
            {
                if (time_diff >= cyclic_timer_arr[i].time_off * 1000)
                {
                    m_switch_get_instance()._control_switch_cb(CH1_ON + 2 * i, SRC_ACTION_CYCLIC, 0, true, false, ble_send);
                    step_toggle[i] = !step_toggle[i]; /* 1 -> 0 , 0 -> 1 */
                    previous_cyclic_timer_active[i] = sys_get_tick_ms();
                }
            }
            break;

            default:
                break;
            }
        }
    }
}

/************************* handler release switch api ****************************/
/* this function use button's get_tick_ms() for check timer out of release switch if it's configured.
 * This function will be call from button_task for polling.
 */
static uint32_t ms_tick_control_switch[MAX_NUM_SWITCH];
static uint8_t user_control_switch_event[MAX_NUM_SWITCH];

void set_user_trigger_press_event(uint8_t channel_index, bool set_event, uint32_t action)
{

    if (channel_index >= m_switch_get_instance().max_number_channel)
        return;

    if (!set_event)
        return;

    ms_tick_control_switch[channel_index] = sys_get_tick_ms();

    uint32_t mode = m_db_get_instance()->_m_device._switch_info->release_mode[channel_index];
    if (mode == 0)
        return;

    user_control_switch_event[channel_index] = 1;
}

void m_switch_press_release_loop(void)
{
    static uint32_t time_diff_release = 0;
    uint32_t time_event = 0;
    uint32_t current_state = 0;
    uint32_t mode;

    /* loop for checking event switch controlled by user */
    for (uint8_t i = 0; i < m_switch_get_instance().max_number_channel; i++)
    {
        // time_diff_release = sys_get_tick_ms() - ms_tick_control_switch[i];

        uint32_t current_tick = sys_get_tick_ms();
        if (ms_tick_control_switch[i] <= current_tick)
                time_diff_release = current_tick - ms_tick_control_switch[i];
            else
                time_diff_release = UINT32_MAX - ms_tick_control_switch[i] + current_tick;

        time_event = m_db_get_instance()->_m_device._switch_info->time_release[i] * 1000;

        if (user_control_switch_event[i] && time_diff_release >= time_event)
        {
            user_control_switch_event[i] = 0;
            mode = m_db_get_instance()->_m_device._switch_info->release_mode[i];
            current_state = (m_control_get_status_switch(i) % 2) ? 1 : 2;

            os_trace("release mode/state [%ld:%ld]\r\n", mode, current_state);

            if (mode == MODE_BOTH)
            {
                m_switch_get_instance()._toggle_switch_cb(i, SRC_ACTION_PRESS_MODE, true, false);
            }
            else
            {
                if (current_state == mode)
                {
                    m_switch_get_instance()._toggle_switch_cb(i, SRC_ACTION_PRESS_MODE, true, false);
                }
            }
        }
    }
}
