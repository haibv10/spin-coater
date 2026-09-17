#include <Arduino.h>
#include "screen_ramp.h"
#include "frame_io.h"
#include "ui.h"

// Biến trạng thái nội bộ
static RampState_t ramp_state = RAMP_STATE_IDLE;
static RampModeParam_t ramp_modes[3] = {
    {0, 0, 0, 0, 0}, // Mode 1
    {0, 0, 0, 0, 0}, // Mode 2
    {0, 0, 0, 0, 0}  // Mode 3
};

static RampModeParam_t temp_rampMode; // Tham số tạm khi setup
static uint8_t current_mode = 0;      // Mode hiện tại
static bool ramp_running = false;     // Đang chạy ramp hay không
static bool ramp_up = true;           // true: min→max, false: max→min
static unsigned long last_update_time = 0;
static uint16_t elapsed_seconds = 0;

static lv_obj_t *ui_barRampProgress = NULL; // Thanh tiến trình
static bool ramp_bar_created = false;
static uint16_t last_bar_update_time = 0;

// Hàm nội bộ tạo tiếng trình (progress bar)
static void Create_RampProgressBar(void)
{
    if (ramp_bar_created)
        return;

    ui_barRampProgress = lv_bar_create(ui_Ramp);
    lv_obj_set_size(ui_barRampProgress, 300, 12);
    lv_obj_align(ui_barRampProgress, LV_ALIGN_CENTER, 0, 95);
    lv_bar_set_range(ui_barRampProgress, 0, 100);
    lv_bar_set_value(ui_barRampProgress, 100, LV_ANIM_OFF);

    // Style nền (phần tĩnh)
    lv_obj_set_style_bg_color(ui_barRampProgress, lv_color_hex(0xE0E0E0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_barRampProgress, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_barRampProgress, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_barRampProgress, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Style indicator (phần hiển thị tiến trình) — màu xám đậm hơn nền
    lv_obj_set_style_bg_color(ui_barRampProgress, lv_color_hex(0xA0A0A0), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_barRampProgress, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_barRampProgress, 6, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    ramp_bar_created = true;
}

// Hàm tiện ích: Xóa/reset thanh tiến trình
static void Reset_RampProgressBar(void)
{
    if (ramp_bar_created && ui_barRampProgress != NULL)
    {
        lv_bar_set_value(ui_barRampProgress, 100, LV_ANIM_OFF);
        lv_obj_del(ui_barRampProgress);
        ui_barRampProgress = NULL;
        ramp_bar_created = false;
    }
}

/*
 * Nhóm hàm chọn mode để cấu hình
 */
void App_Ramp_SelectSetupMode(uint8_t mode)
{
    current_mode = mode;
    temp_rampMode = ramp_modes[current_mode - 1];
    ramp_state = RAMP_STATE_SETUP;

    // Reset màu các nút
    lv_obj_set_style_bg_color(ui_buttonSetupRampMode1, lv_color_hex(0xF2F2F2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_buttonSetupRampMode2, lv_color_hex(0xF2F2F2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_buttonSetupRampMode3, lv_color_hex(0xF2F2F2), LV_PART_MAIN);

    // Đánh dấu nút đang chọn
    switch (mode)
    {
    case 1:
        lv_obj_set_style_bg_color(ui_buttonSetupRampMode1, lv_color_hex(0x808080), LV_PART_MAIN);
        break;
    case 2:
        lv_obj_set_style_bg_color(ui_buttonSetupRampMode2, lv_color_hex(0x808080), LV_PART_MAIN);
        break;
    case 3:
        lv_obj_set_style_bg_color(ui_buttonSetupRampMode3, lv_color_hex(0x808080), LV_PART_MAIN);
        break;
    }

    // Hiển thị thông số của mode hiện tại
    lv_label_set_text_fmt(ui_labelDataMinSpeed, "%u", temp_rampMode.rpm_min);
    lv_label_set_text_fmt(ui_labelDataMinTime, "%u", temp_rampMode.time_min);
    lv_label_set_text_fmt(ui_labelDataMaxSpeed, "%u", temp_rampMode.rpm_max);
    lv_label_set_text_fmt(ui_labelDataMaxTime, "%u", temp_rampMode.time_max);
}

/*
 * Nhóm hàm cấu hình thông số MIN
 */
void App_Ramp_Increase_MIN_Speed(void)
{
    if (ramp_state != RAMP_STATE_SETUP || current_mode == 0)
        return;

    temp_rampMode.rpm_min += STEP_RAMP;

    if (temp_rampMode.rpm_min > 5000)
        temp_rampMode.rpm_min = 5000;

    lv_label_set_text_fmt(ui_labelDataMinSpeed, "%u", temp_rampMode.rpm_min);
}

void App_Ramp_Decrese_MIN_Speed(void)
{
    if (ramp_state != RAMP_STATE_SETUP || current_mode == 0)
        return;

    if (temp_rampMode.rpm_min < STEP_RAMP)
        temp_rampMode.rpm_min = 0;
    else
        temp_rampMode.rpm_min -= STEP_RAMP;

    lv_label_set_text_fmt(ui_labelDataMinSpeed, "%u", temp_rampMode.rpm_min);
}

void App_Ramp_Increase_MIN_Time(void)
{
    if (ramp_state != RAMP_STATE_SETUP || current_mode == 0)
        return;

    temp_rampMode.time_min += STEP_TIME;

    lv_label_set_text_fmt(ui_labelDataMinTime, "%u", temp_rampMode.time_min);
}

void App_Ramp_Decrese_MIN_Time(void)
{
    if (ramp_state != RAMP_STATE_SETUP || current_mode == 0)
        return;

    if (temp_rampMode.time_min < STEP_TIME)
        temp_rampMode.time_min = 0;
    else
        temp_rampMode.time_min -= STEP_TIME;

    lv_label_set_text_fmt(ui_labelDataMinTime, "%u", temp_rampMode.time_min);
}

/*
 * Nhóm hàm cấu hình thông số MAX
 */
void App_Ramp_Increase_MAX_Speed(void)
{
    if (ramp_state != RAMP_STATE_SETUP || current_mode == 0)
        return;

    temp_rampMode.rpm_max += STEP_RAMP;

    if (temp_rampMode.rpm_max > 5000)
        temp_rampMode.rpm_max = 5000;

    lv_label_set_text_fmt(ui_labelDataMaxSpeed, "%u", temp_rampMode.rpm_max);
}

void App_Ramp_Decrese_MAX_Speed(void)
{
    if (ramp_state != RAMP_STATE_SETUP || current_mode == 0)
        return;

    if (temp_rampMode.rpm_max < STEP_RAMP)
        temp_rampMode.rpm_max = 0;
    else
        temp_rampMode.rpm_max -= STEP_RAMP;

    lv_label_set_text_fmt(ui_labelDataMaxSpeed, "%u", temp_rampMode.rpm_max);
}

void App_Ramp_Increase_MAX_Time(void)
{
    if (ramp_state != RAMP_STATE_SETUP || current_mode == 0)
        return;

    temp_rampMode.time_max += STEP_TIME;

    lv_label_set_text_fmt(ui_labelDataMaxTime, "%u", temp_rampMode.time_max);
}

void App_Ramp_Decrese_MAX_Time(void)
{
    if (ramp_state != RAMP_STATE_SETUP || current_mode == 0)
        return;

    if (temp_rampMode.time_max < STEP_TIME)
        temp_rampMode.time_max = 0;
    else
        temp_rampMode.time_max -= STEP_TIME;

    lv_label_set_text_fmt(ui_labelDataMaxTime, "%u", temp_rampMode.time_max);
}

// Lưu cấu hình sau khi set xong các thông số
void App_Ramp_Save(void)
{
    if (current_mode == 0)
        return;
    ramp_modes[current_mode - 1] = temp_rampMode;
    ramp_state = RAMP_STATE_IDLE;
}

// Reset cấu hình sau khi lưu
void App_Ramp_ResetSetup(void)
{
    temp_rampMode = (RampModeParam_t){0, 0, 0, 0, 0};
    ramp_modes[current_mode - 1] = temp_rampMode;

    lv_label_set_text_fmt(ui_labelDataMaxTime, "%u", temp_rampMode.time_max);
    lv_label_set_text_fmt(ui_labelDataMaxSpeed, "%u", temp_rampMode.rpm_max);
    lv_label_set_text_fmt(ui_labelDataMinTime, "%u", temp_rampMode.time_min);
    lv_label_set_text_fmt(ui_labelDataMinSpeed, "%u", temp_rampMode.rpm_min);

    ramp_state = RAMP_STATE_SETUP;
}

// -------------------------- Nhóm hàm Ramp Mode ---------------------------
// Chọn mode trong chế độ Ramp(1-3)
void App_Ramp_SelectMode(uint8_t mode)
{
    if (ramp_running)
        return;

    current_mode = mode;
    ramp_state = RAMP_STATE_SELECTED;

    // Reset màu các nút
    lv_obj_set_style_bg_color(ui_buttonRampMode1, lv_color_hex(0xF2F2F2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_buttonRampMode2, lv_color_hex(0xF2F2F2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_buttonRampMode3, lv_color_hex(0xF2F2F2), LV_PART_MAIN);

    // Đánh dấu nút đang chọn
    switch (mode)
    {
    case 1:
        lv_obj_set_style_bg_color(ui_buttonRampMode1, lv_color_hex(0x808080), LV_PART_MAIN);
        break;
    case 2:
        lv_obj_set_style_bg_color(ui_buttonRampMode2, lv_color_hex(0x808080), LV_PART_MAIN);
        break;
    case 3:
        lv_obj_set_style_bg_color(ui_buttonRampMode3, lv_color_hex(0x808080), LV_PART_MAIN);
        break;
    }

    // Hiển thị dữ liệu mode đã chọn
    lv_label_set_text_fmt(ui_labelDataMinSpeedRampMode, "%u", ramp_modes[current_mode - 1].rpm_min);
    lv_label_set_text_fmt(ui_labelDataMinTimeRampMode, "%u", ramp_modes[current_mode - 1].time_min);
    lv_label_set_text_fmt(ui_labelDataMaxSpeedRampMode, "%u", ramp_modes[current_mode - 1].rpm_max);
    lv_label_set_text_fmt(ui_labelDataMaxTimeRampMode, "%u", ramp_modes[current_mode - 1].time_max);

    Reset_RampProgressBar();
    Create_RampProgressBar();
}

// Bắt đầu coating chế độ Ramp sau khi chọn 1 trong 3 mode
void App_Ramp_Start(void)
{
    if ((ramp_state == RAMP_STATE_SELECTED || ramp_state == RAMP_STATE_STOPPED) && current_mode > 0)
    {
        ramp_state = RAMP_STATE_RUNNING;
        ramp_running = true;
        ramp_up = true;
        elapsed_seconds = 0;
        last_update_time = millis();

        // Gửi dữ liệu khởi động ramp
        uint8_t payLoad[8];
        payLoad[0] = ramp_modes[current_mode - 1].rpm_min & 0xff;
        payLoad[1] = (ramp_modes[current_mode - 1].rpm_min >> 8) & 0xff;
        payLoad[2] = ramp_modes[current_mode - 1].time_min & 0xff;
        payLoad[3] = (ramp_modes[current_mode - 1].time_min >> 8) & 0xff;
        payLoad[4] = ramp_modes[current_mode - 1].rpm_max & 0xff;
        payLoad[5] = (ramp_modes[current_mode - 1].rpm_max >> 8) & 0xff;
        payLoad[6] = ramp_modes[current_mode - 1].time_max & 0xff;
        payLoad[7] = (ramp_modes[current_mode - 1].time_max >> 8) & 0xff;

        // Gửi dữ liệu
        FrameIO_SendData_Bytes(TYPE_MSG_RAMP_START, payLoad, LENGTH_MASTER_DATA_START_RAMP);

        Create_RampProgressBar();
        lv_bar_set_value(ui_barRampProgress, 0, LV_ANIM_OFF);
        lv_label_set_text(ui_labelPauseRamp, "COATING");

        lv_label_set_text_fmt(ui_labelDataMaxTimeRampMode, "%u", ramp_modes[current_mode - 1].time_max);
        lv_label_set_text_fmt(ui_labelDataMinTimeRampMode, "%u", ramp_modes[current_mode - 1].time_min);
    }
}

// Dừng coating
void App_Ramp_Stop(void)
{
    if (ramp_state == RAMP_STATE_RUNNING)
    {
        ramp_state = RAMP_STATE_STOPPED;
        ramp_running = false;

        // Gửi tín hiệu dừng
        FrameIO_SendData_U8U16(TYPE_MSG_RAMP_STOP, 0);

        lv_label_set_text(ui_labelPauseRamp, "PAUSE");
    }
}

// Chuyển về trạng thái IDLE (khi thoát hoặc về Home)
void App_Ramp_Set_IDLE(void)
{
    if (ramp_state == RAMP_STATE_RUNNING)
    {
        FrameIO_SendData_U8U16(TYPE_MSG_RAMP_STOP, 0);
    }

    ramp_state = RAMP_STATE_IDLE;
    ramp_running = false;
    current_mode = 0;

    // Reset toàn bộ giao diện Ramp
    lv_obj_set_style_bg_color(ui_buttonRampMode1, lv_color_hex(0xF2F2F2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_buttonRampMode2, lv_color_hex(0xF2F2F2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_buttonRampMode3, lv_color_hex(0xF2F2F2), LV_PART_MAIN);

    lv_label_set_text(ui_labelDataMinSpeedRampMode, "0");
    lv_label_set_text(ui_labelDataMinTimeRampMode, "0");
    lv_label_set_text(ui_labelDataMaxSpeedRampMode, "0");
    lv_label_set_text(ui_labelDataMaxTimeRampMode, "0");

    Reset_RampProgressBar();
    lv_label_set_text(ui_labelPauseRamp, "PAUSE");
}

// Cập nhật rpm đo được khi đang chạy
void Screen_Ramp_UpdateUI_Mrpm(uint16_t mrpm)
{
    if (ramp_state != RAMP_STATE_RUNNING)
        return;

    // Cập nhật tốc độ đo được lên label
    lv_label_set_text_fmt(ui_labelDataRpmRampMode, "%u", mrpm);
}

// Hàm cập nhật time khi coating
void Screen_Ramp_UpdateUI_Time(void)
{
    static bool phase_min_done = false;

    if (ramp_state != RAMP_STATE_RUNNING)
        return;

    // Cập nhật mỗi giây thời gian còn lại
    if (millis() - last_update_time >= 1000)
    {
        last_update_time = millis();

        if (ramp_up)
        {
            // Pha tăng tốc (MIN)
            if (elapsed_seconds < ramp_modes[current_mode - 1].time_min)
            {
                elapsed_seconds++;
                uint16_t remain = ramp_modes[current_mode - 1].time_min - elapsed_seconds;
                lv_label_set_text_fmt(ui_labelDataMinTimeRampMode, "%u", remain);
            }
            else
            {
                // Hết pha min chuyển qua pha max (giảm tốc)
                ramp_up = false;
                elapsed_seconds = 0;
                phase_min_done = true;
            }
        }
        else
        {
            // Pha giảm tốc (MAX)
            if (elapsed_seconds < ramp_modes[current_mode - 1].time_max)
            {
                elapsed_seconds++;
                uint16_t remain = ramp_modes[current_mode - 1].time_max - elapsed_seconds;
                lv_label_set_text_fmt(ui_labelDataMaxTimeRampMode, "%u", remain);
            }
            else
            {
                // Hoàn tất toàn bộ chu trình ramp
                ramp_state = RAMP_STATE_STOPPED;
                ramp_running = false;
                lv_label_set_text(ui_labelPauseRamp, "DONE");
            }
        }
    }

    // --- Cập nhật thanh tiến trình mỗi 100ms ---
    if (millis() - last_bar_update_time >= 100)
    {
        last_bar_update_time = millis();

        uint16_t total_time = ramp_up
                                  ? ramp_modes[current_mode - 1].time_min
                                  : ramp_modes[current_mode - 1].time_max;

        if (total_time > 0)
        {
            // Tính thời gian đã trôi qua trong pha hiện tại (ms)
            unsigned long elapsed_ms_in_phase = (elapsed_seconds * 1000UL) +
                                                (millis() - last_update_time);
            if (elapsed_ms_in_phase > total_time * 1000UL)
                elapsed_ms_in_phase = total_time * 1000UL;

            // Tiến trình hiển thị ngược lại: 100 về 0
            uint8_t pct = 100 - (uint8_t)((elapsed_ms_in_phase * 100UL) / (total_time * 1000UL));
            lv_bar_set_value(ui_barRampProgress, pct, LV_ANIM_ON);
        }
    }
}

// Trả về trạng thái hiện tại
RampState_t App_Ramp_GetState(void)
{
    return ramp_state;
}
