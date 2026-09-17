#include "mode.h"

static ramp_state sRampState = RAMP_IDLE; // state hiện tại của Ramp mode
static uint32_t sHoldStart = 0;		   // thời điểm bắt đầu giữ ở min/max RPM
static ramp_params sRampParams = {0};	   // tham số ramp hiện tại

#define MODE_ENCODER_AVG_SAMPLES (50U) // đọc encoder trung bình 50 mẫu
#define RPM_MIN_LIMIT (0.0f)
#define RPM_MAX_LIMIT (6000.0f)
#define MODE_RPM_DEADZONE (15.0f) // chênh lệch 15 RPM xem như đủ gần

/**
 * @name clamp_rpm
 * @brief Giới hạn RPM trong khoảng cho phép
 * @param rpm Giá trị RPM cần giới hạn
 */
static inline float clamp_rpm(float rpm)
{
	if (rpm < RPM_MIN_LIMIT)
		rpm = RPM_MIN_LIMIT;
	if (rpm > RPM_MAX_LIMIT)
		rpm = RPM_MAX_LIMIT;
	return rpm;
}

/**
 * @name mode_do_stop
 * @brief Dừng coater và gửi lệnh FINISH nếu cần
 * @param send_finish true để gửi lệnh FINISH, false không gửi
 * @note Hàm này chỉ được gọi khi coater đã dừng hoàn toàn
 * @note Gửi xong thì đặt state về RAMP_IDLE
 */
static void mode_do_stop(bool send_finish)
{
	g_is_running = false;
	esc_set_duty(ESC_MIN_PWM);
	g_coater_ctrl.current_rpm = 0.0f;

	if (send_finish) {
		uart_send_ramp_finish();
	}
	sRampState = RAMP_IDLE;
}

/**
 * @name mode_analog_or_digital
 * @brief Chế độ điều khiển Analog hoặc Digital
 * @param target_rpm Giá trị RPM mục tiêu
 */
void mode_analog_or_digital(float target_rpm)
{
	if (sRampState != RAMP_IDLE) {
		return; // tránh xung đột với Ramp mode
	}

	target_rpm = clamp_rpm(target_rpm);
	g_coater_ctrl.target_rpm = target_rpm;

	float current_rpm = encoder_get_rpm(MODE_ENCODER_AVG_SAMPLES);

	if (!coater_is_idle()) {
		return; // nếu spin coater đang chạy thì không can thiệp thêm
	}

	float diff = current_rpm - target_rpm; // tính chênh lệch giữa RPM hiện tại và mục tiêu
	if (diff < -MODE_RPM_DEADZONE)	     // if RPM hiện tại nhỏ hơn 15 RPM so với mục tiêu
	{
		// thì ramp up lên target_rpm
		coater_ramp_up(target_rpm, RAMP_DEFAULT_RAMP_UP_STEPS, RAMP_DEFAULT_RAMP_UP_TIME_MS);
	} else if (diff > MODE_RPM_DEADZONE) // if RPM hiện tại lớn hơn 15 RPM so với mục tiêu
	{
		// thì ramp down xuống target_rpm
		coater_ramp_down(target_rpm, RAMP_DEFAULT_RAMP_DOWN_STEPS,
				RAMP_DEFAULT_RAMP_DOWN_TIME_MS);
	}
}

/**
 * @name mode_ramp_init
 * @brief Khởi tạo chế độ Ramp với tham số đã cho
 * @param params Tham số Ramp
 */
void mode_ramp_init(ramp_params params)
{
	if (sRampState != RAMP_IDLE) {
		return; // đang có chế độ Ramp khác chạy thì không làm gì cả
		// ví dụ: Mode Analog đang chạy thì chỉ chạy Mode Analog thôi khi vào Mode Ramp thì
		// bị return ngay luôn :3
	}

	// Giới hạn tham số trong khoảng cho phép
	params.min_rpm = clamp_rpm(params.min_rpm);
	params.max_rpm = clamp_rpm(params.max_rpm);

	// Đảm bảo min_rpm không lớn hơn max_rpm
	if (params.min_rpm > params.max_rpm) {
		float temp = params.min_rpm;
		params.min_rpm = params.max_rpm;
		params.max_rpm = temp;
	}

	// Tránh thời gian giữ bằng 0
	if (params.time_min == 0U) {
		params.time_min = 1U;
	}
	if (params.time_max == 0U) {
		params.time_max = 1U;
	}

	sRampParams = params;
	sRampState = RAMP_TO_MIN;
	// Gọi coater_ramp_up để ramp từ RPM hiện tại đến min_rpm
	coater_ramp_up(sRampParams.min_rpm, RAMP_DEFAULT_RAMP_UP_STEPS, RAMP_DEFAULT_RAMP_UP_TIME_MS);
}

/**
 * @name mode_ramp_update
 * @brief Cập nhật state machine của chế độ Ramp
 * @note Hàm này cần được gọi thường xuyên trong main app_loop
 * @note Xử lý các trạng thái của Ramp mode
 */
void mode_ramp_update(void)
{
	switch (sRampState) {
	case RAMP_IDLE:
		break;
	case RAMP_TO_MIN: // ramp xong chuyển đến RAMP_HOLD_MIN, ghi lại thời điểm bắt đầu giữ
		if (coater_is_idle()) {
			sRampState = RAMP_HOLD_MIN;
			sHoldStart = get_tick();
		}
		break;

	case RAMP_HOLD_MIN: // giữ ở minRpm trong time_min ms, sau đó chuyển sang RAMP_TO_MAX

		if (get_tick() - sHoldStart >= sRampParams.time_min) {
			coater_ramp_up(sRampParams.max_rpm, RAMP_DEFAULT_RAMP_UP_STEPS,
				      RAMP_DEFAULT_RAMP_UP_TIME_MS);
			sRampState = RAMP_TO_MAX;
		}
		break;

	case RAMP_TO_MAX: // ramp xong chuyển đến RAMP_HOLD_MAX, ghi lại thời điểm bắt đầu giữ
		if (coater_is_idle()) {
			sRampState = RAMP_HOLD_MAX;
			sHoldStart = get_tick();
		}
		break;

	case RAMP_HOLD_MAX: // giữ ở maxRpm trong time_max ms, sau đó ramp về 0 và chuyển sang
			    // RAMP_FINISH
		if (get_tick() - sHoldStart >= sRampParams.time_max) {
			coater_ramp_down(0.0f, RAMP_DEFAULT_RAMP_DOWN_STEPS,
					RAMP_DEFAULT_RAMP_DOWN_TIME_MS);
			sRampState = RAMP_FINISH;
		}
		break;

	case RAMP_FINISH:
		if (coater_is_idle()) {
			mode_do_stop(true); // true để gửi lệnh FINISH
		}
		break;

	case RAMP_STOP:
		if (coater_is_idle()) {
			mode_do_stop(false /*true*/); // false để không gửi lệnh FINISH
		}
		break;
	}
}

/**
 * @name mode_ramp_stop
 * @brief Dừng chế độ Ramp hiện tại
 * @note Hàm này sẽ ramp coater về 0 RPM và dừng
 */
void mode_ramp_stop(void)
{
	if (sRampState != RAMP_IDLE) {
		coater_ramp_down(0.0f, RAMP_DEFAULT_RAMP_DOWN_STEPS, RAMP_DEFAULT_RAMP_DOWN_TIME_MS);
		sRampState = RAMP_STOP;
	}
}
