#include <Arduino.h>
#include "ui.h"
#include "screen_analog.h"
#include "ec11.h"
#include "frame_io.h"

// Biến trạng thái nội bộ
static AnalogState_t analog_state = ANALOG_STATE_IDLE;      // Trạng thái hiện tại của chế độ Analog
static unsigned long last_update_time = 0;                  // Thời điểm cập nhật thời gian gần nhất (ms)
static uint16_t elapsed_seconds = 0;                        // Thời gian đã chạy (tính bằng giây)

// Hàm nội bộ Reset toàn bộ giao diện Analog
static void ResetAnalogUI(void)
{
    lv_label_set_text(ui_labelPauseAnalog, "PAUSE");
    lv_label_set_text(ui_labelDataSrpmAnalog, "0");
    lv_label_set_text(ui_labelDataMrpmAnalog, "0");
    lv_label_set_text(ui_labelDataTimeAnalog, "0");
}

// Cài đặt RPM khi ấn pause, thực hiện quay EC11 để set RPM
void App_Analog_SetRpm(void)
{
    if (analog_state != ANALOG_STATE_RUNNING)
    {
        analog_state = ANALOG_STATE_SET_SRPM;
        EC11_ResetSrpm();

        lv_label_set_text(ui_labelDataMrpmAnalog, "0");
        lv_label_set_text(ui_labelDataTimeAnalog, "0");
        lv_label_set_text(ui_labelPauseAnalog, "SET RPM");
    }
}

// Ấn start để bắt đầu coating và thực hiện đếm thời gian
void App_Analog_Start(void)
{
    if (analog_state == ANALOG_STATE_SET_SRPM)
    {
        analog_state = ANALOG_STATE_RUNNING;

        // Gửi gói tin bắt đầu chạy cùng giá trị SRPM
        FrameIO_SendData_U8U16(TYPE_MSG_ANALOG_START, EC11_GetSrpm());

        // Khởi tạo lại bộ đếm thời gian
        elapsed_seconds = 0;
        last_update_time = millis();

        lv_label_set_text(ui_labelPauseAnalog, "COATING");
    }
}

// Dừng quá trình coating
void App_Analog_Stop(void)
{
    if (analog_state == ANALOG_STATE_RUNNING)
    {
        analog_state = ANALOG_STATE_STOPPED;

        // Gửi gói tin thông báo dừng
        FrameIO_SendData_U8U16(TYPE_MSG_ANALOG_STOP, 0);

        lv_label_set_text(ui_labelPauseAnalog, "PAUSE");
    }
}

// Chuyển về trạng thái IDLE khi quay lại màn hình chính
void App_Analog_Set_IDLE(void)
{
    if (analog_state == ANALOG_STATE_RUNNING)
    {
        // Nếu đang chạy, gửi tín hiệu dừng trước
        FrameIO_SendData_U8U16(TYPE_MSG_ANALOG_STOP, 0);
    }
    
    analog_state = ANALOG_STATE_IDLE;

    ResetAnalogUI();
}

// Update srpm khi set rpm trên giao diện
void Screen_Analog_UpdateUI_Srpm(uint16_t srpm)
{
    lv_label_set_text_fmt(ui_labelDataSrpmAnalog, "%u", srpm);
}

// Update rpm đo được khi coating trên giao diện
void Screen_Analog_UpdateUI_Mrpm(uint16_t mrpm)
{
    lv_label_set_text_fmt(ui_labelDataMrpmAnalog, "%u", mrpm);
}

// Cập nhật time khi chạy mỗi giây
void Screen_Analog_UpdateUI_Time(void)
{
    if (millis() - last_update_time >= 1000)
    {
        elapsed_seconds++;
        lv_label_set_text_fmt(ui_labelDataTimeAnalog, "%u", elapsed_seconds);
        last_update_time = millis();
    }
}

// Lấy trạng thái hiện tại của chế độ analog
AnalogState_t App_Analog_GetState(void)
{
    return analog_state;
}
