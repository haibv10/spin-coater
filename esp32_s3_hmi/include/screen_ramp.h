#ifndef SCREEN_RAMP_H
#define SCREEN_RAMP_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define STEP_RAMP 500
#define STEP_TIME 1

typedef struct
{
    uint16_t rpm_min;
    uint16_t time_min;
    uint16_t rpm_max;
    uint16_t time_max;
    uint16_t ramp_rpm;
} RampModeParam_t;

typedef enum {
    RAMP_STATE_IDLE = 0,      // Chưa chọn mode
    RAMP_STATE_SELECTED,      // Đang chọn mode
    RAMP_STATE_RUNNING,       // Đang chạy
    RAMP_STATE_STOPPED,       // Dừng
    RAMP_STATE_SETUP,
    RAMP_STATE_SETUP_MIN,     // Đang ở màn setup
    RAMP_STATE_SETUP_MAX
} RampState_t;

//------------API điều khiển màn-----------
//-----Màn Ramp chính
void App_Ramp_SelectMode(uint8_t mode);
void App_Ramp_Start(void);
void App_Ramp_Stop(void);
void App_Ramp_Set_IDLE(void);
//-----Màn Setup Ramp
void App_Ramp_SelectSetupMode(uint8_t mode);
void App_Ramp_Increase_MIN_Speed(void);
void App_Ramp_Decrese_MIN_Speed(void);
void App_Ramp_Increase_MIN_Time(void);
void App_Ramp_Decrese_MIN_Time(void);
void App_Ramp_Increase_MAX_Speed(void);
void App_Ramp_Decrese_MAX_Speed(void);
void App_Ramp_Increase_MAX_Time(void);
void App_Ramp_Decrese_MAX_Time(void);
void App_Ramp_Save(void);
void App_Ramp_ResetSetup(void);
void App_SetupRamp_Back(void);
//-----------------------------------------

void Screen_Ramp_UpdateUI_Mrpm(uint16_t mrpm);
void Screen_Ramp_UpdateUI_Time(void);

RampState_t App_Ramp_GetState(void);


#ifdef __cplusplus
}
#endif


#endif