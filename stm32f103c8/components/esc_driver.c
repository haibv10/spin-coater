#include "esc_driver.h"

/**
 * @brief Giới hạn u16_x trong khoảng [min, max]
 * @param x: giá trị cần giới hạn
 * @param min: giá trị nhỏ nhất
 * @param max: giá trị lớn nhất
 */
static inline uint16_t esc_clamp_u16(uint16_t x, uint16_t min, uint16_t max)
{
	if (x < min)
		return min;
	if (x > max)
		return max;
	return x;
}

/**
 * @brief Giới hạn f_x trong khoảng [min, max]
 * @param x: giá trị cần giới hạn
 * @param min: giá trị nhỏ nhất
 * @param max: giá trị lớn nhất
 */
static inline float esc_clamp_f(float x, float min, float max)
{
	if (x < min)
		return min;
	if (x > max)
		return max;
	return x;
}

/**
 * @name DutyToRpmLogistic Constants
 */
#define L_ESC 5777.0f  // RPM max của ESC
#define K_ESC 0.00945f // hế số dốc
#define D0_ESC 4757.0f // điểm dịch chuyển dọc trục x

/**
 * @name esc_duty_to_rpm_logistic
 * @brief Chuyển đổi duty (tick timer) sang RPM theo đặc tuyến logistic (phi tuyến)
 * @param duty_ticks: số tick trong một chu kỳ PWM
 * @return float: RPM
 */
float esc_duty_to_rpm_logistic(uint16_t duty_ticks)
{
	if (duty_ticks <= ESC_MIN_PWM) {
		return 0.0f;
	}
	if (duty_ticks >= ESC_MAX_PWM) {
		return ESC_MAX_PWM;
	}

	float x = (float)duty_ticks;
	float rpm = L_ESC / (1.0f + expf(-K_ESC * (x - D0_ESC)));

	rpm = esc_clamp_f(rpm, 0.0f, 6000.0f);
	return rpm;
}

/**
 * @name RpmToDutyLogistic Constants
 */
#define ESC_SAT_TK 6000u // ~1.800 ms

/**
 * @name esc_rpm_to_duty_logistic
 * @brief Chuyển đổi RPM sang duty (tick timer) theo đặc tuyến logistic (phi tuyến)
 * @param rpm: tốc độ quay (RPM)
 * @return uint16_t: số tick trong một chu kỳ PWM
 */
uint16_t esc_rpm_to_duty_logistic(float rpm)
{
	if (rpm <= 0.0f) {
		return ESC_MIN_PWM;
	}
	if (rpm >= 6000.0f) {
		return ESC_SAT_TK;
	}

	float ratio = (L_ESC / rpm) - 1.0f;
	if (ratio < 1e-6f) {
		ratio = 1e-6f; /* tránh log(0) */
	}
	float duty = D0_ESC - (logf(ratio) / K_ESC);

	duty = esc_clamp_f(duty, 4500.0f, ESC_SAT_TK);
	return (uint16_t)(duty + 0.5f);
}

/**
 * @name esc_init
 * @brief Cấu hình PWM cho ESC sử dụng Timer1
 */
void esc_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.GPIO_Pin = PIN_ESC;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(PORT_ESC, &GPIO_InitStructure);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct = {0};
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = 60000 - 1;
	TIM_TimeBaseInitStruct.TIM_Prescaler = 24 - 1;
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStruct);

	TIM_OCInitTypeDef TIM_OCInitStruct = {0};
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStruct.TIM_Pulse = 4500;
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;

	TIM_OC1Init(TIM1, &TIM_OCInitStruct);
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

	TIM_GenerateEvent(TIM1, TIM_EventSource_Update);
	TIM_ARRPreloadConfig(TIM1, ENABLE);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
	TIM_Cmd(TIM1, ENABLE);
}

/**
 * Arm Constants
 */
#define ESC_KICK_FRICTION 4630u
#define ESC_DEADBAND_TK 4600u

/**
 * @name esc_arm
 * @brief Hàm này dùng để calib cho ESC, thoát khỏi vùng dead ban đầu
 * @param timeout_ms: thời gian arming tính bằng ms
 */
void esc_arm(uint32_t timeout_ms)
{
	esc_set_duty(ESC_MIN_PWM);
	delay_ms(timeout_ms);

	esc_set_duty(ESC_KICK_FRICTION); // kick để vượt qua ma sát ban đầu
	delay_ms(100);
	esc_set_duty(ESC_MIN_PWM);
}

/**
 * @name esc_get_duty
 * @brief Lấy giá trị duty hiện tại của ESC
 * @return uint16_t: Giá trị duty hiện tại
 */
uint16_t esc_get_duty(void) { return TIM_GetCapture1(TIM1); }

/**
 * @name esc_set_duty
 * @brief Thiết lập giá trị duty (độ rộng xung) cho ESC
 * @param duty_ticks: số tick trong một chu kỳ PWM
 */
void esc_set_duty(uint16_t duty_ticks)
{
	duty_ticks = esc_clamp_u16(duty_ticks, ESC_MIN_PWM, ESC_MAX_PWM);
	TIM_SetCompare1(TIM1, duty_ticks);
}

/**
 * @brief Chuyển đổi tick timer sang RPM theo tuyến tính
 * @warning Hàm này chỉ mang tính chất tham khảo, không sử dụng trong điều khiển
 */
// #define ESC_LINEAR_MAX_RPM 6000.0f

// float esc_duty_to_rpm(uint16_t duty_ticks)
// {
//   duty_ticks = esc_clamp_u16(duty_ticks, ESC_MIN_PWM, ESC_MAX_PWM);
//   float duty = (float)duty_ticks;
//   float slope = ESC_LINEAR_MAX_RPM / (float)(ESC_MAX_PWM - ESC_MIN_PWM);
//   float offset = -slope * (float)ESC_MIN_PWM;
//   return slope * duty + offset;
// }

// uint16_t esc_rpm_to_duty(float rpm)
// {
//   rpm = esc_clamp_f(rpm, 0.0f, ESC_LINEAR_MAX_RPM);

//   float slope = ESC_LINEAR_MAX_RPM / (float)(ESC_MAX_PWM - ESC_MIN_PWM);
//   float duty = (rpm / slope) + (float)ESC_MIN_PWM;
//   return (uint16_t)(duty + 0.5f);
// }
