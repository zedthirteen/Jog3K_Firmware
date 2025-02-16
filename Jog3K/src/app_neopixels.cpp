#include <stdio.h>
#include "pico/stdlib.h"
#include <SPI.h>
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"

#include "pico/stdio.h"
#include "pico/time.h"
#include <Adafruit_NeoPixel.h>

#include "i2c_jogger.h"

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint8_t jog_color[] = {0,255,0};
uint8_t halt_color[] = {0,255,0};
uint8_t hold_color[] = {0,255,0};
uint8_t run_color[] = {0,255,0};
int32_t feed_color[] = {0,10000,0};
int32_t rpm_color[] = {0,10000,0};

void init_neopixels (void){

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(NEO_BRIGHTNESS);
  
  // 20250216 DJF - use a define here instead of magic number
  //for (int i = 0; i < 20; i++){

  for (int i = 0; i < NUMLEDS; i++){
    pixels.clear(); // Set all pixel colors to 'off'
    pixels.setPixelColor(i,pixels.Color(run_color[0], run_color[1], run_color[2]));
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(10);
  }

  pixels.clear(); // Set all pixel colors to 'off'
  pixels.setPixelColor(SYSLED,pixels.Color(run_color[0], run_color[1], run_color[2]));
  pixels.show();   // Send the updated pixel colors to the hardware.

}

