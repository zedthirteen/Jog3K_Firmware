#include <Arduino.h>
#include <SerialTransfer.h>
#include "pico/time.h"

#include "i2c_jogger.h"

#include "buttons.h"
#include "encoders.h"

SerialTransfer packetTransfer;

uint8_t simulation_mode;
CurrentJogAxis previous_jog_axis, current_jog_axis;

static void process_simulation_mode(void);

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
  draw_main_screen(previous_statuspacket, statuspacket);
  //Serial1.write("update pixels\r\n");
  update_neopixels(previous_statuspacket, statuspacket);
  prev_statuspacket = *statuspacket;
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
  init_encoders(statuspacket, countpacket);
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

      //mils=millis();
      //if ( (mils - start_ms) < interval_ms) return; // not enough time
      //start_ms += interval_ms;

      //Serial1.println("receive data loop\n");
   
    if(statuspacket->machine_state == MachineState_Disconnected){
      simulation_mode = 1;
    }

    if(simulation_mode)
      process_simulation_mode();
  
  }
}

void loop() {  

  //at this point need to check the see if alternate functions are activated.
  if(!gpio_get(JOG_SELECT)){
    //set the screenmode to reflect the SHIFT state
    screenmode = JOG_MODIFY;
    //change highlight around axis.  Change LED colors

    readEncoders(statuspacket, countpacket, 1); //read Encoders
    //adjust current axis with main encoder
    //adjust decimal with spindle override encoder
    //adjust jogmode with feed override encoder
    draw_main_screen(previous_statuspacket, statuspacket);
  
  }else if (!gpio_get(JOG_SELECT2)){
      //in future would like to use this to directly adjust current feed rate and current spindle speed setting.
      screenmode = JOG_MODIFY;
      readEncoders(statuspacket, countpacket, 2); //read Encoders
  } else{
    //update the screenmode based on the reported machine state
    switch (statuspacket->machine_state){
      case MachineState_Alarm :
        screenmode = ALARM;
      break;
      case MachineState_Homing :
        screenmode = HOMING;
      break;
      default :
        screenmode = none;
      break;            
    }
    //encoder counts should be updated
    readEncoders(statuspacket, countpacket, 0); //read Encoders
    readButtons(); //read Inputs   
  }
  //Serial1.write("read encoders\r\n");
  receive_data();
  //transmit_data();
  periodic_task();
}

static void process_simulation_mode(void){

  static float fast_stepsize = 15000;
  static float slow_stepsize = 1500;
  static float step_stepsize = 1;
  
  //Serial1.println("jog_mode: ");
  //Serial1.println(statuspacket->jog_mode.value, HEX);

  //during simulation mode the status packet is updated in response to local operations.
  statuspacket->coordinate.x = countpacket->x_axis;
  statuspacket->coordinate.y = countpacket->y_axis;
  statuspacket->coordinate.z = countpacket->z_axis;
  statuspacket->coordinate.a = countpacket->a_axis;

  statuspacket->feed_override = countpacket->feed_over;
  statuspacket->spindle_override = countpacket->spindle_over;

  if(statuspacket->machine_state == MachineState_Disconnected)
    statuspacket->machine_state = MachineState_Idle;

  statuspacket->jog_mode = countpacket->jog_mode;  
  //cacluate jog stepsize
  switch (statuspacket->jog_mode.mode){
    case JOGMODE_FAST :
      statuspacket->jog_stepsize = fast_stepsize;
    break;
    case JOGMODE_SLOW :
      statuspacket->jog_stepsize = slow_stepsize;
    break;
    case JOGMODE_STEP :
      statuspacket->jog_stepsize = step_stepsize;
    break;        
  }

  switch (statuspacket->jog_mode.modifier){
    case 0 :
      statuspacket->jog_stepsize = statuspacket->jog_stepsize;
    break;
    case 1 :
      statuspacket->jog_stepsize = statuspacket->jog_stepsize/10;
    break;
    case 2 :
      statuspacket->jog_stepsize = statuspacket->jog_stepsize/100;
    break;
    case 3 :
      statuspacket->jog_stepsize = statuspacket->jog_stepsize/1000;
    break;              
  }  

  //buttons just set the state directly.  Jog buttons set jogging state.  Run, hold halt set their states (halt sets alarm) etc.
  //pressing alt-spindle sets tool change state.

}