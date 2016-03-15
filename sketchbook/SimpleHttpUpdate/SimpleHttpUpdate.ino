/**
 * httpUpdate.ino
 *
 *  Created on: 27.11.2015
 *
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#define USE_SERIAL Serial
const char* ssid = "YOUR_SSID_HERE";
const char* password = "YOUR_PASSPHRASE_HERE";


String MAC_Address = "";
#define DATA_POS_MAGIC          0
#define EEPROM_MAGIC_BYTE    0xEA
#define DATA_POS_REV            1

//
// check whether first byte in EEPROM is "magic"
//
bool eeIsValid()
{
  bool retVal = true;
  char magicByte;

  if( (magicByte = EEPROM.read( DATA_POS_MAGIC )) !=  EEPROM_MAGIC_BYTE )
  {
    retVal = false;
    Serial.print("wrong magic: ");
    Serial.println( magicByte );
  }

  return(retVal);
}

//
// place a "magic" to the first byte in EEPROM
//
bool eeValidate()
{
  bool retVal = true;
  EEPROM.write( DATA_POS_MAGIC, EEPROM_MAGIC_BYTE );
  EEPROM.commit();

  return(retVal);
}


void setup() {

    uint8_t MAC_Bytes[6];

    USE_SERIAL.begin(115200);
    // USE_SERIAL.setDebugOutput(true);

    WiFi.macAddress(MAC_Bytes);
    for (int i = 0; i < sizeof(MAC_Bytes); ++i)
    {
      MAC_Address += String( MAC_Bytes[i], HEX );
      if( i+1 < sizeof(MAC_Bytes) )
      {
        MAC_Address += ":";
      }
    }
    USE_SERIAL.println();

Serial.print("MAC is: ");
Serial.println( MAC_Address );

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

}

void loop() 
{
  static bool updChkDone = false;
  uint8_t rev;
    
    // wait for WiFi connection
    if(WiFi.waitForConnectResult() == WL_CONNECTED) 
    {
      
      if( !updChkDone )
      {
        EEPROM.begin(16);

        if( eeIsValid() )
        {
          rev = EEPROM.read(DATA_POS_REV);
        }
        else
        {
          rev = 1;
        }

        rev++;
        String UpdFile = "http://192.168.1.109/" + MAC_Address + "-" + String(rev, HEX) + ".bin";
        updChkDone = true;

USE_SERIAL.println();
USE_SERIAL.print("Update file is: ");
USE_SERIAL.println( UpdFile );
USE_SERIAL.println();

        t_httpUpdate_return ret = ESPhttpUpdate.update(UpdFile.c_str());
        //t_httpUpdate_return  ret = ESPhttpUpdate.update("https://server/file.bin");

        switch(ret) {
            case HTTP_UPDATE_FAILED:
                USE_SERIAL.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                if( !eeIsValid() )
                {
                  rev = 0;
                  EEPROM.write(DATA_POS_REV, rev);
                  eeValidate();
                  EEPROM.commit();
                  USE_SERIAL.print("revision: ");
                  USE_SERIAL.print(rev, HEX);
                  USE_SERIAL.println(" stored to EEPROM");
                }
                break;

            case HTTP_UPDATE_NO_UPDATES:
                USE_SERIAL.println("HTTP_UPDATE_NO_UPDATES");
                if( !eeIsValid() )
                {
                  rev = 0;
                  EEPROM.write(DATA_POS_REV, rev);
                  eeValidate();
                  EEPROM.commit();
                  USE_SERIAL.print("revision: ");
                  USE_SERIAL.print(rev, HEX);
                  USE_SERIAL.println(" stored to EEPROM");
                }
                break;

            case HTTP_UPDATE_OK:
                USE_SERIAL.println("HTTP_UPDATE_OK");
                EEPROM.write(DATA_POS_REV, rev);
                eeValidate();
                EEPROM.commit();
                USE_SERIAL.print("revision: ");
                USE_SERIAL.print(rev, HEX);
                USE_SERIAL.println(" stored to EEPROM");
                updChkDone = false;
                break;
        }
      }
    }
}

