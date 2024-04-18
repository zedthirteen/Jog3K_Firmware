#include <Arduino.h>
#include <SerialTransfer.h>
#include "pico/time.h"

#include "i2c_jogger.h"

#include "buttons.h"
#include "encoders.h"

SerialTransfer packetTransfer;

uint8_t simulation_mode;
CurrentJogAxis current_jog_axis;

static void process_simulation_mode(void);
static void activate_alt_functions(void);

char arr2[] = "HITHEE";

float num = 0;

status_context_t status_context, count_context;

machine_status_packet_t prev_statuspacket = {};

machine_status_packet_t *statuspacket = (machine_status_packet_t*) status_context.mem;
machine_status_packet_t *previous_statuspacket = &prev_statuspacket;

pendant_count_packet_t prev_countpacket = {};

pendant_count_packet_t *countpacket = (pendant_count_packet_t*) count_context.mem;
pendant_count_packet_t *previous_countpacket = &prev_countpacket;

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

  previous_statuspacket->machine_state = MachineState_Other;
  simulation_mode = 0;
  current_jog_axis = X;

}



void transmit_data(void){
  // use this variable to keep track of how many
  // bytes we're stuffing in the transmit buffer
  uint16_t sendSize = 0;

  ///////////////////////////////////////// Stuff buffer with struct
  sendSize = packetTransfer.txObj(countpacket, sendSize);

  ///////////////////////////////////////// Send buffer
  packetTransfer.sendData(sendSize);
}

void receive_data(void){

  const uint32_t interval_ms = 25;
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

      //Serial1.println("receive data loop\n");
   
    if(statuspacket->machine_state == MachineState_Disconnected){
      simulation_mode = 1;
    }

    if(simulation_mode)
      process_simulation_mode();
  
  }
}

void loop() {

  //Serial1.write("read buttons\r\n");
  readButtons(statuspacket); //read Inputs
  //Serial1.write("read encoders\r\n");
  readEncoders(statuspacket); //read Encoders

  //at this point need to check the buttons to see if alternate functions are activated.
  activate_alt_functions();

  receive_data();
  //transmit_data();

  periodic_task();
}

static void activate_alt_functions(void){
  //Check the buttons to see if alternate function modes need to be set.
  if(!gpio_get(JOG_SELECT2)){
  
  }else if (!gpio_get(JOG_SELECT)){
    
  }

}

static void process_simulation_mode(void){
  //during simulation mode the status packet is updated in response to local operations.
  statuspacket->coordinate.x = countpacket->x_axis;
  statuspacket->coordinate.y = countpacket->y_axis;
  statuspacket->coordinate.z = countpacket->z_axis;
  statuspacket->coordinate.a = countpacket->a_axis;

  statuspacket->feed_override = countpacket->feed_over;
  statuspacket->spindle_override = countpacket->spindle_over;

}