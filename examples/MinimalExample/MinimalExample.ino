#include <Arduino.h>
#include <RCReader.h>

//Initialize the RCReader for Pin A8
RCReader test(RCR_PIN_A8);

void setup() 
{
  Serial.begin(115200);
}

void loop()
{
  //print the data to the serial monitor
  Serial.println(test.getMicroseconds());
  //add a bit of delay to not flood the serial output
  delay(10);
}