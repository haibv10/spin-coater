#include <Arduino.h>
#include "lvgl/lvgl_init.h"
#include <ui.h>
#include <ec11.h>
#include <frame_io.h>
#include <app_control.h>

AppMode_t mode = APP_MODE_NONE;
frame_master_t frameIn;

void setup()
{
  lvgl_init();
  ui_init();
  Serial.begin(115200);
  App_Control_Init();
  EC11_Init();
  FrameIO_Init();
}

void loop()
{
  lvgl_loop();
  mode = App_Control_GetCurrentMode();
  if (mode == APP_MODE_ANALOG)
  {
    AnalogState_t analogState = App_Analog_GetState();
    switch (analogState)
    {
    case ANALOG_STATE_SET_SRPM:
      Screen_Analog_UpdateUI_Srpm(EC11_GetSrpm());
      break;
    case ANALOG_STATE_RUNNING:
    {
      Screen_Analog_UpdateUI_Time();

      if (FrameIO_Read(&frameIn))
      {
        if (frameIn.type_msg == TYPE_MSG_ANALOG_MRPM_UPDATE)
        {
          float data_received = Convert_From_Bytes_To_Float(frameIn.data[0], frameIn.data[1], frameIn.data[2], frameIn.data[3]);
          printf("Received MRPM: %.2f\n", data_received);
          Screen_Analog_UpdateUI_Mrpm((uint16_t)data_received);
        }
      }
      break;
    }
    default:
      break;
    }
  }
  else if (mode == APP_MODE_DIGITAL)
  {
    DigitalState_t digitalState = App_Digital_GetState();
    switch (digitalState)
    {
    case DIGITAL_STATE_RUNNING:
    {
      Screen_Digital_UpdateUI_Time();

      if (FrameIO_Read(&frameIn))
      {
        if (frameIn.type_msg == TYPE_MSG_DIGITAL_MRPM_UPDATE)
        {
          float data_received = Convert_From_Bytes_To_Float(frameIn.data[0], frameIn.data[1], frameIn.data[2], frameIn.data[3]);
          printf("Received MRPM: %.2f\n", data_received);
          Screen_Digital_UpdateUI_Mrpm((uint16_t)data_received);
        }
      }
      break;
    }
    default:
      break;
    }
  }
  else if (mode == APP_MODE_RAMP)
  {
    RampState_t rampState = App_Ramp_GetState();
    switch (rampState)
    {
    case RAMP_STATE_RUNNING:
    {
      Screen_Ramp_UpdateUI_Time();

      if (FrameIO_Read(&frameIn))
      {
        if (frameIn.type_msg == TYPE_MSG_RAMP_MRPM_UPDATE)
        {
          float data_received = Convert_From_Bytes_To_Float(frameIn.data[0], frameIn.data[1], frameIn.data[2], frameIn.data[3]);
          printf("Received MRPM: %.2f\n", data_received);
          Screen_Ramp_UpdateUI_Mrpm((uint16_t)data_received);
        }
      }
      break;
    }
    default:
      break;
    }
  }
}