/*
  Created by Igor Jarc
  See http://iot-playground.com for details
  Please use community fourum on website do not contact author directly

  Code based on https://github.com/DennisSc/easyIoT-ESPduino/blob/master/sketches/ds18b20.ino

  External libraries:
  - https://github.com/adamvr/arduino-base64
  - https://github.com/milesburton/Arduino-Temperature-Control-Library

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
*/

// ////////////////////////////////////////////////////////
// EEPROM.begin(512);
// int addr=0;
// addr += EEPROM.put(addr, myFloat);
// addr += EEPROM.put(addr, myInt);
// EEPROM.end()
// ////////////////////////////////////////////////////////
// EEPROM.begin(512);
// int addr=0;
// addr += EEPROM.get(addr, myNewFloat);
// addr += EEPROM.get(addr, myNewInt);
// EEPROM.commit();
// EEPROM.end()
// ////////////////////////////////////////////////////////
// GPIO02 is free for 1w Sensors
// ////////////////////////////////////////////////////////




#include <ESP8266WiFi.h>
#include <Base64.h>
#include <EEPROM.h>
#include <Wire.h>
//#include <extEEPROM.h>

#include <OneWire.h>
#include <DallasTemperature.h>

//AP definitions
#define AP_SSID "your ssid"
#define AP_PASSWORD "ap password"

// EasyIoT server definitions
#define EIOT_USERNAME    "admin"
#define EIOT_PASSWORD    "test"
#define EIOT_IP_ADDRESS  "192.168.1.5"
#define EIOT_PORT        80
#define EIOT_NODE        "N13S0"

#define REPORT_INTERVAL 60 // in sec


#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);


#define USER_PWD_LEN 40
char unameenc[USER_PWD_LEN];
float oldTemp;


/*
 *  *******************************************
*/
//Two 24LC256 EEPROMs on the bus
const uint32_t totalKBytes = 64;         //for read and write test functions
extEEPROM eep(kbits_256, 2, 64);         //device size, number of devices, page size

//write test data (32-bit integers) to eeprom, "chunk" bytes at a time
void eeWrite(uint8_t chunk)
{
  chunk &= 0xFC;                //force chunk to be a multiple of 4
  uint8_t data[chunk];
  uint32_t val = 0;
  //    Serial << F("Writing...") << endl;
  uint32_t msStart = millis();

  for (uint32_t addr = 0; addr < totalKBytes * 1024; addr += chunk) {
    //        if ( (addr &0xFFF) == 0 )
    //           Serial << addr << endl;

    for (uint8_t c = 0; c < chunk; c += 4) {
      data[c + 0] = val >> 24;
      data[c + 1] = val >> 16;
      data[c + 2] = val >> 8;
      data[c + 3] = val;
      ++val;
    }
    eep.write(addr, data, chunk);
  }
  uint32_t msLapse = millis() - msStart;
}

//read test data (32-bit integers) from eeprom, "chunk" bytes at a time
void eeRead(uint8_t chunk)
{
  chunk &= 0xFC;                //force chunk to be a multiple of 4
  uint8_t data[chunk];
  uint32_t val = 0, testVal;
  //    Serial << F("Reading...") << endl;
  uint32_t msStart = millis();

  for (uint32_t addr = 0; addr < totalKBytes * 1024; addr += chunk) {
    //        if ( (addr &0xFFF) == 0 )
    //          Serial << addr << endl;

    eep.read(addr, data, chunk);
    for (uint8_t c = 0; c < chunk; c += 4) {
      testVal =  ((uint32_t)data[c + 0] << 24) + ((uint32_t)data[c + 1] << 16) + ((uint32_t)data[c + 2] << 8) + (uint32_t)data[c + 3];
      //            if (testVal != val)

      ++val;
    }
  }
  uint32_t msLapse = millis() - msStart;
  //    Serial << "Last value: " << --val << " Read lapse: " << msLapse << " ms" << endl;
}

//write 0xFF to eeprom, "chunk" bytes at a time
void eeErase(uint8_t chunk, uint32_t startAddr, uint32_t endAddr)
{
  chunk &= 0xFC;                //force chunk to be a multiple of 4
  uint8_t data[chunk];
  for (int i = 0; i < chunk; i++) data[i] = 0xFF;
  uint32_t msStart = millis();

  for (uint32_t a = startAddr; a <= endAddr; a += chunk) {
    //        if ( (a &0xFFF) == 0 )
    //          Serial << a << endl;
    eep.write(a, data, chunk);
  }
  uint32_t msLapse = millis() - msStart;
}

/*
 * *******************************************
*/


void setup() {
  unsigned long time;
  static unsigned long last_time;
  int do_reset = 0;
  //  int vari;


  Serial.begin(115200);

  time = millis();
  if (last_time == 0)
  {
    last_time = time;
  }

  while ( (digitalRead(ONE_WIRE_BUS) == HIGH) && ((time - last_time) <= 5000) )
  {
    time = millis();
  }

  if ( time - last_time >= 1000 )
  {
    do_reset = TRUE;
    //    EEPROM.update(0, 0);
    //    EEPROM.get(0, vari);
    //    EEPROM.put(16, vari);
  }

  if (do_reset == TRUE)
  {

  }
  else
  {
    wifiConnect();

    char uname[USER_PWD_LEN];
    String str = String(EIOT_USERNAME) + ":" + String(EIOT_PASSWORD);
    str.toCharArray(uname, USER_PWD_LEN);
    memset(unameenc, 0, sizeof(unameenc));
    base64_encode(unameenc, uname, strlen(uname));
  }

  oldTemp = -1;

}

void loop() {
  float temp;

  do {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
    Serial.print("Temperature: ");
    Serial.println(temp);
  } while (temp == 85.0 || temp == (-127.0));

  if (temp != oldTemp)
  {
//    sendTeperature(temp);
    oldTemp = temp;
  }

  int cnt = REPORT_INTERVAL;

  while (cnt--)
    delay(1000);
}

void wifiConnect()
{
  Serial.print("Connecting to AP");
  WiFi.begin(AP_SSID, AP_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
}

void sendTeperature(float temp)
{
  WiFiClient client;

  while (!client.connect(EIOT_IP_ADDRESS, EIOT_PORT)) {
    Serial.println("connection failed");
    wifiConnect();
  }

  String url = "";
  url += "/Api/EasyIoT/Control/Module/Virtual/" + String(EIOT_NODE) + "/ControlLevel/" + String(temp); // generate EasIoT server node URL

  Serial.print("POST data to URL: ");
  Serial.println(url);

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + String(EIOT_IP_ADDRESS) + "\r\n" +
               "Connection: close\r\n" +
               "Authorization: Basic " + unameenc + " \r\n" +
               "Content-Length: 0\r\n" +
               "\r\n");

  delay(100);
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("Connection closed");
}

