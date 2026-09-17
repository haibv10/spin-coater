#include "frame_io.h"
#include <Arduino.h>

void FrameIO_Init(void)
{
    Serial1.begin(115200, SERIAL_8N1, UART1_RX, UART1_TX);
}

void FrameIO_SendData_U8U16(uint8_t type_msg, uint16_t data)
{
    frame_master_t frameOut;
    frameOut.start_frame = START_BYTE;
    frameOut.type_msg = type_msg;

    if (((data >> 8) & 0xff) != 0)
    {
        uint8_t *bytes = Convert_From_U16_To_Bytes(data);
        frameOut.data[0] = bytes[0];
        frameOut.data[1] = bytes[1];
    }
    else if (((data >> 8) & 0xff) == 0)
    {
        frameOut.data[0] = data & 0xff;
        frameOut.data[1] = 0;
    }

    uint8_t dataOut[32];
    uint8_t frameLen = Msg_Create_Frame_Master(&frameOut, dataOut);
    Serial1.write(dataOut, frameLen);
}

void FrameIO_SenData_Float(uint8_t type_msg, float data)
{
    frame_master_t frameOut;
    frameOut.start_frame = START_BYTE;
    frameOut.type_msg = type_msg;

    uint8_t *bytes = Convert_From_Float_To_Bytes(data);
    frameOut.data[0] = bytes[0];
    frameOut.data[1] = bytes[1];
    frameOut.data[2] = bytes[2];
    frameOut.data[3] = bytes[3];

    uint8_t dataOut[32];
    uint8_t frameLen = Msg_Create_Frame_Master(&frameOut, dataOut);
    Serial1.write(dataOut, frameLen);
}

void FrameIO_SendData_Bytes(uint8_t type_msg, const uint8_t *payload, uint8_t length)
{
    frame_master_t frameOut;
    frameOut.start_frame = START_BYTE;
    frameOut.type_msg = type_msg;

    memcpy(frameOut.data, payload, length);

    uint8_t dataOut[32];
    uint8_t frameLen = Msg_Create_Frame_Master(&frameOut, dataOut);
    Serial1.write(dataOut, frameLen);
}

bool FrameIO_Read(frame_master_t *frameIn)
{
    static uint8_t frameBuffer[32];
    static uint16_t bufferIndex = 0;

    while (Serial1.available())
    {
        uint8_t byteIn = Serial1.read();
        frameBuffer[bufferIndex++] = byteIn;

        if (bufferIndex >= LENGTH_DATA_HEADER)
        {
            uint16_t start = Convert_From_Bytes_To_U16(frameBuffer[0], frameBuffer[1]);
            if (start != START_BYTE)
            {
                for (uint16_t i = 1; i < bufferIndex; i++)
                {
                    frameBuffer[i - 1] = frameBuffer[i];
                }
                bufferIndex--;
                continue;
            }
            uint16_t lenData = frameBuffer[3];
            uint16_t frameLen = LENGTH_DATA_HEADER + lenData + 2;

            if (bufferIndex >= frameLen)
            {
                if (Msg_Check_CRC(frameBuffer, frameLen))
                {
                    Msg_Decode_Frame_Master(frameBuffer, frameIn);

                    bufferIndex = 0;
                    return true;
                }
                else
                {
                    for (uint16_t i = 1; i < bufferIndex; i++)
                    {
                        frameBuffer[i - 1] = frameBuffer[i];
                    }
                    bufferIndex--;
                }
            }
        }

        if (bufferIndex >= 32)
        {
            bufferIndex = 0;
        }
    }
    return false;
}
