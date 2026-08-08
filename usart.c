#include <REG52.H>
#include "usart.h"


void UART_Init(void)
{
    TMOD = (TMOD & 0x0F) | 0x20;
    TH1 = 0xFD; /* 9600 baud @ 11.0592 MHz */
    TL1 = 0xFD;
    TR1 = 1;
    SCON = 0x50;
    RI = 0;
    TI = 0;
    ES = 0; /* ESP 响应由轮询读取，不使用串口中断 */
}


void UART_SendByte(unsigned char dat)
{
    SBUF = dat;
    while(!TI);
    TI = 0;
}


void UART_SendString(char *str)
{
    while(*str)
        UART_SendByte(*str++);
}
