#ifndef __PID_H__
#define __PID_H__

#include "stm32f10x.h"

typedef struct {
	float kp;
	float ki;
	float kd;

	float error;	       // sai so thoi diem hien tai
	float prev_process_value; // gia tri xu ly thoi diem truoc
	float P;	       // thanh phan P
	float I;	       // thanh phan I
	float D;	       // thanh phan D
	float output;	       // ket qua dau ra cuoi cung sau khi tinh toan PID
	float max_output;       // gioi han dau ra cuc dai
	float deadband;	       // vung chet - khong tinh P va I khi |error| < deadband

	uint32_t current_time;  // thoi diem hien tai
	uint32_t previous_time; // thoi diem truoc do
	uint32_t delta_time;    // khoang thoi gian giua 2 lan tinh toan lien tiep
} pid_controller;

void pid_init(pid_controller *pid, float kp, float ki, float kd, float max_output);
void pid_set_deadband(pid_controller *pid, float deadband);
float pid_update(pid_controller *pid, float set_point, float process_value, float ubias);
void pid_reset(pid_controller *pid);

#endif /* __PID_H__ */
