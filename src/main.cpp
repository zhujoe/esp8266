#include <Arduino.h>
#include "Network.h"

Network network;

void setup()
{
  Serial.begin(115200);
  Serial.println("@boardcard is working");

  network.init();
}

void loop()
{
  Serial.println(network.getTime());
  delay(1000);
}