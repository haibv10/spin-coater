/**
 * @file main.h
 * @brief Header for project Spin Coater.
 * @author haihbv
 * @date December 2025
 */
#ifndef __PROTOCOL_UART_H__
#define __PROTOCOL_UART_H__

#include "main.h"

typedef enum { MODE_IDLE = 0, MODE_ANALOG, MODE_DIGITAL, MODE_RAMP } coater_mode;

extern coater_mode g_current_mode;
extern bool g_is_running; // Flag để kiểm soát việc gửi MRPM

void uart_process_master_frame(frame_master *frame_master_msg);
void uart_parser_from_esp(void);
void uart_send_measure_rpm(void);
void uart_send_ramp_finish(void);

#endif /* __PROTOCOL_UART_H__ */
