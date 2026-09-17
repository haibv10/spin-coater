#include "uart.h"
#include "delay.h"
#include <stdio.h>

#define DEBUG_USART uart1

usart_driver uart1 = {0};
usart_driver uart2 = {0};
usart_driver uart3 = {0};

/** @brief Kích thước ring-buffer RX (phải là lũy thừa của 2). */
#define RX_BUF_SIZE 512

#if (RX_BUF_SIZE & (RX_BUF_SIZE - 1)) != 0
#error "RX_BUF_SIZE must be a power of two"
#endif

/** @brief Mặt nạ chỉ số cho ring-buffer (tối ưu modulo). */
#define RX_IDX_MASK (RX_BUF_SIZE - 1)

/**
 * @brief Cấu trúc ring-buffer đơn giản cho dữ liệu RX.
 * @note head: vị trí ghi tiếp theo; tail: vị trí đọc tiếp theo.
 */
typedef struct {
	uint8_t buf[RX_BUF_SIZE]; /**< Bộ đệm dữ liệu. */
	volatile uint16_t head;	  /**< Con trỏ ghi (tiến vòng). */
	volatile uint16_t tail;	  /**< Con trỏ đọc (tiến vòng). */
} ring_buffer;

/** @brief Ring-buffer cho USART1. */
static ring_buffer usart1_rx = {0};
/** @brief Ring-buffer cho USART2. */
static ring_buffer usart2_rx = {0};
/** @brief Ring-buffer cho USART3. */
static ring_buffer usart3_rx = {0};

void usart_init(USART_TypeDef *usart, uint32_t baud_rate)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	USART_InitTypeDef USART_InitStruct = {0};
	NVIC_InitTypeDef NVIC_InitStruct = {0};

	if (usart == USART1) {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

		/* TX: PA9 */
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* RX: PA10 */
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(GPIOA, &GPIO_InitStruct);

		NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	} else if (usart == USART2) {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

		/* TX: PA2 */
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* RX: PA3 */
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(GPIOA, &GPIO_InitStruct);

		NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
	} else if (usart == USART3) {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

		/* TX: PB10 */
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOB, &GPIO_InitStruct);

		/* RX: PB11 */
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(GPIOB, &GPIO_InitStruct);

		NVIC_InitStruct.NVIC_IRQChannel = USART3_IRQn;
	} else {
		return;
	}

	USART_InitStruct.USART_BaudRate = baud_rate;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(usart, &USART_InitStruct);
	USART_Cmd(usart, ENABLE);

	/* Bật ngắt RXNE để nhận byte vào ring-buffer */
	USART_ITConfig(usart, USART_IT_RXNE, ENABLE);

	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_Init(&NVIC_InitStruct);
}

void usart_send_char(USART_TypeDef *usart, char c)
{
	while (USART_GetFlagStatus(usart, USART_FLAG_TXE) == RESET)
		;
	USART_SendData(usart, c);
}

void usart_send_string(USART_TypeDef *usart, const char *str)
{
	while (*str) {
		usart_send_char(usart, *str++);
	}
}

/**
 * @brief Kiểm tra buffer có dữ liệu.
 * @param rb Con trỏ ring-buffer.
 * @retval !=0 nếu có dữ liệu, 0 nếu trống.
 */
static inline int ring_buffer_available(ring_buffer *rb) { return (rb->head != rb->tail); }

/**
 * @brief Lấy 1 ký tự từ buffer (không chặn).
 * @param rb Con trỏ ring-buffer.
 * @retval Ký tự đọc được; trả về 0 nếu trống.
 * @note Chỉ đọc khi available()!=0.
 */
static inline char ring_buffer_get_char(ring_buffer *rb)
{
	char c = (char)rb->buf[rb->tail];
	rb->tail = (rb->tail + 1) % RX_BUF_SIZE;
	return c;
}

/**
 * @brief Đọc 1 byte với timeout (ms).
 * @param rb Con trỏ ring-buffer.
 * @param ch Con trỏ nhận byte.
 * @param timeout Thời gian chờ (ms).
 * @retval true nếu đọc thành công; false nếu timeout.
 */
