#include <Arduino.h>
#include <SerialTransfer.h>
#include "pico/time.h"

#include "i2c_jogger.h"

#include "buttons.h"
#include "encoders.h"

SerialTransfer myTransfer;

struct __attribute__((packed)) STRUCT {
  char z;
  float y;
} testStruct;

char arr[6];

struct __attribute__((packed)) STRUCT2 {
  char z;
  float y;
} testStruct2;

char arr2[] = "HITHEE";

static struct
{
    uint8_t mem[256];
    uint8_t mem_address;
    bool mem_address_written;
} context;

Machine_status_packet prev_packet = {};

Machine_status_packet *packet = (Machine_status_packet*) context.mem;
Machine_status_packet *previous_packet = &prev_packet;

void transmit_data(void){
  // use this variable to keep track of how many
  // bytes we're stuffing in the transmit buffer
  uint16_t sendSize = 0;

  ///////////////////////////////////////// Stuff buffer with struct
  sendSize = myTransfer.txObj(testStruct2, sendSize);

  ///////////////////////////////////////// Stuff buffer with array
  sendSize = myTransfer.txObj(arr2, sendSize);

  ///////////////////////////////////////// Send buffer
  myTransfer.sendData(sendSize);
}

void periodic_task(void)
{
  const uint32_t interval_ms = 250;
  static uint32_t start_ms = 0;
  static unsigned long mils = 0;

  static bool led_state = false;

  // Blink every interval ms
  mils=millis();
  if ( (mils - start_ms) < interval_ms) return; // not enough time
  start_ms += interval_ms;

  readEncoders(); //read Encoders & send data
  //Serial1.write("update screen\r\n");
  draw_main_screen(previous_packet, packet, 1);
  //Serial1.write("update pixels\r\n");
  update_neopixels(previous_packet, packet);
  prev_packet = *packet;

  transmit_data();
}

void setup() {
  //Setup Serial
  Serial1.setTX(0);
  Serial1.begin(115200);
  Serial.begin(115200);
  myTransfer.begin(Serial);
  //while (!Serial){}

  testStruct2.z = '&';
  testStruct2.y = 62.5;

  delay(1000);

  init_screen();
  init_encoders();
  init_neopixels();
  init_buttons();

  Serial1.println("Jog3K serial debug");
  //Serial.println("Jog3K serial debug");
}

void receive_data(void){
  if(myTransfer.available())
  {
    // use this variable to keep track of how many
    // bytes we've processed from the receive buffer
    uint16_t recSize = 0;

    recSize = myTransfer.rxObj(testStruct, recSize);
    Serial1.print(testStruct.z);
    Serial1.print(testStruct.y);
    Serial1.print(" | ");

    recSize = myTransfer.rxObj(arr, recSize);
    Serial1.println(arr);
  }
}

void loop() {

  //Serial1.write("read buttons\r\n");
  readButtons(); //read Inputs & send data
  //Serial1.write("read encoders\r\n");
  //readEncoders(); //read Encoders & send data

  receive_data();

  periodic_task();
}