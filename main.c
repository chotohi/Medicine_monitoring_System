#include "REG52.h"
#include "hx711.h"
#include "DHT11.h"
#include "delay.h"
#include "usart.h"
#include "esp8266.h"

#define TIMER0_RELOAD       (65536UL - 922UL) /* 1 ms @ 11.0592 MHz / 12 */
#define SAMPLE_INTERVAL_MS  60000U
#define HX711_SCALE         1028UL

#define WIFI_SSID           "WiFi_name"
#define WIFI_PASSWORD       "WiFi_password"
#define SERVER_IP           "8.155.44.172"
#define SERVER_PORT         "2025"

volatile unsigned int system_ms = 0;
volatile unsigned int sample_ms = 0;
volatile bit flag_1min = 0;

unsigned long weight;
unsigned int temp;
unsigned int humidity;
unsigned char col = 0;
unsigned char lin = 0;
int *sensor_data;
char msg[32];

void Timer0_Init(void);
void BuildMsg(char *buf,
              unsigned int name,
              unsigned int hum,
              unsigned int temperature,
              unsigned long current_weight);
void gpio_write(unsigned char line,
                unsigned char column,
                bit value);
bit ConnectNetwork(void);


void timer0_ISR(void) interrupt 1
{
    TH0 = (unsigned char)(TIMER0_RELOAD >> 8);
    TL0 = (unsigned char)TIMER0_RELOAD;

    system_ms++;
    if(++sample_ms >= SAMPLE_INTERVAL_MS)
    {
        sample_ms = 0;
        flag_1min = 1;
    }
}


void Timer0_Init(void)
{
    TMOD = (TMOD & 0xF0) | 0x01;
    TH0 = (unsigned char)(TIMER0_RELOAD >> 8);
    TL0 = (unsigned char)TIMER0_RELOAD;
    ET0 = 1;
    TR0 = 1;
    EA = 1;
}


void main(void)
{
    UART_Init();
    Timer0_Init();
    delay1ms(2000);

    while(!ESP_Init())
        delay1ms(1000);

    while(!ConnectNetwork())
        delay1ms(2000);

    while(1)
    {
        if(flag_1min)
        {
            flag_1min = 0;

            gpio_write(lin, col, 1);
            weight = ReadWeight() / HX711_SCALE;
            sensor_data = DHT11_Read();
            gpio_write(lin, col, 0);

            if(sensor_data[0] >= 0 && sensor_data[1] >= 0)
            {
                humidity = (unsigned int)sensor_data[0];
                temp = (unsigned int)sensor_data[1];

                BuildMsg(msg,
                         (unsigned int)(lin * 10 + col),
                         humidity,
                         temp,
                         weight);

                if(!ESP_TCP_Send(msg))
                {
                    if(ConnectNetwork())
                        ESP_TCP_Send(msg);
                }
            }

            if(++col == 8)
            {
                col = 0;
                if(++lin == 3)
                    lin = 0;
            }
        }
    }
}


bit ConnectNetwork(void)
{
    if(!ESP_ConnectWiFi(WIFI_SSID, WIFI_PASSWORD))
        return 0;

    return ESP_TCP_Connect(SERVER_IP, SERVER_PORT);
}


void gpio_write(unsigned char line,
                unsigned char column,
                bit value)
{
    unsigned char mask = 1 << column;

    switch(line)
    {
        case 0:
            if(value) P0 |= mask;
            else      P0 &= ~mask;
            break;
        case 1:
            if(value) P1 |= mask;
            else      P1 &= ~mask;
            break;
        case 2:
            if(value) P2 |= mask;
            else      P2 &= ~mask;
            break;
    }
}


static char *AppendUnsignedLong(char *buf, unsigned long value)
{
    char digits[10];
    unsigned char count = 0;

    do
    {
        digits[count++] = (char)(value % 10UL) + '0';
        value /= 10UL;
    }
    while(value != 0UL);

    while(count > 0)
        *buf++ = digits[--count];

    return buf;
}


void BuildMsg(char *buf,
              unsigned int name,
              unsigned int hum,
              unsigned int temperature,
              unsigned long current_weight)
{
    char *cursor = buf;

    *cursor++ = 'M';
    *cursor++ = 'e';
    *cursor++ = 'd';
    *cursor++ = (char)(name / 10U) + '0';
    *cursor++ = (char)(name % 10U) + '0';
    *cursor++ = '/';
    cursor = AppendUnsignedLong(cursor, (unsigned long)hum);
    *cursor++ = '/';
    cursor = AppendUnsignedLong(cursor, (unsigned long)temperature);
    *cursor++ = '/';
    cursor = AppendUnsignedLong(cursor, current_weight);
    *cursor++ = '\r';
    *cursor++ = '\n';
    *cursor = '\0';
}
