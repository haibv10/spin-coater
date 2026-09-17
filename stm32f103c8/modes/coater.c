#include "coater.h"

coater_control g_coater_ctrl = {COATER_IDLE, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0};

static pid_controller sPidMotor;

/**
 * @name coater_is_idle
 * @brief Kiểm tra spin coater có đang ở trạng thái nghỉ hay không
 * @warning có thể dùng để chặn không cho ramp mới khi ramp cũ chưa kết thúc
 * @return true nếu đang nghỉ, false nếu đang ramp
 */
bool coater_is_idle(void) { return (g_coater_ctrl.state == COATER_IDLE); }

/**
 * @name coater_pid_init
 * @brief Khởi tạo thông số PID cho spin coater
 * @param pid_cfg: thông số PID
 */
void coater_pid_init(coater_pid_cfg pid_cfg)
{
	pid_init(&sPidMotor, pid_cfg.kp, pid_cfg.ki, pid_cfg.kd, pid_cfg.max_output);
}

/**
 * @name coater_ramp_up
 * @param target_rpm: tốc độ đích (RPM)
 * @param steps: số bước ramp
 * @param ramp_time_ms: thời gian ramp tổng (ms)
 * @brief Bắt đầu quá trình ramp up đến tốc độ target_rpm trong ramp_time_ms
 * @note Chỉ bắt đầu ramp khi đang ở trạng thái nghỉ (idle)
 */
void coater_ramp_up(float target_rpm, uint32_t steps, uint32_t ramp_time_ms)
{
	if (g_coater_ctrl.state != COATER_IDLE) {
		return; // nếu đang ramp cũ thì không làm gì cả
	}

	if (steps == 0u || ramp_time_ms == 0u) {
		return; // tránh chia cho 0 return luôn
	}

	float current_rpm = encoder_get_rpm(
		COATER_ENCODER_AVG_SAMPLE); // đọc RPM hiện tại từ encoder với mẫu được define

	g_coater_ctrl.state = COATER_RAMP_UP;
	g_coater_ctrl.target_rpm = target_rpm; // mục tiêu tốc độ RPM mong muốn
	g_coater_ctrl.start_rpm = current_rpm; // tốc độ RPM hiện tại khi bắt đầu ramp
	g_coater_ctrl.current_rpm =
		current_rpm; // setpoint ban đầu = RPM hiện tại, tránh nhảy setpoint đột ngột
	g_coater_ctrl.steps = steps;			  // số bước ramp
	g_coater_ctrl.delay_per_step = ramp_time_ms / steps; // mỗi bước cách nhau bao lâu (ms)
	g_coater_ctrl.last_tick = get_tick();
	g_coater_ctrl.done_steps = 0u;
}

/**
 * @name coater_ramp_down
 * @brief Giống với coater_ramp_up nhưng là giảm tốc
 */
void coater_ramp_down(float target_rpm, uint32_t steps, uint32_t ramp_time_ms)
{
	// Cho phép RampDown ngắt RAMP_UP để dừng khẩn cấp
	// Chỉ không cho phép nếu đang RAMP_DOWN khác
	if (g_coater_ctrl.state == COATER_RAMP_DOWN) {
		return; // Đang ramp down rồi, không cho phép ramp down khác
	}

	if (steps == 0u || ramp_time_ms == 0u) {
		return;
	}

	float current_rpm = encoder_get_rpm(COATER_ENCODER_AVG_SAMPLE);

	g_coater_ctrl.state = COATER_RAMP_DOWN;
	g_coater_ctrl.target_rpm = target_rpm;
	g_coater_ctrl.start_rpm = current_rpm;
	g_coater_ctrl.current_rpm = current_rpm;
	g_coater_ctrl.steps = steps;
	g_coater_ctrl.delay_per_step = ramp_time_ms / steps;
	g_coater_ctrl.last_tick = get_tick();
	g_coater_ctrl.done_steps = 0u;
}

/**
 * @name coater_update_ramp_profile
 * @brief Cập nhật profile ramp dựa trên thời gian đã trôi qua
 * @note Hàm này được gọi bên trong coater_update()
 */
