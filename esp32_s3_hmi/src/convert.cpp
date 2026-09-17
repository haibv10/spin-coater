#include "convert.h"

uint8_t* Convert_From_Float_To_Bytes(float data)
{
    static data_format_float_bytes input_convert;
    input_convert.data_float = data;
    return input_convert.byte;
}

float Convert_From_Bytes_To_Float(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3)
{
    data_format_float_bytes input_convert;
    input_convert.byte[0] = data0;
    input_convert.byte[1] = data1;
    input_convert.byte[2] = data2;
    input_convert.byte[3] = data3;
    return input_convert.data_float;
}

uint8_t* Convert_From_Int_To_Bytes(uint32_t data)
{
    static data_format_int_bytes input_convert;
    input_convert.data_int = data;
    return input_convert.byte;
}

uint32_t Convert_From_Bytes_To_Int(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3)
{
    data_format_int_bytes input_convert;
    input_convert.byte[0] = data0;
    input_convert.byte[1] = data1;
    input_convert.byte[2] = data2;
    input_convert.byte[3] = data3;
    return input_convert.data_int;
}

uint8_t* Convert_From_U16_To_Bytes(uint16_t data)
{
    static data_format_u8_u16 input_convert;
    input_convert.data_u16 = data;
    return input_convert.byte;
}

uint16_t Convert_From_Bytes_To_U16(uint8_t data0, uint8_t data1)
{
    data_format_u8_u16 input_convert;
    input_convert.byte[0] = data0;
    input_convert.byte[1] = data1;
    return input_convert.data_u16;
}

uint8_t Convert_From_Bytes_To_U8(uint8_t data)
{
    return data;
}