static bool ring_buffer_read_byte_timeout(ring_buffer *rb, uint8_t *ch, uint32_t timeout)
{
	uint32_t start = get_tick();
	while (!ring_buffer_available(rb)) {
		if ((uint32_t)(get_tick() - start) >= timeout) {
			return false; /* timeout */
		}
	}
	*ch = (uint8_t)ring_buffer_get_char(rb);
	return true;
}

/**
 * @brief Đọc nhiều byte với timeout tổng (ms).
 * @param rb Con trỏ ring-buffer.
 * @param buf Bộ đệm đích.
 * @param len Số byte cần đọc.
 * @param timeout Thời gian chờ tổng (ms).
 * @retval true nếu đủ dữ liệu; false nếu timeout.
 * @note Vòng lặp chặn từng byte tới khi đủ hoặc hết thời gian.
 */
static bool ring_buffer_read_buffer_timeout(ring_buffer *rb, uint8_t *buf, uint16_t len,
					 uint32_t timeout)
{
	uint32_t start = get_tick();
	for (uint16_t i = 0; i < len; i++) {
		while (!ring_buffer_available(rb)) {
			if ((uint32_t)(get_tick() - start) >= timeout) {
				return false; /* timeout */
			}
		}
		buf[i] = (uint8_t)ring_buffer_get_char(rb);
	}
	return true;
}

/**
 * @brief Đưa 1 ký tự vào ring-buffer (ghi vòng).
 * @param rb Con trỏ ring-buffer.
 * @param c Ký tự.
 * @retval None
 * @note Nếu đầy (next_head == tail) thì byte mới sẽ ghi đè byte cũ nhất (overwrite mode).
 */
static inline void ring_buffer_put_char(ring_buffer *rb, char c)
{
	uint16_t next_head = (rb->head + 1) % RX_BUF_SIZE;

	// Nếu buffer đầy, di chuyển tail để tránh mất dữ liệu mới
	if (next_head == rb->tail) {
		rb->tail = (rb->tail + 1) % RX_BUF_SIZE;
	}

	// Luôn ghi dữ liệu vào buffer
	rb->buf[rb->head] = (uint8_t)c;
	rb->head = next_head;
}

/** @brief Trả về số byte khả dụng trong RX của USART1 (!=0 nếu có). */
static inline int usart1_available(void) { return ring_buffer_available(&usart1_rx); }
/** @brief Lấy 1 ký tự từ RX USART1 (không chặn, 0 nếu trống). */
static inline char usart1_get_char(void) { return ring_buffer_get_char(&usart1_rx); }
/** @brief Khởi tạo USART1. */
static inline void usart1_init(uint32_t baud_rate) { usart_init(USART1, baud_rate); }
/** @brief Gửi 1 ký tự qua USART1. */
static inline void usart1_send_char(char c) { usart_send_char(USART1, c); }
/** @brief Gửi chuỗi qua USART1. */
static inline void usart1_send_string(const char *str) { usart_send_string(USART1, str); }
/** @brief Đọc 1 byte từ RX USART1 với timeout. */
static inline bool usart1_read_byte_timeout(uint8_t *ch, uint32_t timeout)
{
	return ring_buffer_read_byte_timeout(&usart1_rx, ch, timeout);
}
/** @brief Đọc nhiều byte từ RX USART1 với timeout tổng. */
static inline bool usart1_read_buffer_timeout(uint8_t *buf, uint16_t len, uint32_t timeout)
{
	return ring_buffer_read_buffer_timeout(&usart1_rx, buf, len, timeout);
}

static inline int usart2_available(void) { return ring_buffer_available(&usart2_rx); }
static inline char usart2_get_char(void) { return ring_buffer_get_char(&usart2_rx); }
static inline void usart2_init(uint32_t baud_rate) { usart_init(USART2, baud_rate); }
static inline void usart2_send_char(char c) { usart_send_char(USART2, c); }
static inline void usart2_send_string(const char *str) { usart_send_string(USART2, str); }
static inline bool usart2_read_byte_timeout(uint8_t *ch, uint32_t timeout)
{
	return ring_buffer_read_byte_timeout(&usart2_rx, ch, timeout);
}
static inline bool usart2_read_buffer_timeout(uint8_t *buf, uint16_t len, uint32_t timeout)
{
	return ring_buffer_read_buffer_timeout(&usart2_rx, buf, len, timeout);
}

