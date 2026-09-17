/**
 * @file main.h
 * @brief Header for project Spin Coater.
 * @author haihbv
 * @date December 2025
 */
#ifndef __MAIN_H__
#define __MAIN_H__

/*===============================================================
 * INCLUDES
 *=============================================================*/
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "stm32f10x.h"

#include "pinmap.h"

#include "delay.h"
#include "uart.h"
#include "io_driver.h"

#include "esc_driver.h"
#include "encoder.h"

#include "pid.h"
#include "kalmanfilter.h"

#include "coater.h"
#include "mode.h"

#include "message.h"
#include "convert.h"
#include "protocol_uart.h"

void app_setup(void);
void app_loop(void);

#endif /* __MAIN_H__ */
