//
// ************************************************************************
// Use an ESP8266 as a remote sensor node.
// (C) 2016 Dirk Schanz aka dreamshader
//
// functional description:
// Goal is to add more functionality, sensors and communication
// protocols step by step.
//
// ************************************************************************
//
// At this point a "thank you very much" to all the authors sharing
// their work, knowledge and all the very useful stuff.
// You did a great job!
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
//         Join an existing WLAN, start a webserver, read temperature
//         from a DS18B20 and show it on a "local webpage" ...
// update:
//
// -----------------------------------------------------------------------
//

#include "ESP8266_Config.h"


#include <EEPROM.h>

#define EEPROM_SIZE   4096
#define LEN_TRAILING_LENGTH   2
#define EEPROM_MAGIC_BYTE  0xe7


//
// ----------------------- EEPROM layout ---------------------------------
#define DATA_POS_MAGIC                 0
#define DATA_LEN_MAGIC                 1
#define DATA_POS_CRC32                 1
#define DATA_LEN_CRC32                 4
#define DATA_HDR_END                   5
// data area begins here
//
// ------- data to join WLAN --------
//
#define DATA_POS_SSID                  DATA_HDR_END
#define DATA_LEN_SSID                  (LEN_TRAILING_LENGTH + LEN_SSID_MAX)
#define DATA_POS_PASSWORD              (DATA_POS_SSID + DATA_LEN_SSID)
#define DATA_LEN_PASSWORD              (LEN_TRAILING_LENGTH + LEN_PASSWORD_MAX)
#define DATA_POS_NODE_ID               (DATA_POS_PASSWORD + DATA_LEN_PASSWORD)
#define DATA_LEN_NODE_ID               (LEN_TRAILING_LENGTH + LEN_NODE_ID_MAX)
#define DATA_POS_NODE_NAME             (DATA_POS_NODE_ID + DATA_LEN_NODE_ID)
#define DATA_LEN_NODE_NAME             (LEN_TRAILING_LENGTH + LEN_NODE_NAME_MAX)
#define DATA_POS_NODE_IP               (DATA_POS_NODE_NAME+ DATA_LEN_NODE_NAME)
#define DATA_LEN_NODE_IP               (LEN_TRAILING_LENGTH + LEN_NODE_IP_MAX)
#define DATA_POS_NODE_COMMENT          (DATA_POS_NODE_IP + DATA_LEN_NODE_IP)
#define DATA_LEN_NODE_COMMENT          (LEN_TRAILING_LENGTH + LEN_NODE_COMMENT_MAX)

#define DATA_POS_BEGIN_PARAM           (DATA_POS_NODE_COMMENT + DATA_LEN_NODE_COMMENT)

