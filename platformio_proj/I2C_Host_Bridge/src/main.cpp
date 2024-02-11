#include <Arduino.h>
#include "i2c_jogger.h"

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(22, OUTPUT);
  Serial1.setRX(13);
  Serial1.setTX(12);
  Serial1.begin(115200);

}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(22, HIGH);  // turn the LED on (HIGH is the voltage level)
  Serial1.printf("Hello there\r\n");
  delay(1000);                      // wait for a second
  digitalWrite(22, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);                      // wait for a second
}