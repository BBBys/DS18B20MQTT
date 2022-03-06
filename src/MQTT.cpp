/**
 * @file MQTT.cpp
 * @brief MQTT-Routinen 
 * für Heizung-Temperatur-Sensor
 * @author B. Borys
 * @version 1.1.5
 * @date 6 März 15 12 Jan 2022 20 Feb 2021 31 28 10 9 3 1 Okt Sep 2020
 * @copyright Copyright (c) 2022 2021 2020
 */
#include "MQTT.h"
extern unsigned long LetzteMQTT ; ///< Zeitpunkt letzte Übertragung
// extern unsigned long MQCount ;

/**
 * @brief MQTT-Client zur Übertragung.
 * Parameter für WLAN und FHEM-MQTT-Broker
 */
 EspMQTTClient *myClient=new EspMQTTClient(
    WLANSSID,
    WLANPWD,
    MQTTBROKERIP, /// MQTT Broker server ip
    OTAUSER, OTAPASSWD,
    MQTTNAME, // Client name that uniquely identify your device
    1883      /// The MQTT port, default to 1883. this line can be omitted
);
 /**
 * @brief MQTT-Initialisierung.
 * nur Status-Meldung "Bereit"
 */
 void onConnectionEstablished()
 {
     /*
  client.subscribe("Temp/Innen", [](const String & payload)
  {
    //Serial.println(payload);
    TempI = payload.toDouble();
    //Serial.println(TempI);
    Anzeigen();
  }
                  );
  client.subscribe("Temp/Aussen", [](const String & payload)
  {
    //Serial.println(payload);
    TempA = payload.toDouble();
    //Serial.println(TempA);
    Anzeigen();
  }
                  );
  */
     myClient->publish("SWVersion", VERSION);
     myClient->publish("SWDatum", __DATE__);
     myClient->publish("OTA-Usr", OTAUSER);
     myClient->publish("OTA-Pwd", OTAPASSWD);
     SendeStatus("START");
 } //void onConnectionEstablished()
 /**
 * @brief Temperatur mit MQTT senden
 * @param messung Bezeichnung der Messstelle
 * @param temp die Temperatur, die gesendet werden soll
 * @return float die Temperatur, die gesendet wurde
 */
 float SendeTemp(String messung, float temp)
 {
   const char *FORMAT = "%5.1f";
   const int bufSize = 6;
   char stemp[bufSize];
  //  MQCount++;
   snprintf(stemp, bufSize, FORMAT, temp);
   myClient->publish(messung, stemp);
   LetzteMQTT = millis();
   return temp;
 }
 /**
  * @brief Statusmeldung senden
  * @param status String
  */
 void SendeStatus(String status)
 {
  //  MQCount++;
   myClient->publish("STATE",status);
   LetzteMQTT = millis();
   return;
 }
