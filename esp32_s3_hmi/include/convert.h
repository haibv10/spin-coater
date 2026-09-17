#ifndef CONVERT_H
#define CONVERT_H

#include <stdint.h>

typedef union
{
	float data_float;
	uint8_t byte[4];
} data_format_float_bytes;

typedef union
{
	uint32_t data_int;
	uint8_t byte[4];
} data_format_int_bytes;

typedef union
{
	uint16_t data_u16;
	uint8_t byte[2];
} data_format_u8_u16;

// Float <-> Bytes
uint8_t* Convert_From_Float_To_Bytes(float data);
float Convert_From_Bytes_To_Float(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3);

// Int (32-bit) <-> Bytes
uint8_t* Convert_From_Int_To_Bytes(uint32_t data);
uint32_t Convert_From_Bytes_To_Int(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3);

// Unsigned 16-bit <-> Bytes
uint8_t* Convert_From_U16_To_Bytes(uint16_t data);
uint16_t Convert_From_Bytes_To_U16(uint8_t data0, uint8_t data1);

// 8-bit
uint8_t Convert_From_Bytes_To_U8(uint8_t data);

#endif /* CONVERT_H */