//
// ----- data to access EMONCMS -----
//
#define DATA_POS_EMONCMS_READAPI_KEY   DATA_POS_BEGIN_PARAM
#define DATA_LEN_EMONCMS_READAPI_KEY   (LEN_TRAILING_LENGTH + LEN_EMONCMS_READAPI_KEY_MAX)
#define DATA_POS_EMONCMS_WRITEAPI_KEY  (DATA_POS_EMONCMS_READAPI_KEY + DATA_LEN_EMONCMS_READAPI_KEY)
#define DATA_LEN_EMONCMS_WRITEAPI_KEY  (LEN_TRAILING_LENGTH + LEN_EMONCMS_WRITEAPI_KEY_MAX)
#define DATA_POS_EMONCMS_HOST          (DATA_POS_EMONCMS_WRITEAPI_KEY + DATA_LEN_EMONCMS_WRITEAPI_KEY)
#define DATA_LEN_EMONCMS_HOST          (LEN_TRAILING_LENGTH + LEN_EMONCMS_HOST_MAX)
#define DATA_POS_EMONCMS_PORT          (DATA_POS_EMONCMS_HOST + DATA_LEN_EMONCMS_HOST)
#define DATA_LEN_EMONCMS_PORT          (LEN_TRAILING_LENGTH + LEN_EMONCMS_PORT_MAX)
#define DATA_POS_EMONCMS_URL           (DATA_POS_EMONCMS_PORT + DATA_LEN_EMONCMS_PORT)
#define DATA_LEN_EMONCMS_URL           (LEN_TRAILING_LENGTH + LEN_EMONCMS_URL_MAX)
#define DATA_POS_EMONCMS_FEED_FMT      (DATA_POS_EMONCMS_URL + DATA_LEN_EMONCMS_URL)
#define DATA_LEN_EMONCMS_FEED_FMT      (LEN_TRAILING_LENGTH + LEN_EMONCMS_FEED_FMT_MAX)
//
// ----- data to access SHC -----
//
#define DATA_POS_SHC_READAPI_KEY   (DATA_POS_PASSWORD + DATA_LEN_PASSWORD)
#define DATA_LEN_SHC_READAPI_KEY   (LEN_TRAILING_LENGTH + LEN_SHC_READAPI_KEY_MAX)
#define DATA_POS_SHC_WRITEAPI_KEY  (DATA_POS_SHC_READAPI_KEY + DATA_LEN_SHC_READAPI_KEY)
#define DATA_LEN_SHC_WRITEAPI_KEY  (LEN_TRAILING_LENGTH + LEN_SHC_WRITEAPI_KEY_MAX)
#define DATA_POS_SHC_HOST          (DATA_POS_SHC_WRITEAPI_KEY + DATA_LEN_SHC_WRITEAPI_KEY)
#define DATA_LEN_SHC_HOST          (LEN_TRAILING_LENGTH + LEN_SHC_HOST_MAX)
#define DATA_POS_SHC_PORT          (DATA_POS_SHC_HOST + DATA_LEN_SHC_HOST)
#define DATA_LEN_SHC_PORT          (LEN_TRAILING_LENGTH + LEN_SHC_PORT_MAX)
#define DATA_POS_SHC_URL           (DATA_POS_SHC_PORT + DATA_LEN_SHC_PORT)
#define DATA_LEN_SHC_URL           (LEN_TRAILING_LENGTH + LEN_SHC_URL_MAX)
#define DATA_POS_SHC_FEED_FMT      (DATA_POS_SHC_URL + DATA_LEN_SHC_URL)
#define DATA_LEN_SHC_FEED_FMT      (LEN_TRAILING_LENGTH + LEN_SHC_FEED_FMT_MAX)
//
// ----- table of DS18B20 sensors -----
//
#define MAX_DS18B20_ENTRIES           10
//
#define DATA_POS_DS18B20_TBL_BEGIN (DATA_POS_SHC_FEED_FMT + DATA_LEN_SHC_FEED_FMT)
//

#define DATA_LEN_NUM_TBL_ENTRIES   LEN_DS18B20_NUM_ENTRIES
#define DATA_POS_DS18B20_FIRST_REC (DATA_POS_DS18B20_TBL_BEGIN + DATA_LEN_NUM_TBL_ENTRIES)
//
#define DATA_LEN_VALIDATION_FLAG   LEN_DS18B20_VAL_FLAG
#define DATA_LEN_SENSOR_ID         (LEN_TRAILING_LENGTH + LEN_DS18B20_ID_MAX)
#define DATA_LEN_SENSOR_FEEDER_ID  (LEN_TRAILING_LENGTH + LEN_DS18B20_FEEDER_ID_MAX)
#define DATA_LEN_SENSOR_NAME       (LEN_TRAILING_LENGTH + LEN_DS18B20_NAME_MAX)
//
#define LEN_DS18B20_SENSOR_RECORD  (DATA_LEN_VALIDATION_FLAG + DATA_LEN_SENSOR_ID + DATA_LEN_SENSOR_FEEDER_ID + DATA_LEN_SENSOR_NAME)
//
#define LEN_DS18B20_TABLE_SIZE     ((LEN_DS18B20_SENSOR_RECORD * MAX_DS18B20_ENTRIES) + DATA_LEN_NUM_TBL_ENTRIES)
//
#define DATA_POS_DS18B20_TBL_END   (DATA_POS_DS18B20_TBL_BEGIN + LEN_DS18B20_TABLE_SIZE)
//
// ---- special marker to make life easier
#define DATA_ENDPOS_EEPROM             DATA_POS_DS18B20_TBL_END

