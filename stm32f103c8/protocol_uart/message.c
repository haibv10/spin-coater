#include "message.h"

// Receive from ESP
void msg_decode_frame_master(uint8_t *data_in, frame_master *frame_out)
{
	uint16_t index = 0;

	frame_out->start_frame = convert_from_bytes_to_u16(data_in[index], data_in[index + 1]);
	index += 2;

	frame_out->type_msg = data_in[index++];

	frame_out->length = convert_from_bytes_to_u8(data_in[index]);
	index += 1;

	for (uint16_t i = 0; i < frame_out->length && i < 12; i++) {
		frame_out->data[i] = data_in[index++];
	}

	frame_out->check_frame = convert_from_bytes_to_u16(data_in[index], data_in[index + 1]);
}

// Respond to ESP
uint8_t msg_create_frame_slave(frame_slave *frame_in, uint8_t *data_out)
{
	uint8_t frame_len = 0;

	if (frame_in->start_frame != START_BYTE)
		return 0;

	switch (frame_in->type_msg) {
	case TYPE_MSG_ANALOG_MRPM_UPDATE:
	case TYPE_MSG_DIGITAL_MRPM_UPDATE:
	case TYPE_MSG_RAMP_MRPM_UPDATE:
		frame_in->length = LENGTH_SLAVE_DATA_MRPM;
		break;
	case TYPE_MSG_RAMP_FINISH:
		frame_in->length = LENGTH_OTHER_DATA_TYPE;
		break;
	default:
		frame_in->length = LENGTH_OTHER_DATA_TYPE;
		break;
	}

	data_out[0] = frame_in->start_frame & 0xFF;
	data_out[1] = (frame_in->start_frame >> 8) & 0xFF;
	data_out[2] = frame_in->type_msg;
	data_out[3] = frame_in->length;

	for (uint8_t i = 0; i < frame_in->length; i++) {
		data_out[4 + i] = frame_in->data[i];
	}

	frame_len = LENGTH_DATA_HEADER + frame_in->length;

	uint16_t crc = msg_calculate_crc(data_out, frame_len);
	frame_in->check_frame = crc;

	data_out[frame_len] = crc & 0xFF;
	data_out[frame_len + 1] = (crc >> 8) & 0xFF;

	frame_len += 2;
	return frame_len;
}

uint16_t msg_calculate_crc(uint8_t *buf, uint8_t length)
{
	uint16_t crc = 0xFFFF;

	for (uint8_t pos = 0; pos < length; pos++) {
		crc ^= (uint16_t)buf[pos];
		for (uint8_t i = 0; i < 8; i++) {
			if (crc & 0x0001) {
				crc >>= 1;
				crc ^= 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}

	return crc;
}

uint8_t msg_check_crc(uint8_t *frame_data, uint16_t buf_len)
{
	if (buf_len < LENGTH_DATA_HEADER + 2)
		return 0;

	uint8_t length = convert_from_bytes_to_u8(frame_data[3]);

	uint16_t crcCalc = msg_calculate_crc(frame_data, LENGTH_DATA_HEADER + length);
	uint16_t crcFrame =
		convert_from_bytes_to_u16(frame_data[LENGTH_DATA_HEADER + length],
					  frame_data[LENGTH_DATA_HEADER + length + 1]);

	if (crcCalc == crcFrame)
		return 1;

	return 0;
}
