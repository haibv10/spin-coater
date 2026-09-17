/**
 * @file main.h
 * @brief Header for project Spin Coater.
 * @author haihbv
 * @date December 2025
 */
#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "convert.h"

#define START_BYTE 0xAA55
#define LENGTH_DATA_HEADER 4
#define LENGTH_MASTER_DATA_START 2
#define LENGTH_MASTER_DATA_START_RAMP 8
#define LENGTH_SLAVE_DATA_MRPM 4
#define LENGTH_OTHER_DATA_TYPE 1
#define FRAME_MAX_DATA_LENGTH 12U

// chon mode - Master (ESP32)
typedef enum {
	TYPE_MSG_SET_ANALOG = 0x10,
	TYPE_MSG_SET_DIGITAL = 0x11,
	TYPE_MSG_SET_RAMP = 0x12
} Type_Message_Mode_e;

typedef enum {
	TYPE_MSG_RELAY_VACCUM_ON = 0x13,
	TYPE_MSG_RELAY_VACCUM_OFF = 0x14
} Type_Message_Relay_Vaccum_e;

typedef enum {
	TYPE_MSG_RELAY_UV_LED_ON = 0x15,
	TYPE_MSG_RELAY_UV_LED_OFF = 0x16
} Type_Message_Relay_UV_LED_e;

// Analog - Master (ESP32)
typedef enum {
	TYPE_MSG_ANALOG_START = 0x20,
	TYPE_MSG_ANALOG_STOP = 0x21
} Type_Message_Master_Mode_Analog_e;

// Analog - Slave (STM32)
typedef enum {
	TYPE_MSG_ANALOG_MRPM_UPDATE = 0x31,
} Type_Message_Slave_Mode_Analog_e;

/* Digital - Master (ESP32) */
typedef enum {
	TYPE_MSG_DIGITAL_START = 0x40,
	TYPE_MSG_DIGITAL_STOP = 0x41
} Type_Message_Master_Mode_Digital_e;

/* Digital - Slave (STM32) */
typedef enum {
	TYPE_MSG_DIGITAL_MRPM_UPDATE = 0x51,
} Type_Message_Slave_Mode_Digital_e;

/* Ramp - Master (ESP32) */
typedef enum {
	TYPE_MSG_RAMP_START = 0x60,
	TYPE_MSG_RAMP_STOP = 0x61
} Type_Message_Master_Mode_Ramp_e;

/* Ramp - Slave (STM32) */
typedef enum {
	TYPE_MSG_RAMP_MRPM_UPDATE = 0x71,
	TYPE_MSG_RAMP_FINISH = 0x72, // quay het
} Type_Message_Slave_Mode_Ramp_e;

/* ================== Frame Structures ================== */
typedef struct {
	uint16_t start_frame;
	uint8_t type_msg;
	uint8_t length;
	uint8_t data[12];
	uint16_t check_frame;
} frame_master; // Frame từ ESP32 (Master)

typedef struct {
	uint16_t start_frame;
	uint8_t type_msg;
	uint8_t length;
	uint8_t data[12];
	uint16_t check_frame;
} frame_slave; // Frame từ STM32 (Slave)

// STM32 receives from ESP32 (Master -> Slave)
void msg_decode_frame_master(uint8_t *data_in, frame_master *frame_out);

// STM32 sends response to ESP32 (Slave -> Master)
uint8_t msg_create_frame_slave(frame_slave *frame_in, uint8_t *data_out);

// Common functions
uint16_t msg_calculate_crc(uint8_t *buf, uint8_t length);
uint8_t msg_check_crc(uint8_t *frame_data, uint16_t buf_len);

#endif /* __MESSAGE_H__ */
