#include "protocol_uart.h"

#define UART_RX_BUFFER_SIZE (32U)
#define UART_MRPM_SEND_INTERVAL_MS (50U)

coater_mode g_current_mode = MODE_IDLE;
bool g_is_running = false;

static void uart_safe_stop(void)
{
	float currentRpm = encoder_get_rpm(COATER_ENCODER_AVG_SAMPLE);
	if (currentRpm > 10.0f) {
		coater_ramp_down(0.0f, RAMP_DEFAULT_RAMP_DOWN_STEPS, RAMP_DEFAULT_RAMP_DOWN_TIME_MS);
	} else {
		esc_set_duty(ESC_MIN_PWM);
		g_coater_ctrl.current_rpm = 0.0f;
		g_coater_ctrl.target_rpm = 0.0f;
		g_coater_ctrl.state = COATER_IDLE;
	}
	// printf("Dung Spin Coating truoc khi chuyen mode\r\n");
}

void uart_process_master_frame(frame_master *frame_master_msg)
{
	float rpm_analog;
	float rpm_digital;
	ramp_params ramp_params_msg = {0};

	switch (frame_master_msg->type_msg) {
	case TYPE_MSG_RELAY_UV_LED_ON:
		RELAY_UV_LED_ON;
		io_buzzer_start(1, 50, 100);
		// printf("UV LED ON\r\n");
		break;
	case TYPE_MSG_RELAY_UV_LED_OFF:
		RELAY_UV_LED_OFF;
		io_buzzer_start(1, 50, 100);
		// printf("UV LED OFF\r\n");
		break;

	case TYPE_MSG_RELAY_VACCUM_ON:
		RELAY_VACCUMP_PUMP_ON;
		io_buzzer_start(1, 50, 100);
		// printf("Vacuum Pump ON\r\n");
		break;
	case TYPE_MSG_RELAY_VACCUM_OFF:
		RELAY_VACCUMP_PUMP_OFF;
		io_buzzer_start(1, 50, 100);
		// printf("Vacuum Pump OFF\r\n");
		break;

	// Set Mode
	case TYPE_MSG_SET_ANALOG:
		if (g_current_mode != MODE_ANALOG) {
			if (g_current_mode != MODE_IDLE) {
				uart_safe_stop();
			}
			g_current_mode = MODE_ANALOG;
			io_buzzer_start(1, 100, 200);
			// printf("[MODE]: ANALOG\r\n");
		}
		break;

	case TYPE_MSG_SET_DIGITAL:
		if (g_current_mode != MODE_DIGITAL) {
			if (g_current_mode != MODE_IDLE) {
				uart_safe_stop();
			}
			g_current_mode = MODE_DIGITAL;
			io_buzzer_start(1, 100, 200);
			// printf("[MODE]: DIGITAL\r\n");
		}
		break;

	case TYPE_MSG_SET_RAMP:
		if (g_current_mode != MODE_RAMP) {
			if (g_current_mode != MODE_IDLE) {
				uart_safe_stop();
			}
			g_current_mode = MODE_RAMP;
			io_buzzer_start(1, 100, 200);
			// printf("[MODE]: RAMP\r\n");
		}
		break;

		// Run Mode
	case TYPE_MSG_ANALOG_START:
		if (g_current_mode != MODE_ANALOG) {
			// printf("Khong chay dc mode analog\r\n");
			return;
		}

		rpm_analog =
			convert_from_bytes_to_u16(frame_master_msg->data[0], frame_master_msg->data[1]);
		mode_analog_or_digital(rpm_analog);
		g_is_running = true;
		io_buzzer_start(2, 100, 200); // Beep khi start
		// printf("ANALOG Start: Target RPM = %.2f\r\n", (double)rpm_analog);
		break;

	case TYPE_MSG_ANALOG_STOP:
		if (g_current_mode == MODE_ANALOG) {
			uart_safe_stop();
			g_is_running = false;
			io_buzzer_start(2, 100, 200);
			// printf("Dung mode analog\r\n");
		}
		break;

	// Run mode digital
	case TYPE_MSG_DIGITAL_START:
		if (g_current_mode != MODE_DIGITAL) {
			// printf("Khong chay dc mode digital\r\n");
			return;
		}

		rpm_digital =
			convert_from_bytes_to_u16(frame_master_msg->data[0], frame_master_msg->data[1]);
		mode_analog_or_digital(rpm_digital);
		g_is_running = true;
		io_buzzer_start(2, 100, 200); // Beep khi start
		// printf("DIGITAL Start: Target RPM = %.2f\r\n", (double)rpm_digital);
		break;

	case TYPE_MSG_DIGITAL_STOP:
		if (g_current_mode == MODE_DIGITAL) {
			uart_safe_stop();
			g_is_running = false;
			io_buzzer_start(2, 100, 200);
			// printf("Dung mode digital\r\n");
		}
		break;

	// Run mode Ramp
	case TYPE_MSG_RAMP_START:
		if (g_current_mode != MODE_RAMP) {
			// printf("Khong chay duoc mode Ramp\r\n");
			return;
		}

		ramp_params_msg.min_rpm =
			convert_from_bytes_to_u16(frame_master_msg->data[0], frame_master_msg->data[1]);
		ramp_params_msg.time_min =
			convert_from_bytes_to_u16(frame_master_msg->data[2], frame_master_msg->data[3]) *
			1000;
		ramp_params_msg.max_rpm =
			convert_from_bytes_to_u16(frame_master_msg->data[4], frame_master_msg->data[5]);
		ramp_params_msg.time_max =
			convert_from_bytes_to_u16(frame_master_msg->data[6], frame_master_msg->data[7]) *
			1000;

		mode_ramp_init(ramp_params_msg);
		g_is_running = true;
		io_buzzer_start(2, 100, 200); // Beep khi start ramp
		// printf("RAMP Start: Min=%.2f | TimeMin=%u ms | Max=%.2f | TimeMax=%u ms\r\n",
		// (double)ramp_params_msg.min_rpm, ramp_params_msg.time_min,
		// (double)ramp_params_msg.max_rpm, ramp_params_msg.time_max);
		break;
	case TYPE_MSG_RAMP_FINISH:
		break;
	case TYPE_MSG_RAMP_STOP:
		if (g_current_mode == MODE_RAMP) {
			mode_ramp_stop();
			g_is_running = false;
			io_buzzer_start(2, 100, 200);
			// printf("Dung mode ramp\r\n");
		}
		break;
	default:
		// printf("Error: Unknown message type 0x%02X\r\n", frame_master_msg->type_msg);
		break;
	}
}

