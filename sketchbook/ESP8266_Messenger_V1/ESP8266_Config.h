//
// ************************************************************************
// A rework on use an ESP8266-module as a remote sensor/actor.
// (C) 2016 Dirk Schanz aka dreamshader
//
// functional description:
// 
// protocols step by step.
//
// ************************************************************************
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ************************************************************************
//
//-------- History -------------------------------------------------------
//
// 1st version: 02/08/16
//         Join an existing WLAN and start a webserver
// update:
//
// -----------------------------------------------------------------------
//
// ######################### configuration section ########################
//
// ************************************************************************
// set these defines to match your ESP-module 
// ************************************************************************
// 
#define USE_ESP01
// #undef USE_ESP01
//
// #define USE_ESP07
#undef USE_ESP07
//
// #define USE_ESP12
#undef USE_ESP12
//
// #define USE_ESP12E
#undef USE_ESP12E
//
// which pin is used for factory settings
#define GPIO_AS_RESET           0  // GPIO00 is always possible
// #undef GPIO_AS_RESET              // no reset
//
// ************************************************************************
// set these defines to match used communication interface(s)
// ************************************************************************
// 
// ------------------- (de)activate I2C communication
#define USE_I2C
// #undef USE_I2C
//
// ------------------- I2C settings
#ifdef USE_I2C
#define PIN_SDA  2
#define PIN_SCL  4
#endif // USE_I2C
//
// ------------------- (de)activate 1 Wire connection
#define USE_1WIRE
// #undef USE_1WIRE
//
// ------------------- 1 Wire settings
#ifdef USE_1WIRE
#define PIN_1WIRE  4
#endif // USE_1WIRE
//
// ------------------- (de)activate RS232 connection
#define USE_RS232
// #undef USE_RS232
//
// ------------------- RS232 settings
#ifdef USE_RS232
#define PIN_TXD     2
#define PIN_RXD     2
#endif // USE_RS232
//
// ------------------- (de)activate SPI communication
#define USE_SPI
// #undef USE_SPI
//
// ------------------- SPI settings
#ifdef USE_SPI
#define PIN_MOSI     2
#define PIN_MISO     2
#define PIN_SCK      2
#define PIN_CS       2
#endif // USE_SPI
//
// ---- other -----
//
// ************************************************************************
// set these defines to match your sensor(s)
// ************************************************************************
//
// --------------------- sensor type DS18B20 -----------------------------
#define USE_DS18B20
// #undef USE_DS18B20
// ------------------- sensor type DHT11/DHT22 ---------------------------
// #define USE_DHT
#undef USE_DHT
// ------------------ sensor type BMP085/BMP180 --------------------------
// #define USE_BMP085
#undef USE_BMP085
//
// ************************************************************************
// set these defines to match your actor(s)
// ************************************************************************
//
// ------------------- display data on I2C LCD ---------------------------
// #define USE_I2C_LCD
#undef USE_I2C_LCD
//
// ------------------ display data on 4 bit LCD --------------------------
// #define USE_4BIT_LCD
#undef USE_4BIT_LCD
//
// ------------------ signaling using a RGB LED --------------------------
// #define USE_RGB_LED
#undef USE_RGB_LED
//
// ---------------------- alert using buzzer -----------------------------
// #define USE_BUZZER
#undef USE_BUZZER
//
// --------- switch realais depending of sensor state(s) -----------------
// #define USE_RELAIS
#undef USE_RELAIS
//
// ************************************************************************
// set these defines to match your API(s) used to deliver data
// ************************************************************************
//
// -------- use SHC for storage and visualization of data ----------------
// #define USE_WEBAPI_SHC
#undef USE_WEBAPI_SHC
//
// ----------- use EMONCMS to store and visualize data -------------------
// #define USE_WEBAPI_EMONCMS
#undef USE_WEBAPI_EMONCMS
//
// ************************************************************************
// set these defines to match software features of your ESP module
// ************************************************************************
// use logging ( otherwise no logging code will be generated )
#define USE_LOGGING
// #undef USE_LOGGING
//
//  create a wifi client
#define USE_WIFICLIENT
//  create a webserver
#define USE_WWWSERVER
//  port for the webserver
#define WWW_LISTENPORT         80
//  support for SD cards
#define SD_SUPPORT
//
// ########################### read only section ##########################
//
// ************************************************************************
// WARNING!
// do NOT change anything in the following section except you really
// know what you are doing!
// ************************************************************************
//
//
// ************************************************************************
// include necessary lib header
// ************************************************************************
//
#ifdef USE_LOGGING
#include "SimpleLog.h"
#endif //
//

#ifdef USE_WIFICLIENT
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#endif // USE_WIFICLIENT

#include <ArduinoJson.h>
#include <EEPROM.h>

#ifdef SD_SUPPORT
#include <SPI.h>
#include <SD.h>
#endif // SD_SUPPORT

#ifdef USE_DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#endif // USE_DS18B20

#ifdef USE_DHT
#include <DHT.h>
#endif // USE_DHT

#ifdef USE_BMP085
#include <Wire.h>
#include <Adafruit_BMP085.h>
#endif // USE_BMP085

//
// ####################### nothing behind this line #######################

