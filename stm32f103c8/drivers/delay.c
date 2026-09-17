#include "delay.h"

delay_dev delay_drv;

volatile uint32_t tick = 0;

void delay_init(void)
{
	/*** SysTick Timer ***/
	STK_CTRL = 0;
	STK_LOAD = (SystemCoreClock / 1000) - 1;
	STK_VAL = 0;
	STK_CTRL |= ((1 << 0) | (1 << 1) | (1 << 2));

	/*** Data Watchpoint Trigger ***/
	DWT_DEMCR |= (1 << 24); // TRCENA
	DWT_CYCCNT = 0;
	DWT_CTRL |= 1; // CYCCNTENA

	__asm volatile("dsb");
	__asm volatile("isb");
}

inline uint32_t get_tick(void) { return tick; }

void delay_ms(uint32_t ms)
{
	uint32_t tick_start = get_tick();
	while ((get_tick() - tick_start) < ms);
}

void delay_us(uint32_t us)
{
	uint32_t temp_tick = us * (SystemCoreClock / 1000000u);
	volatile uint32_t start = DWT_CYCCNT;
	while ((DWT_CYCCNT - start) < temp_tick);
}

void SysTick_Handler(void) { tick++; }

void delay_autoinit(void) __attribute__((constructor));

void delay_autoinit(void) { delay_drv.init = &delay_init; }