void uart_parser_from_esp(void)
{
	static uint8_t sRxBuffer[UART_RX_BUFFER_SIZE];
	static uint16_t sRxIndex = 0;

	// Giới hạn số frame xử lý mỗi lần để tránh block quá lâu
	u8 frames_processed = 0;
	const uint8_t MAX_FRAMES_PER_CALL = 3;

	while (uart2.available() && frames_processed < MAX_FRAMES_PER_CALL) {
		u8 byte = (u8)uart2.get_char();

		if (sRxIndex >= sizeof(sRxBuffer)) {
			sRxIndex = 0;
			for (uint16_t i = 0; i < UART_RX_BUFFER_SIZE; i++) {
				sRxBuffer[i] = 0;
			}
		}

		sRxBuffer[sRxIndex++] = byte;

		/* Frame hop le */
		if (sRxIndex >= 6) // khi nhan du Start Frame (2 byte) + Type Msg (1 byte) + Length
				   // Data (1 byte) + Data (0 byte) + CRC (2 byte) = 6 byte
		{
			u16 start_frame = convert_from_bytes_to_u16(sRxBuffer[0], sRxBuffer[1]);

			if (start_frame != START_BYTE) {
				sRxIndex = 0;
				continue;
			}

			/* Len Data */
			u8 len_data = sRxBuffer[3];

			if (len_data > 12) {
				// printf("Uart invalid length\r\n");
				sRxIndex = 0;
				continue;
			}

			u16 totalFrameLen = LENGTH_DATA_HEADER + len_data + 2; // 2 byte CRC nua

			if (sRxIndex >= totalFrameLen) {
				if (msg_check_crc(sRxBuffer, totalFrameLen)) {
					frame_master frame;
					msg_decode_frame_master(sRxBuffer, &frame);
					uart_process_master_frame(&frame);
					frames_processed++; // Tăng biến đếm frame
				} else {
					// printf("Error!\r\n");
				}
				sRxIndex = 0;
			}
		}
	}
}

void uart_send_measure_rpm(void)
{
	static uint32_t sLastTime = 0;

	if (get_tick() - sLastTime < UART_MRPM_SEND_INTERVAL_MS) {
		return;
	}

	sLastTime = get_tick();

	if (g_is_running == false || g_current_mode == MODE_IDLE) {
		return;
	}

	float measuredRpm = encoder_get_rpm(UART_MRPM_SEND_INTERVAL_MS);

	uint8_t msgType = '\0';

	switch (g_current_mode) {
	case MODE_IDLE:
		break;
	case MODE_ANALOG:
		msgType = TYPE_MSG_ANALOG_MRPM_UPDATE;
		break;
	case MODE_DIGITAL:
		msgType = TYPE_MSG_DIGITAL_MRPM_UPDATE;
		break;
	case MODE_RAMP:
		msgType = TYPE_MSG_RAMP_MRPM_UPDATE;
		break;
	}

	frame_slave txSlave = {0};
	txSlave.start_frame = START_BYTE;
	txSlave.type_msg = msgType;

	uint8_t *rpmBytes = convert_from_float_to_bytes(measuredRpm);
	txSlave.data[0] = rpmBytes[0];
	txSlave.data[1] = rpmBytes[1];
	txSlave.data[2] = rpmBytes[2];
	txSlave.data[3] = rpmBytes[3];

	uint8_t output[UART_RX_BUFFER_SIZE];
	uint8_t len = msg_create_frame_slave(&txSlave, output);

	if (len > 0) {
		for (uint8_t i = 0; i < len; i++) {
			uart2.send_char(output[i]);
		}
	}
}

void uart_send_ramp_finish(void)
{
	frame_slave txSlave = {0};
	txSlave.start_frame = START_BYTE;
	txSlave.type_msg = TYPE_MSG_RAMP_FINISH;

	uint8_t output[UART_RX_BUFFER_SIZE];
	uint8_t len = msg_create_frame_slave(&txSlave, output);

	if (len > 0) {
		for (u8 i = 0; i < len; i++) {
			uart2.send_char(output[i]);
		}
	}

	// Beep 2 lan khi ramp finish thanh cong
	io_buzzer_start(2, 100, 200);

	// printf("Ramp Finish Success\r\n");
}
