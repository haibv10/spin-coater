/**
 * @file encoder.h
 * @brief Header file for encoder.c
 * @author haihbv
 * @date December 4, 2025
 */
#ifndef ENCODER_H_
#define ENCODER_H_

#include "main.h"

void encoder_init(void);
uint16_t encoder_get_count(void);
int32_t encoder_get_total_count(void);
uint16_t encoder_get_pulse_count(uint32_t sample_time_ms);
float encoder_get_rpm(uint32_t sample_time_ms);

#endif /* ENCODER_H_ */
