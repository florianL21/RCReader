#include <Arduino.h>
#include <RCReader.h>

//Initialize the RCReader for Pin A8, 60ms timeout period, and a valid value range from 1300 to 1800
RCReader test(RCR_PIN_A8, 60, 1300, 1800);
uint16_t value;

void setup() 
{
  Serial.begin(115200);
}

void loop() 
{
  //Get the value from the RCReader by reference and process the return status flag with a switch case.
  //Print the according error message in case of a failure, and print the value if everything is fine.
  switch(test.getMicroseconds(&value))
  {
    case RCR_OK:
      Serial.println(value);
    break;
    case RCR_InvalidValue:
      Serial.println("Value out of bounds");
    break;
    case RCR_Timeout:
      Serial.println("RCR timed out");
    break;
  }
  //add a bit of delay to not flood the serial output
  delay(10);
}