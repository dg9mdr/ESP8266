#ifndef _IO_STREAMS_
#define _IO_STREAMS_

#include <inttypes.h>
#include <stdarg.h>
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
// #include "WProgram.h"
#include "SoftwareSerial.h"


extern "C" {
}

class ioStreams
{
 public:
   ioStreams();
   void begin(HardwareSerial *serIn); 
   void begin(SoftwareSerial *serIn);       
 private:
   HardwareSerial *_hardSerial;
   SoftwareSerial *_softSerial;
};

#endif // _IO_STREAMS_

