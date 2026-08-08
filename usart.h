#ifndef __USART_H__
#define __USART_H__

void UART_Init(void);
void UART_SendByte(unsigned char dat);
void UART_SendString(char *str);

#endif
