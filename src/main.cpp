/**
 * @file main.cpp
 * @brief Heizung-Temperatur-Sensor
 * ESP-01 mit DS18B20-Sensoren für Vorlauf-, Rücklauf- und Warmwasser-Temperatur.
 * @author Bernd-Burkhard Borys
 * @version 2.1.0
 * @date 6 März 15 12 Jan 2022 20 10 Feb 2021 1 Nov 31 28 10 9 3 1 Okt Sep 2020
 * @copyright Copyright (c) 2022-2021
 */
#include <Arduino.h>
#include "HS2.h"
#include <math.h>
#include "MQTT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
extern EspMQTTClient *myClient;
#define GPIO2 2 ///< alle Sensoren an GPIO2 von ESP-01.

/**
 * @brief Anschluss über OneWire
 */
OneWire SensorAnschluss(GPIO2);
/**
 * @brief DS18B20
 * @return DallasTemperature-Objekt
 */
DallasTemperature sensorenTemperatur(&SensorAnschluss);

/**
 * @brief Setup-Programm
 */
void setup()
{
#ifndef NDEBUG
  Serial.begin(115200);
#endif
  pinMode(LED_BUILTIN, OUTPUT);   /// blaue LED vorbereiten
  digitalWrite(LED_BUILTIN, LOW); /// und einschalten
  // Optionnal functionnalities of EspMQTTClient :
  // client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  myClient->enableHTTPWebUpdater();                                     /// Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
  myClient->enableLastWillMessage("HeizungSensor/lastwill", "Abbruch"); /// LWT-Meldung
  sensorenTemperatur.begin();                                           /// Sensoren initialisieren
} //setup
float WwGesendet = (-200), temp1 = (-200), VlGemessen = (-200), RlGemessen = (-200), deltaGesendet = (-200); ///< Temperaturen bei letzter MQTT-Nachricht. Initialisiert mit unmöglichem Wert
unsigned long LetzteMQTT = 0;                                                                                ///< Zeitpunkt letzte Übertragung
unsigned long LetzteMuss = 0;                                                                                ///< Zeitpunkt letzte Zwangsübertragung
// unsigned long MQCount = 0;
unsigned long LetzteRuecklaufMessen = 0; ///< Zeitpunkt letzte Messung der Rücklauftemperatur, wird jeder Minute gemessen, nach 5 Messungen gesendet
unsigned long LetzteVorlaufMessen = 0;   ///< Zeitpunkt letzte Messung der Vorlauftemperatur, wird jeder Minute gemessen, nach 5 Messungen gesendet
/**
 * @brief das übliche Hauptprogramm
 */
