/**
 * @file main.h
 * @brief Header for project Spin Coater.
 * @author haihbv
 * @date December 2025
 */
#ifndef __MODE_H__
#define __MODE_H__

#include "main.h"

/**
 * Default ramp parameters
 * RAMP_UP: up 100 steps in 5000 ms
 * RAMP_DOWN: down 120 steps in 6000 ms
 */
#define RAMP_DEFAULT_RAMP_UP_STEPS (100U)
#define RAMP_DEFAULT_RAMP_UP_TIME_MS (5000U)
#define RAMP_DEFAULT_RAMP_DOWN_STEPS (120U)
#define RAMP_DEFAULT_RAMP_DOWN_TIME_MS (6000U)

typedef struct {
	float min_rpm; // RPM thap nhat trong profile
	float max_rpm; // RPM cao nhat trong profile
	u32 time_min;  // thời gian giữ ở min_rpm ms
	u32 time_max;  // thời gian giữ ở max_rpm ms
} ramp_params;

typedef enum {
	RAMP_IDLE = 0, // không chạy profile ramp
	RAMP_TO_MIN,   // đang ramp tới min_rpm
	RAMP_HOLD_MIN, // đã ở min_rpm, giữ trong time_min
	RAMP_TO_MAX,   // ramp từ min_rpm lên max_rpm
	RAMP_HOLD_MAX, // đã ở max_rpm, giữ trong time_max
	RAMP_FINISH,   // đã ramp về 0, chờ ramp xong để gửi FINISH
	RAMP_STOP      // lệnh stop giữa chừng, ramp về 0 và dừng
} ramp_state;

void mode_analog_or_digital(float target_rpm);
void mode_ramp_init(ramp_params params);
void mode_ramp_update(void);
void mode_ramp_stop(void);

#endif /* __MODE_H__ */
