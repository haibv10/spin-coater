#include "pid.h"
#include "delay.h"

void pid_init(pid_controller *pid, float kp, float ki, float kd, float max_output)
{
	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;

	pid->error = 0.0f;
	pid->prev_process_value = 0.0f;
	pid->P = 0.0f;
	pid->I = 0.0f;
	pid->D = 0.0f;

	pid->output = 0.0f;
	pid->max_output = (max_output > 0.0f) ? max_output : 0.0f;
	pid->deadband = 0.0f; // mac dinh khong co deadband

	pid->current_time = 0u;
	pid->previous_time = 0u;
	pid->delta_time = 0u;
}

float pid_update(pid_controller *pid, float set_point, float process_value, float ubias)
{
	pid->current_time = get_tick();

	if (pid->previous_time == 0u) {
		pid->previous_time = pid->current_time;
		pid->prev_process_value = process_value;

		float tempUbias = ubias;
		if (pid->max_output > 0.0f) {
			if (tempUbias > pid->max_output) {
				tempUbias = pid->max_output;
			}
			if (tempUbias < -pid->max_output) {
				tempUbias = -pid->max_output;
			}
		}
		pid->output = tempUbias;
		return pid->output;
	} else if (pid->current_time - pid->previous_time >= 1) {
		pid->delta_time = pid->current_time - pid->previous_time;
		pid->previous_time = pid->current_time;
		// Giới hạn delta_time tối đa 10ms để tránh D term quá lớn
		if (pid->delta_time > 10) {
			pid->delta_time = 10;
		}
	} else {
		if (pid->current_time - pid->previous_time < 1) {
			return pid->output;
		}
	}

	// Tính toán các thành phần PID
	pid->error = set_point - process_value;

	// Áp dụng deadband: nếu error nhỏ thì không điều chỉnh P và I
	if (pid->deadband > 0.0f && pid->error > -pid->deadband && pid->error < pid->deadband) {
		pid->P = 0.0f;
		// Không tích lũy I khi trong vùng deadband
	} else {
		pid->P = pid->kp * pid->error;
		pid->I += pid->ki * pid->error * ((float)pid->delta_time / 1000.0f);
	}

	if (pid->previous_time != 0u && pid->delta_time >= 1u) {
		pid->D = -pid->kd * (process_value - pid->prev_process_value) * 1000 /
			 (float)pid->delta_time;
	}
	pid->prev_process_value = process_value;

	// Giới hạn phần điều chỉnh PID (P+I+D), không phải tổng output
	float pidCorrection = pid->P + pid->I + pid->D;

	if (pid->max_output > 0.0f) {
		if (pidCorrection > pid->max_output) {
			// Giảm I để không vượt quá giới hạn
			pid->I = pid->max_output - pid->P - pid->D;
			pidCorrection = pid->max_output;
		} else if (pidCorrection < -pid->max_output) {
			// Giảm I để không vượt quá giới hạn
			pid->I = -pid->max_output - pid->P - pid->D;
			pidCorrection = -pid->max_output;
		}
	}

	pid->output = ubias + pidCorrection;
	return pid->output;
}

void pid_reset(pid_controller *pid)
{
	pid->P = 0.0f;
	pid->I = 0.0f;
	pid->D = 0.0f;
	pid->error = 0.0f;
	pid->prev_process_value = 0.0f;
	pid->output = 0.0f;

	pid->current_time = 0u;
	pid->previous_time = 0u;
	pid->delta_time = 0u;
}

void pid_set_deadband(pid_controller *pid, float deadband)
{
	pid->deadband = (deadband >= 0.0f) ? deadband : 0.0f;
}
