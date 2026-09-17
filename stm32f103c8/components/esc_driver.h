/**
 * @file esc_driver.h
 * @brief Header file for esc_driver.c
 * @author haihbv
 * @date December 4, 2025
 */
#ifndef ESC_DRIVER_H_
#define ESC_DRIVER_H_

#include "main.h"

#define ESC_MIN_PWM 4500u
#define ESC_MAX_PWM 6000u

void esc_init(void);
void esc_arm(uint32_t timeout_ms);
void esc_set_duty(uint16_t duty_ticks);
uint16_t esc_get_duty(void);
float esc_duty_to_rpm_logistic(uint16_t duty_ticks);
uint16_t esc_rpm_to_duty_logistic(float rpm);

/**
 * @name Linear Conversion Functions
 * @brief These functions are for reference only and are not used in control
 */
float esc_duty_to_rpm(uint16_t duty_ticks);
uint16_t esc_rpm_to_duty(float rpm);

#endif /* ESC_DRIVER_H_ */
