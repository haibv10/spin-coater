#include "ec11.h"
#include <Arduino.h>
#include <ESP32Encoder.h>

ESP32Encoder encoder;
static uint32_t srpm = 0;

void EC11_Init(void)
{
    encoder.attachFullQuad(EC11_PIN_A, EC11_PIN_B);
    encoder.clearCount();
}

uint16_t EC11_GetSrpm(void)
{
    static int64_t lastStep = 0;

    if (encoder.getCount() <= 0)
    {
      encoder.clearCount();
    }

    if (encoder.getCount() > 400)
    {
      encoder.clearCount();
      encoder.setCount(400);
    }

    int64_t rawCount = encoder.getCount();

    int64_t step = rawCount / 4;
    if (step != lastStep)
    {
      srpm = (uint16_t)(step * 50);
      lastStep = step;
    }
    return srpm;
}

void EC11_ResetSrpm(void)
{
  encoder.clearCount();
}