void loop()
{
  static bool HeizungAn = false;
  static float sumVl = 0.0, sumDelta = 0.0;
  static int nVl = 0;
  static float sumRl = 0.0;
  static int nRl = 0, nDelta = 0;
  myClient->loop();                /// MQTT-Loop
  if (myClient->isMqttConnected()) /// wenn MQTT-Verbindung besteht
  {
    // normal: Mittelung über 10 Minuten = Abfrage alle 30 s und Mittelung über 20 Werte
    const unsigned long DELTAABSTAND = 30000; /// Mindestabstand MQTT-Übertragung 15 Sekunden in ms
    const unsigned long RUECKABSTAND = 30000; /// Mindestabstand Rücklauf-Messung 30 s in ms
    const unsigned long VORABSTAND = 30000;   /// Mindestabstand Vorlauf-Messung 30 s in ms
    const int NDELTA = 20;                    /// Mittelung der Differenz über NDELTA Messungen
    const int NRL = 20;                       /// Mittelung der Rücklauftemperatur über NRL Messungen
    const int NVL = 20;                       /// Mittelung der Vorlauftemperatur über NVL Messungen
    // schneller:
    //  const unsigned long DELTAABSTAND = 1500;   /// Mindestabstand MQTT-Übertragung 15 Sekunden in ms
    //  const unsigned long RUECKABSTAND = 6000; /// Mindestabstand Rücklauf-Messung 1 Minute in ms
    //  const unsigned long VORABSTAND = 6000;   /// Mindestabstand Vorlauf-Messung 1 Minute in ms
    //  const int NDELTA = 20;                   /// Mittelung der Differenz über NDELTA Messungen
    //  const int NRL = 10;                      /// Mittelung der Rücklauftemperatur über NRL Messungen
    //  const int NVL = 10;                      /// Mittelung der Vorlauftemperatur über NVL Messungen
    /// erst Abfrage aller Sensoren
    sensorenTemperatur.requestTemperatures();
    /// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    /// nur für Warmwasser und Vor/Rück-Differenz:
    /// messen und senden, wenn minimaler Abstand und minimale Änderung überschritten
    /// für Differenz:
    /// NDELTA Messungen addieren,
    /// nach der NDELTA. Messung den Mittelwert senden,
    /// wenn Delta unter 2 K: Heizung ist vermutlich aus
    if (((unsigned long)(millis() - LetzteMQTT) > DELTAABSTAND)) /// und DELTAABSTAND vorüber: Temperaturen abfragen und senden
                                                                 ///(funktioniert in dieser Form auch nach Überlauf von millis() )
    {
      const float MINANDAN = 3, MINANDAUS = 20; /// Mindeständerung in K für Warmwasser an und aus
      float WwGemessen, delta;                  // gemeldete Temperatur
      LetzteMQTT = millis();
      digitalWrite(LED_BUILTIN, HIGH); /// blaue LED aus im Normalfall
      /// dann Temperaturen einzeln und bei Änderung senden
      WwGemessen = sensorenTemperatur.getTempCByIndex(0);
      if (((WwGemessen - WwGesendet) > MINANDAN) // Warmwasser an: schnell reagieren
          |
          ((WwGesendet - WwGemessen) > MINANDAUS) // Warmwasser aus: Wert länger halten
      )
        WwGesendet = SendeTemp("Warmwasser", WwGemessen);
      VlGemessen = sensorenTemperatur.getTempCByIndex(1);
      RlGemessen = sensorenTemperatur.getTempCByIndex(2);
      delta = VlGemessen - RlGemessen;
      sumDelta += delta;
      nDelta++;

      if (nDelta >= NDELTA)
      {
        delta = sumDelta / (float)nDelta;
        if (delta > 2.0)
        {
          // SendeStatus("TEST");
          deltaGesendet = SendeTemp("Delta", delta);
          if (!HeizungAn)
          {
            HeizungAn = true;
            SendeStatus("AN");
          }
        }
        else
        {
          HeizungAn = false;
          deltaGesendet = 0;
          SendeStatus("AUS");
        }

        nDelta = 0;
        sumDelta = 0.0;
      } // if (nDelta >= NDELTA)
    }   // DELTAABSTAND
    /// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    /// nur für Rücklauf:
    /// messen, wenn RUECKABSTAND (1 Minute) vorüber
    /// NRL (100) Messungen addieren
    /// nach der NRL. Messung den Mittelwert senden
    if ((unsigned long)(millis() - LetzteRuecklaufMessen) > RUECKABSTAND) /// RUECKABSTAND vorüber:  jetzt messen
                                                                          /// (funktioniert in dieser Form auch nach Überlauf von millis() )
    {
      LetzteRuecklaufMessen = millis();
      RlGemessen = sensorenTemperatur.getTempCByIndex(2);
      sumRl += RlGemessen;
      nRl++;
      if (nRl > NRL)
      {
        SendeTemp("Ruecklauf", sumRl / (float)nRl);
        nRl = 0;
        sumRl = 0;
      } // nRl > NRL
    }   // RUECKABSTAND
    /// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    /// nur für Vorlauf:
    /// messen, wenn VORABSTAND vorüber
    /// NVL Messungen addieren
    /// nach der letzten Mittelwert senden
    if ((unsigned long)(millis() - LetzteVorlaufMessen) > VORABSTAND) /// VORABSTAND vorüber:  jetzt messen
                                                                      ///  (funktioniert in dieser Form auch nach Überlauf von millis() )
    {
      LetzteVorlaufMessen = millis();
      VlGemessen = sensorenTemperatur.getTempCByIndex(1);
      sumVl += VlGemessen;
      nVl++;
      if (nVl > NVL)
      {
        SendeTemp("Vorlauf", sumVl / (float)nVl);
        nVl = 0;
        sumVl = 0;
      } // nVl > NVL
    }   // VORABSTAND
  }     // Verbindung
  else  /// wenn keine MQTT-Verbindung: Blaue LED blinkt
  {
    LetzteMQTT = 0;
    BlaueLEDblinkt();
  } // else: wenn keine MQTT-Verbindung: Blaue LED blinkt
  delay(1000);
} // loop