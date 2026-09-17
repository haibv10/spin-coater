#ifndef SCREEN_ANALOG_H
#define SCREEN_ANALOG_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ANALOG_STATE_IDLE = 0,
    ANALOG_STATE_SET_SRPM,
    ANALOG_STATE_RUNNING,
    ANALOG_STATE_STOPPED 
} AnalogState_t;

//-----API logic cho button trên màn hình Analog--------
void App_Analog_SetRpm(void);
void App_Analog_Start(void);
void App_Analog_Stop(void);
void App_Analog_Set_IDLE(void);
//-------------------------------------------------

//------Trả về trạng thái hiện tại-----------------
AnalogState_t App_Analog_GetState(void);
//-------------------------------------------------

//--------API cập nhật màn hình------------------
void Screen_Analog_UpdateUI_Srpm(uint16_t srpm);
void Screen_Analog_UpdateUI_Mrpm(uint16_t mrpm);
void Screen_Analog_UpdateUI_Time(void);
//-------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif