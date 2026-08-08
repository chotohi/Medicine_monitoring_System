#include "intrins.h"
#include "delay.h"


void delay1ms(unsigned int milliseconds) /* 11.0592 MHz */
{
    unsigned char i;
    unsigned char j;

    while(milliseconds--)
    {
        _nop_();
        i = 2;
        j = 199;
        do
        {
            while(--j);
        }
        while(--i);
    }
}