static inline int usart3_available(void) { return ring_buffer_available(&usart3_rx); }
static inline char usart3_get_char(void) { return ring_buffer_get_char(&usart3_rx); }
static inline void usart3_init(uint32_t baud_rate) { usart_init(USART3, baud_rate); }
static inline void usart3_send_char(char c) { usart_send_char(USART3, c); }
static inline void usart3_send_string(const char *str) { usart_send_string(USART3, str); }
static inline bool usart3_read_byte_timeout(uint8_t *ch, uint32_t timeout)
{
	return ring_buffer_read_byte_timeout(&usart3_rx, ch, timeout);
}
static inline bool usart3_read_buffer_timeout(uint8_t *buf, uint16_t len, uint32_t timeout)
{
	return ring_buffer_read_buffer_timeout(&usart3_rx, buf, len, timeout);
}

void usart_auto_init(void) __attribute__((constructor));
void usart_auto_init(void)
{
	uart1.init = usart1_init;
	uart1.send_char = usart1_send_char;
	uart1.send_string = usart1_send_string;
	uart1.available = usart1_available;
	uart1.get_char = usart1_get_char;
	uart1.read_byte_timeout = usart1_read_byte_timeout;
	uart1.read_buffer_timeout = usart1_read_buffer_timeout;

	uart2.init = usart2_init;
	uart2.send_char = usart2_send_char;
	uart2.send_string = usart2_send_string;
	uart2.available = usart2_available;
	uart2.get_char = usart2_get_char;
	uart2.read_byte_timeout = usart2_read_byte_timeout;
	uart2.read_buffer_timeout = usart2_read_buffer_timeout;

	uart3.init = usart3_init;
	uart3.send_char = usart3_send_char;
	uart3.send_string = usart3_send_string;
	uart3.available = usart3_available;
	uart3.get_char = usart3_get_char;
	uart3.read_byte_timeout = usart3_read_byte_timeout;
	uart3.read_buffer_timeout = usart3_read_buffer_timeout;
}

__attribute__((noreturn, unused)) static void _sys_exit(int x)
{
	(void)x;
	for (;;)
		;
}

struct __FILE {
	int handle;
};

/** @brief Đối tượng stdout giả lập cho printf. */
FILE __stdout;

/**
 * @brief Gửi 1 ký tự cho printf (retarget).
 * @param ch Ký tự.
 * @param f  FILE* (không dùng).
 * @retval Ký tự đã gửi.
 * @note Tự động chèn '\\r' trước '\\n' để hợp chuẩn terminal.
 */
int fputc(int ch, FILE *f)
{
	(void)f;
	if (ch == '\n') {
		DEBUG_USART.send_char('\r');
	}
	DEBUG_USART.send_char((char)ch);
	return ch;
}

/**
 * @brief Nhận 1 ký tự cho scanf (retarget).
 * @param f FILE* (không dùng).
 * @retval Ký tự đọc được.
 * @note Chờ tới khi có dữ liệu trong buffer RX.
 */
int fgetc(FILE *f)
{
	(void)f;
	while (!DEBUG_USART.available()) {
	}
	return (int)(DEBUG_USART.get_char());
}

/**
 * @brief Trình phục vụ ngắt RX USART1.
 * @note Đọc DR, đẩy vào ring-buffer, xóa cờ ngắt.
 */
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		uint8_t c = (uint8_t)(USART_ReceiveData(USART1) & 0xFF);
		ring_buffer_put_char(&usart1_rx, (char)c);
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}

void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		uint8_t c = (uint8_t)(USART_ReceiveData(USART2) & 0xFF);
		ring_buffer_put_char(&usart2_rx, (char)c);
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}
}

void USART3_IRQHandler(void)
{
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
		uint8_t c = (uint8_t)(USART_ReceiveData(USART3) & 0xFF);
		ring_buffer_put_char(&usart3_rx, (char)c);
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}
