/**
 * @file HS2Hilfen.cpp
 * @author Bernd-Burkhard Borys
 * @brief Hilfsprogramme für Heizungsensor2
 * @version 1.0
 * @date 6 März 15 Jan 2022
 * @copyright Copyright (c) 2022
 */
#include <Arduino.h>
#include "HS2.h"
/**
 * @brief Blaue LED blinkt
 */
void BlaueLEDblinkt()
{
digitalWrite(LED_BUILTIN, LOW);
delay(333);
digitalWrite(LED_BUILTIN, HIGH);
delay(333);
digitalWrite(LED_BUILTIN, LOW);
}