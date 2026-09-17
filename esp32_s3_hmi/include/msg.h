#ifndef MSG_H
#define MSG_H

#include <stdint.h>
#include "convert.h"

#define START_BYTE                    0xAA55
/*-----------------------------------------------------
 * Độ dài phần khung (chưa tính dữ liệu và checksum)
 * Gồm: 2 byte start + 1 byte type + 1 byte lenData = 4 byte
 *----------------------------------------------------*/
#define LENGTH_DATA_HEADER            4

#define LENGTH_MASTER_DATA_START      2   // Số byte data trong frame khi master gửi start với digital và analog, data kiểu uin16
#define LENGTH_MASTER_DATA_START_RAMP 8   // Số byte data trong frame khi master gửi start với ramp, data kiểu uint16
#define LENGTH_SLAVE_DATA_MRPM        4   // Số byte data trong frame khi slave gửi mrp, data kiểu float
#define LENGTH_OTHER_DATA_TYPE        1   // Số byte data trong frame cho các loại type message còn lại ví dụ stop,..

/* Chon Mode - Master */
typedef enum
{
	TYPE_MSG_SET_ANALOG = 0x10,
	TYPE_MSG_SET_DIGITAL = 0x11,
	TYPE_MSG_SET_RAMP = 0x12
} Type_Message_Mode_e;

/* Analog - Master */
typedef enum
{
	TYPE_MSG_ANALOG_START = 0x20,
	TYPE_MSG_ANALOG_STOP = 0x21,
} Type_Message_Master_Mode_Analog_e; // esp

/* Analog - Slave */
typedef enum
{
	TYPE_MSG_ANALOG_MRPM_UPDATE = 0x31,
} Type_Message_Slave_Mode_Analog_e; // stm

/* Digital - Master */
typedef enum
{
	TYPE_MSG_DIGITAL_START = 0x40,
	TYPE_MSG_DIGITAL_STOP = 0x41
} Type_Message_Master_Mode_Digital_e; // esp

/* Digital - Slave */
typedef enum
{
	TYPE_MSG_DIGITAL_MRPM_UPDATE = 0x51,
} Type_Message_Slave_Mode_Digital_e; // stm

/* Ramp - Master */
typedef enum
{
	TYPE_MSG_RAMP_START = 0x60,
	TYPE_MSG_RAMP_STOP = 0x61
} Type_Message_Master_Mode_Ramp_e; // esp

/* Ramp - Slave */
typedef enum
{
	TYPE_MSG_RAMP_MRPM_UPDATE = 0x71,
	TYPE_MSG_RAMP_FINISH = 0x72, // quay het
} Type_Message_Slave_Mode_Ramp_e;

typedef struct
{
    uint16_t start_frame;
    uint8_t type_msg;
    uint8_t length_data;
    uint8_t data[12];
    uint16_t check_frame;

} frame_master_t;

uint8_t Msg_Create_Frame_Master(frame_master_t *frameIn, uint8_t *dataOut);
uint16_t Calculate_CRC(uint8_t *buf, uint8_t length);
uint8_t Msg_Check_CRC(uint8_t *frameData, uint16_t bufLen);
void Msg_Decode_Frame_Master(uint8_t *dataIn, frame_master_t *frameOut);

#endif

// sửa leng data thành 1 byte