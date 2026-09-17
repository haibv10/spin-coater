/**
 * @file main.h
 * @brief Header for project Spin Coater.
 * @author haihbv
 * @date December 2025
 */
#ifndef COATER_H_
#define COATER_H_

#include "main.h"

#define COATER_ENCODER_AVG_SAMPLE (50u)

typedef enum {
	COATER_IDLE = 0, // không ramp, đang đứng yên hoặc giữ tốc độ ổn định
	COATER_RAMP_UP,	 // đang trong quá trình tăng tốc
	COATER_RAMP_DOWN // đang trong quá trình giảm tốc
} coater_state;

typedef struct {
	coater_state state;
	float target_rpm;
	float start_rpm;
	float current_rpm;
	uint32_t steps;	       // tổng số bước Ramp
	uint32_t delay_per_step; // Thời gian chờ giữa các bước (ms)
	uint32_t last_tick;     // Thời điểm cập nhật lần cuối (ms)
	uint32_t done_steps;    // Số bước hoàn thành
} __attribute__((packed)) coater_control;

typedef struct {
	float kp;
	float ki;
	float kd;
	float max_output; // giá trị tối đa của output
} coater_pid_cfg;

extern coater_control g_coater_ctrl; // global

bool coater_is_idle(void);
void coater_update(float target_rpm);
void coater_ramp_up(float target_rpm, uint32_t steps, uint32_t ramp_time_ms);
void coater_ramp_down(float target_rpm, uint32_t steps, uint32_t ramp_time_ms);
void coater_pid_init(coater_pid_cfg pid_cfg);

#endif /* COATER_H_ */
