#include <Arduino.h>
#include <SerialTransfer.h>
#include "pico/time.h"

#include "i2c_jogger.h"

#include "buttons.h"
#include "encoders.h"

SerialTransfer packetTransfer;

char arr2[] = "HITHEE";

static struct
{
    uint8_t mem[256];
    uint8_t mem_address;
    bool mem_address_written;
} status_context;

machine_status_packet_t prev_statuspacket = {};

machine_status_packet_t *statuspacket = (machine_status_packet_t*) status_context.mem;
machine_status_packet_t *previous_statuspacket = &prev_statuspacket;

static struct
{
    uint8_t mem[256];
    uint8_t mem_address;
    bool mem_address_written;
} count_context;

pendant_count_packet_t prev_countpacket = {};

pendant_count_packet_t *countpacket = (pendant_count_packet_t*) count_context.mem;
pendant_count_packet_t *previous_countpacket = &prev_countpacket;

float num = 0;

void transmit_data(void){
  // use this variable to keep track of how many
  // bytes we're stuffing in the transmit buffer
  uint16_t sendSize = 0;

  ///////////////////////////////////////// Stuff buffer with struct
  sendSize = packetTransfer.txObj(countpacket, sendSize);

  ///////////////////////////////////////// Send buffer
  packetTransfer.sendData(sendSize);
}

void periodic_task(void)
{
  const uint32_t interval_ms = 50;
  static uint32_t start_ms = 0;
  static unsigned long mils = 0;

  static bool led_state = false;

  // Blink every interval ms
  mils=millis();
  if ( (mils - start_ms) < interval_ms) return; // not enough time
  start_ms += interval_ms;

  //Serial1.write("update screen\r\n");
  draw_main_screen(previous_statuspacket, statuspacket, 1);
  //Serial1.write("update pixels\r\n");
  update_neopixels(previous_statuspacket, statuspacket);
  *previous_statuspacket = *statuspacket;
}

void setup() {
  //Setup Serial
  Serial1.setTX(0);
  Serial1.begin(115200);
  Serial.begin(115200);
  packetTransfer.begin(Serial);
  //while (!Serial){}

  delay(250);

  init_screen();
  init_encoders();
  init_neopixels();
  init_buttons();

  Serial1.println("Jog3K serial debug");
  //Serial.println("Jog3K serial debug");
}

void receive_data(void){

  const uint32_t interval_ms = 50;
  static uint32_t start_ms = 0;
  static unsigned long mils = 0;

  if(packetTransfer.available())
  {
    // use this variable to keep track of how many
    // bytes we've processed from the receive buffer
    uint16_t recSize = 0;

    recSize = packetTransfer.rxObj(statuspacket, recSize);

  } else{

      mils=millis();
      if ( (mils - start_ms) < interval_ms) return; // not enough time
      start_ms += interval_ms;

      statuspacket->coordinate.x = num * 13.23222;
      statuspacket->coordinate.y = num * -7.89;
      statuspacket->coordinate.z = num * 10;
      statuspacket->coordinate.a = num * 3;
      num += 0.1;
  }
}

void loop() {

  //Serial1.write("read buttons\r\n");
  readButtons(); //read Inputs
  //Serial1.write("read encoders\r\n");
  readEncoders(); //read Encoders

  receive_data();
  transmit_data();

  periodic_task();
}