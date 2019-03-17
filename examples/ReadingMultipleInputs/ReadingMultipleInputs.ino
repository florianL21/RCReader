#include <Arduino.h>
#include <RCReader.h>

//Initialize the RCReader for Pin A8, 60ms timeout period, and a valid value range from 1300 to 1800
//Set the last parameter to true so that the library holds the output in the last valid state
RCReader Trottle(RCR_PIN_A8, 60, 1300, 1800, true);

//Same as above, but without last parameter to showcase the difference
RCReader Pitch(RCR_PIN_A8, 60, 1300, 1800);

//create an unsigned 16 bit variable to hold our measured value
uint16_t trottleValue;

//For pitch we need a signed variable, because it returns -1 when there was an error
int16_t pitchValue;

void setup() 
{
  Serial.begin(115200);
}

void loop()
{
  
  pitchValue = Pitch.getMicroseconds();
  if(pitchValue != -1)
  {
    Serial.print(pitchValue);
  } else
  {
    Serial.print(0);
  }
  Serial.print(", ");
  Serial.println(Trottle.getMicroseconds());
  //add a bit of delay to not flood the serial output
  delay(10);
}