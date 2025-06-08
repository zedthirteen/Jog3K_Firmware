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

status_context_t status_context, prev_count_context, count_context = {};

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

  static float num = 0;

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

  countpacket->uptime = (uint32_t) (millis() / 250);
}


void setup() {
  //Setup Serial
  Serial1.setTX(0);
  Serial1.begin(115200);
  Serial.begin(115200);
  packetTransfer.begin(Serial, true, Serial1, 50);
  //while (!Serial){}

  delay(250);

  init_screen();
  init_encoders(statuspacket, countpacket);
  init_neopixels();
  init_buttons();

  Serial1.println("Jog3K serial debug");
  //Serial.println("Jog3K serial debug");

    //copy data into the output buffer - DJF Note: this is just clearing the first 45 bytes of the context memory
  for (size_t i = 0; i < sizeof(pendant_count_packet_t); i++) {
    count_context.mem[i] = 0;
  }

  previous_statuspacket->machine_state = MachineState_Other;
  simulation_mode = 0;
  current_jog_axis = X;

}

void transmit_data(void){
  // use this variable to keep track of how many
  // bytes we're stuffing in the transmit buffer

  uint8_t strbuf[1024];
  bool send_data_now;

  const uint32_t interval_ms = 25;
  static uint32_t start_ms = 0;
  static unsigned long mils = 0;

  #if 0

  countpacket->x_axis = num * 0.1;
  countpacket->y_axis = num * 10;
  countpacket->z_axis = num * -1;

  num = num + 0.001;
  if (num > 9999)
    num = 0;
  //Serial1.print("\033c");

  //Serial1.println("Size for TX?\n");
  //Serial1.println(sizeof(machine_status_packet_t), DEC);
  #endif

  mils=millis();
  if ( (mils - start_ms) < interval_ms) return; // not enough time
  
  

  //copy data into the output buffer, only send it if there is a change
  // 20250327 - DJF Note: I don't trust this logic so will write out long hand
                                                                                                                                                                                                                                            
  //
  send_data_now = memcmp(count_context.mem + sizeof(uint32_t), prev_count_context.mem + sizeof(uint32_t), sizeof(pendant_count_packet_t) -  + sizeof(uint32_t)) != 0;
  {
    memcpy(prev_count_context.mem, count_context.mem, sizeof(pendant_count_packet_t));
    //for (size_t i = 0; i < sizeof(pendant_count_packet_t); i++) {
    //  strbuf[i] = count_context.mem[i];
    //}

    if(Serial.availableForWrite() && (send_data_now == true)){
      #if 0
      //Serial1.print("\033c");

      Serial1.print("Serial.availableForWrite()   : ");
      Serial1.println(Serial.availableForWrite(), DEC);

      Serial1.print("uptime   : ");
      Serial1.println(countpacket->uptime, DEC);

      Serial1.print("jog_mode   : ");
      Serial1.println(countpacket->jog_mode.value, DEC);

      Serial1.print("feed_over   : ");
      Serial1.println(countpacket->feed_over, DEC);

      Serial1.print("spindle_over: ");
      Serial1.println(countpacket->spindle_over, DEC);

      Serial1.print("rapid_over: ");
      Serial1.println(countpacket->rapid_over, DEC);

      Serial1.print("buttons: ");
      Serial1.println(countpacket->buttons, DEC);

      Serial1.print("X Axis: ");
      Serial1.println(countpacket->x_axis, DEC);

      Serial1.println("Size?\n");
      Serial1.println(sizeof(pendant_count_packet_t), DEC);
      #endif    

      uint16_t sendSize = 0;
                  
      ///////////////////////////////////////// Stuff buffer with struct
      packetTransfer.reset();

      sendSize = packetTransfer.txObj(prev_count_context.mem, sendSize, sizeof(pendant_count_packet_t));

      ///////////////////////////////////////// Send buffer
      packetTransfer.sendData(sendSize);
    }
  }
  start_ms += interval_ms;//only update if successfully transmitted
}

