#include <REG52.H>
#include "delay.h"
#include "usart.h"
#include "esp8266.h"

#define ESP_RETRY_COUNT          3
#define ESP_AT_TIMEOUT_MS        2000U
#define ESP_WIFI_TIMEOUT_MS      20000U
#define ESP_TCP_TIMEOUT_MS       10000U
#define ESP_SEND_TIMEOUT_MS      5000U

extern volatile unsigned int system_ms;


static unsigned int ESP_GetMillis(void)
{
    unsigned int value;
    unsigned char interrupt_state = EA;

    EA = 0;
    value = system_ms;
    EA = interrupt_state;
    return value;
}


static bit ESP_WaitTwo(char *first, char *second, unsigned int timeout_ms)
{
    unsigned char first_index = 0;
    unsigned char second_index = 0;
    unsigned char received;
    unsigned int start = ESP_GetMillis();

    while((unsigned int)(ESP_GetMillis() - start) < timeout_ms)
    {
        if(RI)
        {
            RI = 0;
            received = SBUF;

            if(received == first[first_index])
            {
                first_index++;
                if(first[first_index] == '\0')
                    return 1;
            }
            else
            {
                first_index = (received == first[0]) ? 1 : 0;
            }

            if(second != 0)
            {
                if(received == second[second_index])
                {
                    second_index++;
                    if(second[second_index] == '\0')
                        return 1;
                }
                else
                {
                    second_index = (received == second[0]) ? 1 : 0;
                }
            }
        }
    }

    return 0;
}


bit ESP_Wait(char *str, unsigned int timeout_ms)
{
    return ESP_WaitTwo(str, 0, timeout_ms);
}


void ESP_SendCmd(char *cmd)
{
    RI = 0;
    UART_SendString(cmd);
    UART_SendString("\r\n");
}


bit ESP_Init(void)
{
    unsigned char attempt;

    for(attempt = 0; attempt < ESP_RETRY_COUNT; attempt++)
    {
        ESP_SendCmd("AT");
        if(ESP_Wait("OK", ESP_AT_TIMEOUT_MS))
            return 1;
        delay1ms(300);
    }

    return 0;
}


bit ESP_ConnectWiFi(char *ssid, char *pwd)
{
    unsigned char attempt;

    for(attempt = 0; attempt < ESP_RETRY_COUNT; attempt++)
    {
        RI = 0;
        UART_SendString("AT+CWJAP=\"");
        UART_SendString(ssid);
        UART_SendString("\",\"");
        UART_SendString(pwd);
        UART_SendString("\"\r\n");

        if(ESP_WaitTwo("OK", "WIFI GOT IP", ESP_WIFI_TIMEOUT_MS))
            return 1;
        delay1ms(500);
    }

    return 0;
}


bit ESP_TCP_Connect(char *ip, char *port)
{
    unsigned char attempt;

    for(attempt = 0; attempt < ESP_RETRY_COUNT; attempt++)
    {
        RI = 0;
        UART_SendString("AT+CIPSTART=\"TCP\",\"");
        UART_SendString(ip);
        UART_SendString("\",");
        UART_SendString(port);
        UART_SendString("\r\n");

        if(ESP_WaitTwo("OK", "ALREADY CONNECTED", ESP_TCP_TIMEOUT_MS))
            return 1;
        delay1ms(500);
    }

    return 0;
}


static void ESP_SendLength(unsigned int value)
{
    char digits[5];
    unsigned char count = 0;

    do
    {
        digits[count++] = (char)(value % 10U) + '0';
        value /= 10U;
    }
    while(value != 0U);

    while(count > 0)
        UART_SendByte(digits[--count]);
}


bit ESP_TCP_Send(char *payload)
{
    unsigned int length = 0;

    while(payload[length] != '\0')
        length++;

    RI = 0;
    UART_SendString("AT+CIPSEND=");
    ESP_SendLength(length);
    UART_SendString("\r\n");

    if(!ESP_Wait(">", ESP_SEND_TIMEOUT_MS))
        return 0;

    UART_SendString(payload);
    return ESP_Wait("SEND OK", ESP_SEND_TIMEOUT_MS);
}
