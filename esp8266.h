#ifndef __ESP8266_H__
#define __ESP8266_H__

bit ESP_Wait(char *str, unsigned int timeout_ms);
void ESP_SendCmd(char *cmd);
bit ESP_Init(void);
bit ESP_ConnectWiFi(char *ssid, char *pwd);
bit ESP_TCP_Connect(char *ip, char *port);
bit ESP_TCP_Send(char *payload);

#endif
