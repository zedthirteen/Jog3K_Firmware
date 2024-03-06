#include <Arduino.h>

#include "i2c_jogger.h"
#include "encoders.h"

#define QUADENC                  

  #include "pio_encoder.h"
  #include "quadrature.h"
  #define QUADENCS 3  //how many Rotary Encoders do you want?
  
    // Encoders have 2 signals, which must be connected to 2 pins.

  constexpr int ENCODER1_DATA_PIN = 25;
  constexpr int ENCODER1_CLK_PIN = 26;

  constexpr int ENCODER2_DATA_PIN = 8;
  constexpr int ENCODER2_CLK_PIN = 9;

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

void init_encoders(){
  #ifdef QUADENC
if(QuadEncs>=1){
  #if QUADENCS >= 1
    Encoder0.begin();
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

void readEncoders(){
    if(QuadEncs>=1){
      #if QUADENCS >= 1
        EncCount[0] = Encoder0.getCount()/QuadEncMp[0];
      #endif
    }
    if(QuadEncs>=2){
      #if QUADENCS >= 2
        EncCount[1] = Encoder1.getCount()/QuadEncMp[1];
      #endif
    }
    if(QuadEncs>=3){
      #if QUADENCS >= 3
        EncCount[2] = Encoder2.getCount()/QuadEncMp[2];
      #endif
    }
    if(QuadEncs>=4){
      #if QUADENCS >= 4
        EncCount[3] = Encoder3.getCount()/QuadEncMp[3];
      #endif
    }
    if(QuadEncs>=5){
      #if QUADENCS >= 5
        EncCount[4] = Encoder4.getCount()/QuadEncMp[4];
      #endif
    }

    #if 0
    for ( int i = 0; i<QuadEncs; i++){
      Serial1.print("encoder");
      Serial1.println(i, DEC);
      Serial1.print(": ");
      Serial1.println(EncCount[i], DEC);
    }
    #endif

}