void receive_data(void){

  const uint32_t interval_ms = 15;
  static uint32_t start_ms = 0;
  static unsigned long mils = 0;
  uint8_t strbuf[1024];

  if (Serial.available()) {

    if(packetTransfer.available())
    {
      // use this variable to keep track of how many
      // bytes we've processed from the receive buffer
      uint16_t recSize = 0;

      //Serial1.println("receive data\n");
      //Serial1.println(sizeof(machine_status_packet_t), DEC);

      recSize = packetTransfer.rxObj(strbuf, recSize );

      #if 0
      Serial1.print("\033c");
      Serial1.println("statuspacket_size");
      Serial1.println(sizeof(machine_status_packet_t), DEC);

      Serial1.println("receive data");
      Serial1.println(recSize, DEC);

      for (size_t i = 0; i < recSize; i++) {
        // Print each byte as a two-digit hexadecimal number
        if (strbuf[i] < 16) {
          Serial1.print("0"); // Print leading zero for single digit numbers
        }
        Serial1.print(strbuf[i], HEX); // Print byte in hexadecimal format
        Serial1.print(" "); // Print space between bytes
        
        // Insert a line break after every 16 bytes for readability
        if ((i + 1) % 16 == 0) {
          Serial1.println();
        }
      }
      // Print a final line break if necessary
      if (recSize % 16 != 0) {
        Serial1.println();
      }
      #endif

      //copy data into the statuspackate
      for (size_t i = 0; i < sizeof(machine_status_packet_t); i++) {
        status_context.mem[i] = strbuf[i];
      }
      packetTransfer.reset();
    }
  } else{

      mils=millis();
      if ( (mils - start_ms) < interval_ms) return; // not enough time
      start_ms += interval_ms;

      //Serial1.println("receive data loop\n");
   
    if(statuspacket->machine_state == MachineState_Disconnected){
      //simulation_mode = 1;
    }

    //if(simulation_mode)
      //process_simulation_mode();
  
  }
}

void loop() {  

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
      //at this point need to check the see if alternate functions are activated.
      if(!gpio_get(JOG_SELECT)){
        //set the screenmode to reflect the SHIFT state
        screenmode = JOG_MODIFY;
        readEncoders(statuspacket, countpacket, 1); //read Encoders
        //draw_main_screen(previous_statuspacket, statuspacket);
      
      }else if (!gpio_get(JOG_SELECT2)){
          screenmode = JOG_MODIFY;
          readEncoders(statuspacket, countpacket, 2); //read Encoders to set spindle RPM and feed rate/stepsize
          //draw_main_screen(previous_statuspacket, statuspacket);
      } else if (!gpio_get(JOG_SELECT2) && !gpio_get(JOG_SELECT)){
        //in future would like to use this to select a macro from the SD card.
      } else{
        //encoder counts should be updated
        readEncoders(statuspacket, countpacket, 0); //read Encoders
      }
    break;//close default screenmode          
  }//close switch statement
  
  readButtons(); //read Inputs   
  
  //Serial1.write("read encoders\r\n");
  transmit_data();
  //Serial.flush();
  receive_data();  
  periodic_task();
}

static void process_simulation_mode(void){
 
  static float fast_stepsize = 15000;
  static float slow_stepsize = 1500;
  static float step_stepsize = 1;

  
  //Serial1.println("jog_mode: ");
  //Serial1.println(statuspacket->jog_mode.value, HEX);

  //during simulation mode the status packet is updated in response to local operations.
  //statuspacket->coordinate.x = countpacket->x_axis;
  //statuspacket->coordinate.y = countpacket->y_axis;
  //statuspacket->coordinate.z = countpacket->z_axis;
  //statuspacket->coordinate.a = countpacket->a_axis;

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

  //if(previous_countpacket->feedrate != countpacket->feedrate){
    statuspacket->jog_stepsize = statuspacket->jog_stepsize+countpacket->feedrate;
  //}  

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

  switch (statuspacket->jog_mode.mode){
    case JOGMODE_FAST :
      statuspacket->coordinate.x = countpacket->x_axis*10;
      statuspacket->coordinate.y = countpacket->y_axis*10;
      statuspacket->coordinate.z = countpacket->z_axis*10;
      statuspacket->coordinate.a = countpacket->a_axis*10;      
    break;
    case JOGMODE_SLOW :
      statuspacket->coordinate.x = countpacket->x_axis;
      statuspacket->coordinate.y = countpacket->y_axis;
      statuspacket->coordinate.z = countpacket->z_axis;
      statuspacket->coordinate.a = countpacket->a_axis;         
    break;
    case JOGMODE_STEP :
      statuspacket->coordinate.x = countpacket->x_axis*statuspacket->jog_stepsize;
      statuspacket->coordinate.y = countpacket->y_axis*statuspacket->jog_stepsize;
      statuspacket->coordinate.z = countpacket->z_axis*statuspacket->jog_stepsize;
      statuspacket->coordinate.a = countpacket->a_axis*statuspacket->jog_stepsize;         
    break;        
  }
  
  if(previous_countpacket->spindle_rpm != countpacket->spindle_rpm)
    statuspacket->spindle_rpm = countpacket->spindle_rpm;

  *previous_countpacket = *countpacket;

  //buttons just set the state directly.  Jog buttons set jogging state.  Run, hold halt set their states (halt sets alarm) etc.
  //pressing alt-spindle sets tool change state.
}