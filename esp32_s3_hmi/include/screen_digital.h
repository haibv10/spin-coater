#ifndef SCREEN_DIGITAL_H
#define SCREEN_DIGITAL_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SRPM_MAX 5000

typedef enum {
    DIGITAL_STATE_IDLE = 0,
    DIGITAL_STATE_SET_SRPM,
    DIGITAL_STATE_RUNNING,
    DIGITAL_STATE_STOPPED

} DigitalState_t;

//-----API logic cho button trên màn hình Analog--------
void App_Digital_Incre_Inc(void);
void App_Digital_Decre_Inc(void);
void App_Digital_Incre_SRPM(void);
void App_Digital_Decre_SRPM(void);
void App_Digital_Start(void);
void App_Digital_Stop(void);
void App_Digital_Set_IDLE(void);
//------------------------------------------------------

//------Trả về trạng thái hiện tại-----------------
DigitalState_t App_Digital_GetState(void);
// -----------------------------------------------

//--------API cập nhật màn hình------------------
void Screen_Digital_UpdateUI_Time(void);
void Screen_Digital_UpdateUI_Mrpm(uint16_t mrpm);
//-----------------------------------------------

#ifdef __cplusplus
}
#endif


#endif