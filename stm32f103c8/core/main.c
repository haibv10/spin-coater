#include "main.h"

static void coater_setup_pid_init(void);

void app_setup()
{
	SystemInit();
	delay_drv.init();

	io_init();
	RELAY_OFF;

	esc_init();
	esc_arm(1500);

	encoder_init();

	coater_setup_pid_init();

	uart1.init(115200);
	uart2.init(115200);

	delay_ms(2000);

	/* Beep bao hieu he thong da san sang */
	io_buzzer_start(2, 100, 200);
}

void app_loop()
{
	/* Nhan lenh tu ESP32 */
	uart_parser_from_esp();

	/* Cap nhat state machine cua Ramp mode */
	if (g_current_mode == MODE_RAMP)
		mode_ramp_update();

	/* Cap nhat PID controller */
	coater_update(g_coater_ctrl.target_rpm);

	/* Cap nhat buzzer */
	io_buzzer_update();

	/* Gui RPM do duoc ve ESP32 */
	uart_send_measure_rpm();
}

int main()
{
	app_setup();

	while (1)
		app_loop();
}

static void coater_setup_pid_init(void)
{
	coater_pid_cfg motor_pid = {
		.kp = 0.1f,
		.ki = 1.0f,
		.kd = 0.0f,
		.max_output = 1500.0f,
	};

	coater_pid_init(motor_pid);
}
