#ifndef FRAME_IO_H
#define FRAME_IO_H

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART1_TX 17
#define UART1_RX 18

void FrameIO_Init(void);
bool FrameIO_Read(frame_master_t *frameIn);
void FrameIO_SendData_U8U16(uint8_t type_msg, uint16_t data);
void FrameIO_SenData_Float(uint8_t type_msg, float data);
void FrameIO_SendData_Bytes(uint8_t type_msg, const uint8_t *payload, uint8_t length);

#ifdef __cplusplus
}
#endif


#endif