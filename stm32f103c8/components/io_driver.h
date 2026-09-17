#ifndef IO_DRIVER_H_
#define IO_DRIVER_H_

#include "main.h"

#define BUZZER_ON() GPIO_SetBits(PORT_BUZZER, PIN_BUZZER)
#define BUZZER_OFF() GPIO_ResetBits(PORT_BUZZER, PIN_BUZZER)
#define RELAY_ON GPIO_SetBits(PORT_RELAY, PINMASK_RELAY_ALL)
#define RELAY_OFF GPIO_ResetBits(PORT_RELAY, PINMASK_RELAY_ALL)

#define RELAY_UV_LED_ON GPIO_SetBits(PORT_RELAY, PIN_RELAY_1)
#define RELAY_UV_LED_OFF GPIO_ResetBits(PORT_RELAY, PIN_RELAY_1)

#define RELAY_VACCUMP_PUMP_ON GPIO_SetBits(PORT_RELAY, PIN_RELAY_2)
#define RELAY_VACCUMP_PUMP_OFF GPIO_ResetBits(PORT_RELAY, PIN_RELAY_2)

typedef enum { BUZZER_IDLE = 0x00, BUZZER_ON = 0x01, BUZZER_OFF } BuzzerState_t;

typedef struct {
	BuzzerState_t state;
	bool is_busy;
	uint8_t times;	     // so lan beep con lai
	uint16_t duration_ms; // thoi gian ON moi beep
	uint16_t interval_ms; // thoi gian nghi giua 2 beep
	uint32_t counter_ms;  // moc thoi gian
} __attribute__((packed)) BuzzerFSM_t;

extern BuzzerFSM_t buzzer;

void io_init(void);
void io_set_relay(uint16_t gpio_pin, BitAction bit_val);
void io_set_buzzer(uint16_t gpio_pin, BitAction bit_val);

void io_buzzer_start(uint8_t times, uint16_t duration_ms, uint16_t interval_ms);
void io_buzzer_update(void);
bool io_buzzer_is_busy(void);

#endif /* IO_DRIVER_H_ */
