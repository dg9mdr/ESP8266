// #include "WProgram.h"
#include "ioStreams.h"


ioStreams::ioStreams()
{
 _hardSerial = NULL;
 _softSerial = NULL;
 
}

void ioStreams::begin(HardwareSerial *serIn)
{
 _hardSerial = serIn;
 _hardSerial->begin(38400);
 _hardSerial->println("Ready to Rip!");
}

void ioStreams::begin(SoftwareSerial *serIn)
{
 _softSerial = serIn;
 _softSerial->begin(38400);
 _softSerial->println("Ready to Rip!");
}

