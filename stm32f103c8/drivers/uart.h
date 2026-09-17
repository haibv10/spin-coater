/**
 * @file main.h
 * @brief Header for project Spin Coater.
 * @author haihbv
 * @date December 2025
 */
#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdbool.h>

/**
 * @brief Bảng hàm (driver) cho một instance USART.
 *
 * @note Các hàm trong bảng này đều là wrapper quanh hiện thực phần cứng cụ thể
 *       (USART1/USART2/USART3). Dùng như một đối tượng driver.
 */
typedef struct {
	/**
	 * @brief Khởi tạo USART với baudrate chỉ định, bật ngắt RXNE.
	 * @param baud_rate Baudrate (ví dụ 115200).
	 */
	void (*init)(uint32_t baud_rate);

	/**
	 * @brief Gửi 1 ký tự.
	 * @param c Ký tự cần gửi.
	 */
	void (*send_char)(char c);

	/**
	 * @brief Gửi chuỗi C kết thúc bằng '\\0'.
	 * @param str Con trỏ chuỗi.
	 */
	void (*send_string)(const char *str);

	/**
	 * @brief Kiểm tra buffer RX có dữ liệu.
	 * @retval !=0 nếu có dữ liệu, 0 nếu trống.
	 */
	int (*available)(void);

	/**
	 * @brief Lấy 1 ký tự từ buffer RX.
	 * @retval Ký tự đọc được; trả về 0 nếu trống.
	 * @note Không chặn.
	 */
	char (*get_char)(void);

	/**
	 * @brief Đọc 1 byte với timeout.
	 * @param ch Con trỏ nhận byte.
	 * @param timeout Thời gian chờ (ms).
	 * @retval true nếu đọc được; false nếu timeout.
	 */
	bool (*read_byte_timeout)(uint8_t *ch, uint32_t timeout);

	/**
	 * @brief Đọc nhiều byte với timeout tổng.
	 * @param buf Bộ đệm đích.
	 * @param len Số byte cần đọc.
	 * @param timeout Thời gian chờ tổng (ms).
	 * @retval true nếu đủ dữ liệu; false nếu timeout.
	 */
	bool (*read_buffer_timeout)(uint8_t *buf, uint16_t len, uint32_t timeout);
} usart_driver;

/** @brief Đối tượng driver cho USART1. */
extern usart_driver uart1;
/** @brief Đối tượng driver cho USART2. */
extern usart_driver uart2;
/** @brief Đối tượng driver cho USART3. */
extern usart_driver uart3;

/**
 * @brief Khởi tạo phần cứng cho usart và bật ngắt RXNE.
 * @param usart Con USART (USART1/USART2/USART3).
 * @param baud_rate Baudrate mong muốn (ví dụ 115200).
 * @note Cấu hình 8-N-1, không dùng flow control.
 */
void usart_init(USART_TypeDef *usart, uint32_t baud_rate);

/*****************************************************************
 * Transmit
 *****************************************************************/

/**
 * @brief Gửi 1 ký tự qua USART (polling).
 * @param usart Con USART.
 * @param c Ký tự.
 * @note Chờ cờ TXE trước khi ghi.
 */
void usart_send_char(USART_TypeDef *usart, char c);

/**
 * @brief Gửi chuỗi C qua USART (polling).
 * @param usart Con USART.
 * @param str Chuỗi kết thúc '\\0'.
 */
void usart_send_string(USART_TypeDef *usart, const char *str);

/*****************************************************************
 * Receive (IRQ handlers)
 *****************************************************************/

/**
 * @brief Trình phục vụ ngắt RX cho USART1.
 * @note Đọc DR và đẩy vào ring-buffer.
 */
void USART1_IRQHandler(void);

/**
 * @brief Trình phục vụ ngắt RX cho USART2.
 */
void USART2_IRQHandler(void);

/**
 * @brief Trình phục vụ ngắt RX cho USART3.
 */
void USART3_IRQHandler(void);

#endif // __USART_H

/* Designed by haihbv */
