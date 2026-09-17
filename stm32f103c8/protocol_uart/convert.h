/**
 * @file main.h
 * @brief Header for project Spin Coater.
 * @author haihbv
 * @date December 2025
 */
#ifndef __CONVERT_H__
#define __CONVERT_H__

#include <stdint.h>

typedef union {
	float data_float;
	uint8_t byte[4];
} float_bytes;

typedef union {
	uint32_t data_int;
	uint8_t byte[4];
} int_bytes;

typedef union {
	uint16_t data_u16;
	uint8_t byte[2];
} u16_bytes;

uint8_t *convert_from_float_to_bytes(float data);
float convert_from_bytes_to_float(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3);
uint8_t *convert_from_int_to_bytes(uint32_t data);
uint32_t convert_from_bytes_to_int(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3);
uint8_t *convert_from_u16_to_bytes(uint16_t data);
uint16_t convert_from_bytes_to_u16(uint8_t data0, uint8_t data1);
uint8_t convert_from_bytes_to_u8(uint8_t data);

#endif /* __CONVERT_H__ */
