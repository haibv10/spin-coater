
/**
 * @file pinmap.h
 * @brief Defination of pin mappings for Spin Coater project.
 * @author haihbv
 * @date December 2025
 */

#ifndef __PINMAP_H__
#define __PINMAP_H__

/*******************************************************
 * RELAY
 ********************************************************/
#define PIN_RELAY_1 GPIO_Pin_0
#define PIN_RELAY_2 GPIO_Pin_1
#define PINMASK_RELAY_ALL (PIN_RELAY_1 | PIN_RELAY_2)
#define PORT_RELAY GPIOB

/*******************************************************
 * BUZZER
 ********************************************************/
#define PIN_BUZZER GPIO_Pin_7
#define PORT_BUZZER GPIOA

/*******************************************************
 * ESC
 ********************************************************/
#define PIN_ESC GPIO_Pin_8
#define PORT_ESC GPIOA

/*******************************************************
 * Encoder
 ********************************************************/
#define PIN_ENCODER_A GPIO_Pin_1
#define PIN_ENCODER_B GPIO_Pin_0
#define PORT_ENCODER GPIOA

#endif /* __PINMAP_H__ */
