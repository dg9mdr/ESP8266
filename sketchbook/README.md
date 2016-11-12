## A brief description of the sketches in this folder:

### ESP8266_18TFT:
shows, how to connect a 1.8" TFT display (ST7735 controller) to an ESP8266-12 using a SPI connection

### ESP8266_22TFT:
this shows, how to connect a 2.2" TFT display (ILI9340 controller) to an ESP8266-12 using a SPI connection

### ESP8266_22TFT_DUMMY:
is a template for a 2.2" TFT (ILI9340 controller) ESP8266-12 system monitoring program

### ESP8266_22TFT_NET:
is a monitoring program, that displays several system parameters of a host on a 2.2" TFT (ILI9340 controller). The system parameters are sent from the host (a Raspberry Pi in my case) to an ESP8266-12-Modul via WLAN.

### ESP8266_Messenger_V1:
In the subfolder ESP8266_Messenger_V1 is a work in progress project. It's delayed for a couple of month because of lack of time to spend in it.
The idea is to interchange human readable messages, e.g. alarms, reminder, information, on a small display, that is connected to an ESP-modul acting as a sensor node.

### ESP8266_SensorNode_DS18B20:
The following five folders contain a step by step description to create a universal sensor node on a ESP8266.
First step is to read temperature values of several Dallas DS18B20 Sensors unsing 1Wire-Protocol.

### ESP8266_SensorNode_DS18B20_EE:
In this step I show you how to save several essential parameters of the ESP8266 to the onchip-eeprom und restore them on every restart of the sensor-node.

### SP8266_SensorNode-Step3:
In step 3 we first add a DHT22 sensor to the ESP-modul. Then feed the values, read from the DS18B20 and DHT22 sensors, to an EMONCMS hosting system for graphical representation.

### ESP8266_SensorNode-Step4:
It's time for a code review, some cosmetic operations and explaining the code. This is done in step 4.

### ESP8266_SensorNode-Step5:
Now we add a BMP085/BMP180 barometric sensor to our sensor-node. All the data is sent to a host with EMONCMS as a data storage and graphical reporting solution.

### SimpleHttpUpdate:
A very useful feature of the ESP-moduls is the capability to update the binary sketch from a web-site and flash is on the fly. Afer a automatically reboot the new software is immediately running on the module.

### Finally, int the subfolder libraries you'll find additional libraries I wrote to make life easier.


