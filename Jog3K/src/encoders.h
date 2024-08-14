
#ifndef ENCODERS_H_
#define ENCODERS_H_
#pragma once

#define QUADENCS 3  //how many Rotary Encoders do you want?

#if QUADENCS == 1 
  const int QuadEncs = 1;  
#endif
#if QUADENCS == 2 
  const int QuadEncs = 2;  
#endif
#if QUADENCS == 3 
  const int QuadEncs = 3;  
#endif
#if QUADENCS == 4 
  const int QuadEncs = 4;  
#endif
#if QUADENCS == 5 
  const int QuadEncs = 5;  
#endif

extern long EncCount[QuadEncs];

void init_encoders(machine_status_packet_t *statuspacket, pendant_count_packet_t *countpacket);
void readEncoders (machine_status_packet_t *statuspacket, pendant_count_packet_t *countpacket, uint8_t function);

#endif