void update_neopixels(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){

  //set override LEDS
  if (packet->feed_override > 100)
    feed_color[0] = ((packet->feed_override - 100) * 255)/100;
  else
    feed_color[0] = 0;
  if (packet->feed_override > 100)
    feed_color[1] = 255 - (((packet->feed_override - 100) * 255)/100);
  else
    feed_color[1] = 255- (((100-packet->feed_override) * 255)/100);
  if(packet->feed_override < 100)
    feed_color[2] = ((100 - packet->feed_override) * 255)/100;
  else
    feed_color[2] = 0;

  if (packet->spindle_override > 100)
    rpm_color[0] = ((packet->spindle_override - 100) * 255)/100;
  else
    rpm_color[0] = 0;
  if (packet->spindle_override > 100)
    rpm_color[1] = 255 - (((packet->spindle_override - 100) * 255)/100);
  else
    rpm_color[1] = 255- (((100-packet->spindle_override) * 255)/100);
  if(packet->spindle_override < 100)
    rpm_color[2] = ((100 - packet->spindle_override) * 255)/100;
  else
    rpm_color[2] = 0;    

  pixels.setPixelColor(FEEDLED,pixels.Color((uint8_t) feed_color[0], (uint8_t) feed_color[1], (uint8_t) feed_color[2]));
  // 20250216 DJF only handle old LEDs if board revision is 2
  #if BOARD_REVISION < 3
  pixels.setPixelColor(FEEDLED1,pixels.Color((uint8_t) feed_color[0], (uint8_t) feed_color[1], (uint8_t) feed_color[2]));
  #endif
  pixels.setPixelColor(SPINLED,pixels.Color(rpm_color[0], rpm_color[1], rpm_color[2]));
  // 20250216 DJF only handle old LEDs if board revision is 2
  #if BOARD_REVISION < 3
  pixels.setPixelColor(SPINLED1,pixels.Color(rpm_color[0], rpm_color[1], rpm_color[2]));
  #endif

  //set home LED
  if(packet->home_state.value)
    pixels.setPixelColor(HOMELED,pixels.Color(0, 255, 0));
  else
    pixels.setPixelColor(HOMELED,pixels.Color(200, 135, 0));

  //set spindleoff LED
  if(packet->spindle_rpm > 0)
    pixels.setPixelColor(SPINDLELED,pixels.Color(255, 75, 0));
  else
    pixels.setPixelColor(SPINDLELED,pixels.Color(75, 255, 130));

  //set Coolant LED
  if(packet->coolant_state.flood)
    pixels.setPixelColor(COOLED,pixels.Color(0, 100, 255));
  else
    pixels.setPixelColor(COOLED,pixels.Color(0, 0, 100));  

  //set Coolant LED
  if(packet->coolant_state.mist)
    pixels.setPixelColor(MISTLED,pixels.Color(0, 100, 255));
  else
    pixels.setPixelColor(MISTLED,pixels.Color(0, 0, 100));      

  //preload jog LED colors depending on speed
  switch (packet->jog_mode.mode) {
  case JOGMODE_FAST :
    jog_color[0] = 255; jog_color[1] = 0; jog_color[2] = 0; //RGB
    break;
  case JOGMODE_SLOW : 
    jog_color[0] = 0; jog_color[1] = 255; jog_color[2] = 0; //RGB      
    break;
  case JOGMODE_STEP : 
    jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 255; //RGB      
    break;
  default :
    //jog_color[0] = 255; jog_color[1] = 255; jog_color[2] = 255; //RGB
  break;      
  }//close jogmode

  switch (packet->machine_state){
    case STATE_IDLE :
    //no change to jog colors
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB   
    break; //close idle case

    case STATE_HOLD :
    //no jog during hold to jog colors
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB
      jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 0; //RGB     
    break; //close idle case

    case STATE_TOOL_CHANGE : 
    //no change to jog colors
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB   
    break;//close tool change case 

    case STATE_JOG :
        //Indicate jog in progress
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB        
      jog_color[0] = 255; jog_color[1] = 150; jog_color[2] = 0; //RGB
    break;//close jog case

    case STATE_HOMING :
        //No jogging during homing
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB        
      jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 0; //RGB    
    break;//close homing case

    case STATE_CYCLE :
      //No jogging during job
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB        
      jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 0; //RGB     

    break;//close cycle state

    case STATE_ALARM :
          //handle alarm state at bottom
      jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 0; //RGB
      run_color[0] = 255; run_color[1] = 0; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 0; hold_color[2] = 0; //RGB   
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB 

      //also override above colors for maximum alarm 
      pixels.setPixelColor(SPINDLELED,pixels.Color(255, 0, 0));
      pixels.setPixelColor(COOLED,pixels.Color(255, 0, 0));
      pixels.setPixelColor(HOMELED,pixels.Color(255, 0, 0));
      pixels.setPixelColor(SPINLED,pixels.Color(255, 0, 0));
      pixels.setPixelColor(FEEDLED,pixels.Color(255, 0, 0));
      // 20250216 DJF only handle old LEDs if board revision is 2
      #if BOARD_REVISION < 3
      pixels.setPixelColor(SPINLED1,pixels.Color(255, 0, 0));
      pixels.setPixelColor(FEEDLED1,pixels.Color(255, 0, 0));      
      #endif
    break;//close alarm state

    default :  //this is active when there is a non-interactive controller
      //run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      //hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      //halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB   

    break;//close alarm state
  }//close machine_state switch statement

  if(screenmode == JOG_MODIFY){
    //some overrides for alternate functions
    //violet = (138, 43, 226)
    //jog_color[0] = 138; jog_color[1] = 43; jog_color[2] = 226;
    //run_color[0] = 138; run_color[1] = 43; run_color[2] = 226; //RGB
    //hold_color[0] = 138; hold_color[1] = 43; hold_color[2] = 226; //RGB   
    //halt_color[0] = 0; halt_color[1] = 0; halt_color[2] = 0; //RGB 
    //pixels.setPixelColor(COOLED,pixels.Color(138, 43, 226));
    //pixels.setPixelColor(MISTLED,pixels.Color(138, 43, 226));
    //pixels.setPixelColor(HOMELED,pixels.Color(138, 43, 226));
    //pixels.setPixelColor(SPINDLELED,pixels.Color(138, 43, 226));
    pixels.setPixelColor(SYSLED,pixels.Color(138, 43, 226));
    // 20250216 DJF - Only handle SEL LEDs if board revision is 2
    #if BOARD_REVISION < 3
    pixels.setPixelColor(SELLED,pixels.Color(138, 43, 226));
    pixels.setPixelColor(SEL2LED,pixels.Color(138, 43, 226));
    #endif
  }

  //set jog LED values
  if(screenmode == JOG_MODIFY){
    pixels.setPixelColor(SYSLED,pixels.Color(138, 43, 226));
    // 20250216 DJF - Only handle old LEDs if board revision is 2
    #if BOARD_REVISION < 3
    pixels.setPixelColor(SELLED,pixels.Color(138, 43, 226));
    pixels.setPixelColor(SEL2LED,pixels.Color(138, 43, 226));
    #endif
  } else {
    pixels.setPixelColor(SYSLED,pixels.Color(run_color[0], run_color[1], run_color[2]));
    // 20250216 DJF - Only handle old LEDs if board revision is 2
    #if BOARD_REVISION < 3
    pixels.setPixelColor(SELLED,pixels.Color(0, 0, 0));
    pixels.setPixelColor(SEL2LED,pixels.Color(0, 0, 0));
    #endif
  }

  if(packet->coordinate.a==0xFFFFFFFF && screenmode == JOG_MODIFY){
    pixels.setPixelColor(RAISELED,pixels.Color(138, 43, 226));
    pixels.setPixelColor(LOWERLED,pixels.Color(138, 43, 226));
  } else {
    pixels.setPixelColor(RAISELED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
    pixels.setPixelColor(LOWERLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  }
  pixels.setPixelColor(LEFTLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(RIGHTLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(UPLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(DOWNLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));      

  pixels.setPixelColor(HALTLED,pixels.Color(halt_color[0], halt_color[1], halt_color[2]));
  pixels.setPixelColor(HOLDLED,pixels.Color(hold_color[0], hold_color[1], hold_color[2]));
  pixels.setPixelColor(RUNLED,pixels.Color(run_color[0], run_color[1], run_color[2]));

  pixels.show();
}

void activate_jogled(void){
  jog_color[0] = 255; jog_color[1] = 150; jog_color[2] = 0; //RGB
  pixels.setPixelColor(RAISELED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(LOWERLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(LEFTLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(RIGHTLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(UPLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(DOWNLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.show();
}

// Main loop - initilises system and then loops while interrupts get on with processing the data

/*
red = (255, 0, 0)
orange = (255, 165, 0)
yellow = (255, 150, 0)
green = (0, 255, 0)
blue = (0, 0, 255)
indigo = (75, 0, 130)
violet = (138, 43, 226)
off = (0, 0, 0)
*/
