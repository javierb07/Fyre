#include "KickJrPPG.h"

KickJrPPG ppg;


void setup()
{
  Serial.begin(9600);
  ppg.begin();

  //turn on LED
}

void loop()
{
  //start sampling PPG circuit, etc.
}
