#include <Arduino.h>
#include "ui.h"
#include "screen_digital.h"
#include "frame_io.h"

// Biến trạng thái nội bộ
static DigitalState_t digital_state = DIGITAL_STATE_IDLE;   // Trạng thái hiện tại của chế độ Digital
static uint16_t current_srpm = 0;                           // Giá trị SRPM đang cài đặt
static uint16_t current_inc[4] = {0, 50, 100, 500};         // Các mức tăng/giảm RPM
static int8_t inc_index = 0;                                // Chỉ số mức tăng hiện tại        
static unsigned long last_update_time = 0;                  // Thời điểm cập nhật thời gian gần nhất (ms)
static uint16_t elapsed_seconds = 0;                        // Thời gian đã chạy (tính bằng giây)

// Tăng mức thay đổi RPM (Increment step)
void App_Digital_Incre_Inc(void)
{
    inc_index++;
    if (inc_index > 3)
        inc_index = 0;

    lv_label_set_text_fmt(ui_labelDataIncDigital, "%u", current_inc[inc_index]);
}

// Giảm mức thay đổi RPM (Increment step)
void App_Digital_Decre_Inc(void)
{
    inc_index--;
    if (inc_index < 0)
        inc_index = 3;

    lv_label_set_text_fmt(ui_labelDataIncDigital, "%u", current_inc[inc_index]);
}

// Tăng giá trị SRPM
void App_Digital_Incre_SRPM(void)
{
    // Nếu đang dừng hoặc idle thì chuyển sang trạng thái cài đặt SRPM
    if (digital_state == DIGITAL_STATE_STOPPED || digital_state == DIGITAL_STATE_IDLE)
    {
        digital_state = DIGITAL_STATE_SET_SRPM;

        lv_label_set_text(ui_labelDataTimeDigital, "0");
        lv_label_set_text(ui_labelDataMrpmDigital, "0");
        lv_label_set_text(ui_labelPauseDigital, "SET RPM");
    }

    // Tăng giá trị SRPM trong giới hạn
    if (digital_state == DIGITAL_STATE_SET_SRPM)
    {
        current_srpm += current_inc[inc_index];
        if (current_srpm >= SRPM_MAX)
            current_srpm = SRPM_MAX;

        lv_label_set_text_fmt(ui_labelDataSrpmDigital, "%u", current_srpm);
    }
}

// Giảm giá trị SRPM
void App_Digital_Decre_SRPM(void)
{
    if (digital_state == DIGITAL_STATE_STOPPED || digital_state == DIGITAL_STATE_IDLE)
    {
        digital_state = DIGITAL_STATE_SET_SRPM;

        lv_label_set_text(ui_labelDataTimeDigital, "0");
        lv_label_set_text(ui_labelDataMrpmDigital, "0");
        lv_label_set_text(ui_labelPauseDigital, "SET RPM");
    }

    // Giảm giá trị SRPM (không âm)
    if (digital_state == DIGITAL_STATE_SET_SRPM)
    {
        if (current_srpm >= current_inc[inc_index])
            current_srpm -= current_inc[inc_index];
        else
            current_srpm = 0;

        lv_label_set_text_fmt(ui_labelDataSrpmDigital, "%u", current_srpm);
    }
}

// Bắt đầu quá trình chạy coating
void App_Digital_Start(void)
{
    if (digital_state == DIGITAL_STATE_SET_SRPM)
    {
        digital_state = DIGITAL_STATE_RUNNING;

        elapsed_seconds = 0;
        last_update_time = millis();

        // Gửi tín hiệu bắt đầu và giá trị SRPM
        FrameIO_SendData_U8U16(TYPE_MSG_DIGITAL_START, current_srpm);

        lv_label_set_text(ui_labelPauseDigital, "COATING");
    }
}
// Dừng quá trình đang chạy
void App_Digital_Stop(void)
{
    if (digital_state == DIGITAL_STATE_RUNNING)
    {
        digital_state = DIGITAL_STATE_STOPPED;

        // Gửi tín hiệu dừng
        FrameIO_SendData_U8U16(TYPE_MSG_DIGITAL_STOP, 0);

        lv_label_set_text(ui_labelPauseDigital, "PAUSE");
    }
}

// Chuyển về trạng thái IDLE (khi thoát hoặc về Home)
void App_Digital_Set_IDLE(void)
{
    if (digital_state == DIGITAL_STATE_RUNNING)
    {
        FrameIO_SendData_U8U16(TYPE_MSG_DIGITAL_STOP, 0);
    }

    digital_state = DIGITAL_STATE_IDLE;
    current_srpm = 0;
    inc_index = 0;

    lv_label_set_text(ui_labelPauseDigital, "PAUSE");
    lv_label_set_text_fmt(ui_labelDataSrpmDigital, "%u", current_srpm);
    lv_label_set_text(ui_labelDataMrpmDigital, "0");
    lv_label_set_text(ui_labelDataTimeDigital, "0");
    lv_label_set_text_fmt(ui_labelDataIncDigital, "%u", current_inc[inc_index]);
}

// Cập nhật thời gian chạy
void Screen_Digital_UpdateUI_Time(void)
{
    if (millis() - last_update_time >= 1000)
    {
        elapsed_seconds++;
        lv_label_set_text_fmt(ui_labelDataTimeDigital, "%u", elapsed_seconds);
        last_update_time = millis();
    }
}

// Cập nhật giá trị thực tế đo được
void Screen_Digital_UpdateUI_Mrpm(uint16_t mrpm)
{
    lv_label_set_text_fmt(ui_labelDataMrpmDigital, "%u", mrpm);
}

// Lấy trạng thái hiện tại của chế độ Digital
DigitalState_t App_Digital_GetState(void)
{
    return digital_state;
}
