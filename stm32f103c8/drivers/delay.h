#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

/*** SysTick Timer ***/
#define STK_BASE 0xE000E010U
#define STK_CTRL (*(volatile uint32_t *)(STK_BASE + 0x00))
#define STK_LOAD (*(volatile uint32_t *)(STK_BASE + 0x04))
#define STK_VAL (*(volatile uint32_t *)(STK_BASE + 0x08))
#define STK_CALIB (*(volatile uint32_t *)(STK_BASE + 0x0C))

/*** Data Watchpoint Trigger ***/
#define DWT_DEMCR (*(volatile uint32_t *)0xE000EDFC)
#define DWT_CTRL (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)

extern volatile uint32_t tick;

typedef struct {
	void (*init)(void);
} delay_dev;

extern delay_dev delay_drv;

void delay_init(void);
void delay_ms(uint32_t ms);
uint32_t get_tick(void);
void delay_us(uint32_t us);
void SysTick_Handler(void);

#endif /* __DELAY_H */
