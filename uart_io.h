/*
 * uart_io.h
 *
 *  Created on: Jul 8, 2019
 *      Author: Tyler
 */

#ifndef UART_IO_H_
#define UART_IO_H_

#include "Board.h"
#include "eusci_a_uart.h"
#include "gpio.h"

void uart_io_init(void);
void printWord(char* word);
void itoa(int32_t num);

#endif /* UART_IO_H_ */
