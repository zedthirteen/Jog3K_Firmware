#include <stdio.h>
#include "pico/stdlib.h"
#include <SPI.h>
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"

#include "pico/stdio.h"
#include "pico/time.h"
#include <tusb.h>
#include <Adafruit_NeoPixel.h>

#include "i2c_jogger.h"

void readButtons(){
#if 0
void readKeypad(void){
  //detect if Button is Pressed
  for (int col = 0; col < numCols; col++) {
    pinMode(colPins[col], OUTPUT);
    digitalWrite(colPins[col], LOW);
    // Read the state of the row pins
    for (int row = 0; row < numRows; row++) {
      pinMode(rowPins[row], INPUT_PULLUP);
      if (digitalRead(rowPins[row]) == LOW && lastKey != keys[row][col]) {
        // A button has been pressed
        sendData('M',keys[row][col],1);
        lastKey = keys[row][col];
        row = numRows;

      }
      if (digitalRead(rowPins[row]) == HIGH && lastKey == keys[row][col]) {
        // The Last Button has been unpressed
        sendData('M',keys[row][col],0);
        lastKey = -1; //reset Key pressed
        row = numRows;
      }
    }
    
    // Set the column pin back to input mode
    pinMode(colPins[col], INPUT);
  }

}
#endif

}