static void coater_update_ramp_profile(void)
{
	if (g_coater_ctrl.state == COATER_IDLE) {
		return; // nếu đang nghỉ thì không ramp
	}

	uint32_t now = get_tick();
	if ((now - g_coater_ctrl.last_tick) < g_coater_ctrl.delay_per_step) {
		return; // chưa đủ thời gian cho 1 step mới thì return luôn
	}

	// thời điểm đã đủ cho 1 step
	g_coater_ctrl.last_tick = now;

	float progress = 0.0f;
	if (g_coater_ctrl.steps > 0u) {
		progress = (float)g_coater_ctrl.done_steps /
			   (float)g_coater_ctrl.steps; // tính tiến độ ramp
	}

	// camp progress trong khoảng 0.0 -> 1.0
	if (progress < 0.0f)
		progress = 0.0f;
	if (progress > 1.0f)
		progress = 1.0f;

	float start = g_coater_ctrl.start_rpm;
	float target = g_coater_ctrl.target_rpm;

	// ramp tuyến tính theo RPM
	float rpmProfile = start + (target - start) * progress;

	// clamp theo chiều ramp
	if (g_coater_ctrl.state == COATER_RAMP_UP && rpmProfile > target) {
		rpmProfile = target;
	}
	if (g_coater_ctrl.state == COATER_RAMP_DOWN && rpmProfile < target) {
		rpmProfile = target;
	}

	g_coater_ctrl.current_rpm = rpmProfile; // setpoint RPM PID phải bám tại thời điểm này

	g_coater_ctrl.done_steps++;
	if (g_coater_ctrl.done_steps >= g_coater_ctrl.steps) {
		g_coater_ctrl.current_rpm = g_coater_ctrl.target_rpm;

		if (g_coater_ctrl.state == COATER_RAMP_DOWN &&
		    g_coater_ctrl.current_rpm < 0.1f) {
			g_coater_ctrl.current_rpm = 0.0f;
		}

		g_coater_ctrl.state =
			COATER_IDLE; // chuyển về trạng thái nghỉ khi hoàn thành ramp
	}
}

/**
 * @name coater_update
 * @brief Cập nhật điều khiển PID cho spin coater
 * @param target_rpm: tốc độ mục tiêu (RPM) khi không ramp
 * @note Hàm này cần được gọi liên tục trong hàm main app_loop
 */
void coater_update(float target_rpm)
{
	/**
	 * @name coater_update_ramp_profile
	 * @note nếu đang ramp, hàm này có thể thay đổi g_coater_ctrl.currentRpm
	 * @note nếu nghỉ, thì không làm gì cả
	 */
	coater_update_ramp_profile();

	/**
	 * @name Chọn setpoint để PID bám theo
	 * @note Nếu đang ramp thì bám theo g_coater_ctrl.currentRpm
	 * @note Nếu nghỉ thì bám theo target_rpm truyền vào coater_update()
	 */
	float setpoint;

	if (g_coater_ctrl.state != COATER_IDLE) {
		setpoint = g_coater_ctrl.current_rpm;
	} else {
		setpoint = target_rpm;
		g_coater_ctrl.target_rpm = target_rpm; // lưu lại nếu cần
	}

	// nếu setpoint gần 0 tắt motor
	if (setpoint <= 0.1f) {
		esc_set_duty(ESC_MIN_PWM);
		g_coater_ctrl.current_rpm = 0.0f;
		// Reset PID để tránh integral windup
		sPidMotor.I = 0.0f;
		sPidMotor.D = 0.0f;
		sPidMotor.error = 0.0f;
		sPidMotor.previous_time = 0u;
		return;
	}

	float current_rpm = encoder_get_rpm(COATER_ENCODER_AVG_SAMPLE); // đọc RPM hiện tại từ encoder với lấy trung bình 50 mẫu

	uint16_t baseDuty = esc_rpm_to_duty_logistic(setpoint); // tính duty ước lượng trước sao cho nếu không có
						 // PID thì motor cx gần đạt setpoint

	/**
	 * @name pid_update
	 * @brief Cập nhật PID để tính toán duty cuối cùng
	 * @param pid: con trỏ đến cấu trúc PID
	 * @param setpoint: RPM mục tiêu
	 * @param current_rpm: RPM hiện tại
	 */
	float pidOutput = pid_update(&sPidMotor, setpoint, current_rpm, (float)baseDuty);

	esc_set_duty((uint16_t)pidOutput); // gửi duty cuối cùng đến ESC
}
