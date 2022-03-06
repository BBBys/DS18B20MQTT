/**
 * @file MQTT.h
 * @author BBB
 * @brief MQTT-Programme für Heizungsensor2
 * @version 1.0
 * @date 15 Jan 2022 2021
 * 
 * @copyright Copyright (c) 2022-2021
 * 
 */
#ifndef MQTT_h
#define MQTT_h
#include <EspMQTTClient.h>
float SendeTemp(String messung, float temp);
void SendeStatus(String messung);
#endif