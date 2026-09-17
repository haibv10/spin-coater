#include "io_driver.h"

BuzzerFSM_t buzzer;

void io_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.GPIO_Pin = PIN_RELAY_1 | PIN_RELAY_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(PORT_RELAY, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PIN_BUZZER;
	GPIO_Init(PORT_BUZZER, &GPIO_InitStructure);

	RELAY_ON;
	// GPIO_ResetBits(PORT_RELAY, PINMASK_RELAY_ALL); // Tat relay
	BUZZER_OFF(); // Tat buzzer

	buzzer.state = BUZZER_IDLE;
	buzzer.is_busy = false;
	buzzer.times = 0;
	buzzer.duration_ms = 0;
	buzzer.interval_ms = 0;
	buzzer.counter_ms = 0;
}

void io_set_relay(u16 gpio_pin, BitAction bit_val)
{
	if (bit_val == Bit_RESET) {
		GPIOB->BSRR = gpio_pin;
	} else {
		GPIOB->BRR = gpio_pin;
	}
}

void io_set_buzzer(u16 gpio_pin, BitAction bit_val)
{
	if (bit_val == Bit_RESET) {
		GPIOA->BSRR = gpio_pin;
	} else {
		GPIOA->BRR = gpio_pin;
	}
}

void io_buzzer_start(u8 times, u16 duration_ms, u16 interval_ms)
{
	if (buzzer.is_busy == true) {
		return;
	}

	buzzer.times = times;
	buzzer.duration_ms = duration_ms;
	buzzer.interval_ms = interval_ms;
	buzzer.counter_ms = get_tick();
	buzzer.is_busy = true;
	buzzer.state = BUZZER_ON;

	BUZZER_ON(); // on Buzzer
}
void io_buzzer_update(void)
{
	if (buzzer.is_busy == false) {
		return;
	}

	switch (buzzer.state) {
	case BUZZER_IDLE: {
		buzzer.is_busy = false;
		BUZZER_OFF(); // OFF
		break;
	}
	case BUZZER_ON: {
		if (get_tick() - buzzer.counter_ms >= buzzer.duration_ms) {
			BUZZER_ON(); // ON
			buzzer.counter_ms = get_tick();
			buzzer.state = BUZZER_OFF;
			if (buzzer.times > 0)
				buzzer.times--;
		}
		break;
	}
	case BUZZER_OFF: {
		if (get_tick() - buzzer.counter_ms >= buzzer.interval_ms) {
			if (buzzer.times == 0) {
				buzzer.state = BUZZER_IDLE;
				buzzer.is_busy = false;
				BUZZER_OFF();
			} else {
				BUZZER_OFF();
				buzzer.counter_ms = get_tick();
				buzzer.state = BUZZER_ON;
			}
		}
		break;
	}
	}
}

bool io_buzzer_is_busy(void) { return buzzer.is_busy; }
