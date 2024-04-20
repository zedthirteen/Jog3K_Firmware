#include <Arduino.h>

#include "i2c_jogger.h"
#include "encoders.h"

#define QUADENC                  

  #include "pio_encoder.h"
  #include "quadrature.h"
  #define QUADENCS 3  //how many Rotary Encoders do you want?
  
    // Encoders have 2 signals, which must be connected to 2 pins.

  constexpr int ENCODER1_DATA_PIN = 26;
  constexpr int ENCODER1_CLK_PIN = 25;

  constexpr int ENCODER2_DATA_PIN = 9;
  constexpr int ENCODER2_CLK_PIN = 8;

    //Sanguino	        2, 10, 11	                  0

  //First encoder uses PIO

PioEncoder Encoder0(22);      //A,B Pin Encoder 0 is PIO based
Quadrature_encoder<ENCODER1_DATA_PIN, ENCODER1_CLK_PIN> Encoder1 = Quadrature_encoder<ENCODER1_DATA_PIN, ENCODER1_CLK_PIN>();
Quadrature_encoder<ENCODER2_DATA_PIN, ENCODER2_CLK_PIN> Encoder2 = Quadrature_encoder<ENCODER2_DATA_PIN, ENCODER2_CLK_PIN>();
//PioEncoder Encoder1(25);    //A,B Pin
//PioEncoder Encoder2(8);
//PioEncoder Encoder1(25);    //A,B Pin
//PioEncoder Encoder2(8);
//Encoder Encoder3(A,B);
//Encoder Encoder4(A,B);                      
  const int QuadEncSig[] = {2,2,2};   //define wich kind of Signal you want to generate. 
                                  //1= send up or down signal (typical use for selecting modes in hal)
                                  //2= send position signal (typical use for MPG wheel)
  const int QuadEncMp[] = {4,2,2};   //some Rotary encoders send multiple Electronical Impulses per mechanical pulse. How many Electrical impulses are send for each mechanical Latch?            

long EncCount[QuadEncs];
long prev_EncCount[QuadEncs];

void init_encoders(machine_status_packet_t *statuspacket, pendant_count_packet_t *countpacket){

  for (uint8_t i = 0; i < QUADENCS; i++){
    prev_EncCount[i] = 0;
  }

  countpacket->feed_over = 100;
  countpacket->spindle_over = 100;

  #ifdef QUADENC
if(QuadEncs>=1){
  #if QUADENCS >= 1
    Encoder0.begin();
    Encoder0.flip(1);
  #endif
}
if(QuadEncs>=2){
  #if QUADENCS >= 2
    Encoder1.begin(pull_direction::up, resolution::full);
  #endif
}
if(QuadEncs>=3){
  #if QUADENCS >= 3
    Encoder2.begin(pull_direction::up, resolution::full);
  #endif
}
if(QuadEncs>=4){
  #if QUADENCS >= 4
    Encoder3.begin();
  #endif
}
#endif
}

void readEncoders(machine_status_packet_t *statuspacket, pendant_count_packet_t *countpacket, uint8_t function){

  float i;

    if(function == 1){
      for (uint8_t j = 0; j < QUADENCS; j++){
        prev_EncCount[j] = EncCount[j];
      }

      EncCount[0] = Encoder0.getCount()/QuadEncMp[0];
      EncCount[1] = Encoder1.getCount()/QuadEncMp[1];
      EncCount[2] = Encoder2.getCount()/QuadEncMp[2];           
      //adjust current axis with main encoder
      i = current_jog_axis;
      i = i + (EncCount[0]-prev_EncCount[0]);//increment the axis by the delta count

      if (i>(NUMBER_OF_AXES-1))
        i=0;
      if(i<0)
        i=(NUMBER_OF_AXES-1);
      
      previous_jog_axis = current_jog_axis;
      current_jog_axis = (CurrentJogAxis)i;

      //adjust decimal with spindle override encoder
      i = statuspacket->jog_mode.mode;
      i = i + (EncCount[1]-prev_EncCount[1]);//increment the axis by the delta count

      if (i>(JOGMODE_MAX))
        i=0;
      if(i<0)
        i=(JOGMODE_MAX);

      countpacket->jog_mode.mode=i;

      i = statuspacket->jog_mode.modifier;
      i = i + (EncCount[2]-prev_EncCount[2]);//increment the axis by the delta count

      if (i>(JOGMODIFY_MAX))
        i=0;
      if(i<0)
        i=(JOGMODIFY_MAX);

      countpacket->jog_mode.modifier=i;

    } else if (function == 2){
      for (uint8_t j = 0; j < QUADENCS; j++){
        prev_EncCount[j] = EncCount[j];
      }

      EncCount[0] = Encoder0.getCount()/QuadEncMp[0];
      EncCount[1] = Encoder1.getCount()/QuadEncMp[1];
      EncCount[2] = Encoder2.getCount()/QuadEncMp[2];

      //adjust the RPM with the main encoder
      i = statuspacket->spindle_rpm;
      i = i + (EncCount[0]-prev_EncCount[0]);//increment the axis by the delta count
      if(i<0)
        i=0;  
      countpacket->spindle_rpm=i;     

      //adjust the feed rate with the feed encoder 
      i = statuspacket->jog_stepsize;
      i = i + (EncCount[1]-prev_EncCount[1]);//increment the axis by the delta count
      if(i<0)
        i=0;
      countpacket->feedrate=i;

    } else {//function = 0

      //start by copying the data to the old count for operations that only check the delta
      for (uint8_t j = 0; j < QUADENCS; j++){
        prev_EncCount[j] = EncCount[j];
      }

      EncCount[0] = Encoder0.getCount()/QuadEncMp[0];
      EncCount[1] = Encoder1.getCount()/QuadEncMp[1];
      EncCount[2] = Encoder2.getCount()/QuadEncMp[2];

      switch(current_jog_axis){
        case X :
          countpacket->x_axis = countpacket->x_axis + (EncCount[0]-prev_EncCount[0]);//increment the axis by the delta count
        break;
        case Y :
          countpacket->y_axis = countpacket->y_axis + (EncCount[0]-prev_EncCount[0]);//increment the axis by the delta count
        break;
        case Z :
          countpacket->z_axis = countpacket->z_axis + (EncCount[0]-prev_EncCount[0]);//increment the axis by the delta count
        break;
        case A :
          countpacket->a_axis = countpacket->a_axis + (EncCount[0]-prev_EncCount[0]);//increment the axis by the delta count
        break;
        default :
          //something wrong, do nothing with the count.
        break;                        
      }

      countpacket->feed_over = countpacket->feed_over + (EncCount[1]-prev_EncCount[1]);//increment the override by the delta count
      countpacket->spindle_over = countpacket->spindle_over + (EncCount[2]-prev_EncCount[2]);//increment the override by the delta count
    }
}