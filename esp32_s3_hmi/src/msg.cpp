#include "msg.h"

uint8_t Msg_Create_Frame_Master(frame_master_t *frameIn, uint8_t *dataOut)
{
    uint8_t frameLen = 0;

    if (frameIn->start_frame != START_BYTE)
        return 0;

    // Cập nhật length_data
    switch (frameIn->type_msg)
    {
    case TYPE_MSG_ANALOG_START:
        frameIn->length_data = LENGTH_MASTER_DATA_START;
        break;
    case TYPE_MSG_ANALOG_MRPM_UPDATE:
        frameIn->length_data = LENGTH_SLAVE_DATA_MRPM;
        break;
    case TYPE_MSG_DIGITAL_START:
        frameIn->length_data = LENGTH_MASTER_DATA_START;
        break;
    case TYPE_MSG_DIGITAL_MRPM_UPDATE:
        frameIn->length_data = LENGTH_SLAVE_DATA_MRPM;
        break;
    case TYPE_MSG_RAMP_START:
        frameIn->length_data = LENGTH_MASTER_DATA_START_RAMP;
        break;
    case TYPE_MSG_RAMP_MRPM_UPDATE:
        frameIn->length_data = LENGTH_SLAVE_DATA_MRPM;
        break;
    default:
        frameIn->length_data = LENGTH_OTHER_DATA_TYPE;
        break;
    }

    dataOut[0] = frameIn->start_frame & 0xFF;
    dataOut[1] = (frameIn->start_frame >> 8) & 0xFF;
    dataOut[2] = frameIn->type_msg;
    dataOut[3] = frameIn->length_data;

    for (uint8_t i = 0; i < frameIn->length_data; i++)
        dataOut[4 + i] = frameIn->data[i];

    frameLen = LENGTH_DATA_HEADER + frameIn->length_data;

    uint16_t crc = Calculate_CRC(dataOut, frameLen);
    frameIn->check_frame = crc;

    dataOut[frameLen] = crc & 0xFF;
    dataOut[frameLen + 1] = (crc >> 8) & 0xFF;

    frameLen += 2;
    return frameLen;
}

uint16_t Calculate_CRC(uint8_t *buf, uint8_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint8_t pos = 0; pos < length; pos++)
    {
        crc ^= (uint16_t)buf[pos];
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

uint8_t Msg_Check_CRC(uint8_t *frameData, uint16_t bufLen)
{
    if (bufLen < LENGTH_DATA_HEADER + 2)
        return 0;

    uint8_t lengthData = Convert_From_Bytes_To_U8(frameData[3]);

    uint16_t crcCalc = Calculate_CRC(frameData, LENGTH_DATA_HEADER + lengthData);
    uint16_t crcFrame = Convert_From_Bytes_To_U16(frameData[LENGTH_DATA_HEADER + lengthData],
                                                  frameData[LENGTH_DATA_HEADER + lengthData + 1]);

    if (crcCalc == crcFrame)
        return 1;
    else return 0;
}

void Msg_Decode_Frame_Master(uint8_t *dataIn, frame_master_t *frameOut)
{
    uint16_t index = 0;

    frameOut->start_frame = Convert_From_Bytes_To_U16(dataIn[index], dataIn[index + 1]);
    index += 2;

    frameOut->type_msg = dataIn[index++];

    frameOut->length_data = Convert_From_Bytes_To_U8(dataIn[index]);
    index += 1;

    for (uint16_t i = 0; i < frameOut->length_data && i < 12; i++)
    {
        frameOut->data[i] = dataIn[index++];
    }

    frameOut->check_frame = Convert_From_Bytes_To_U16(dataIn[index], dataIn[index + 1]);
}
