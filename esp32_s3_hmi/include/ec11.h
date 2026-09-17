#ifndef EC11_H
#define EC11_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EC11_PIN_A 14
#define EC11_PIN_B 15

void EC11_Init(void);
uint16_t EC11_GetSrpm(void);
void EC11_ResetSrpm(void);

#ifdef __cplusplus
}
#endif

#endif