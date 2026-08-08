#include "DHT11.h"
#include "intrins.h"

sbit DATA = P3^3;


void Delay1us(unsigned char microseconds) /* 约 1 us @ 11.0592 MHz */
{
    unsigned char i;

    for(i = 0; i < microseconds; i++)
        _nop_();
}


static bit WaitWhile(bit level, unsigned char timeout)
{
    while(DATA == level)
    {
        if(timeout-- == 0)
            return 0;
        Delay1us(1);
    }

    return 1;
}


static bit ReadByte(unsigned char *value)
{
    unsigned char i;
    unsigned char high_time;
    unsigned char result = 0;

    for(i = 0; i < 8; i++)
    {
        if(!WaitWhile(0, 100))
            return 0;

        high_time = 0;
        while(DATA)
        {
            if(high_time++ >= 100)
                return 0;
            Delay1us(1);
        }

        result <<= 1;
        if(high_time > 30)
            result |= 0x01;
    }

    *value = result;
    return 1;
}


int *DHT11_Read(void)
{
    unsigned char humidity_integer;
    unsigned char humidity_decimal;
    unsigned char temp_integer;
    unsigned char temp_decimal;
    unsigned char checksum;
    static int sensor_values[2] = {-1, -1};

    sensor_values[0] = -1;
    sensor_values[1] = -1;

    DATA = 0;
    delay1ms(20);
    DATA = 1;
    Delay1us(30);

    if(!WaitWhile(1, 100))
        return sensor_values;
    if(!WaitWhile(0, 100))
        return sensor_values;
    if(!WaitWhile(1, 100))
        return sensor_values;

    if(!ReadByte(&humidity_integer))
        return sensor_values;
    if(!ReadByte(&humidity_decimal))
        return sensor_values;
    if(!ReadByte(&temp_integer))
        return sensor_values;
    if(!ReadByte(&temp_decimal))
        return sensor_values;
    if(!ReadByte(&checksum))
        return sensor_values;

    if(checksum == (unsigned char)(humidity_integer + humidity_decimal
                                  + temp_integer + temp_decimal))
    {
        sensor_values[0] = humidity_integer;
        sensor_values[1] = temp_integer;
    }

    return sensor_values;
}
