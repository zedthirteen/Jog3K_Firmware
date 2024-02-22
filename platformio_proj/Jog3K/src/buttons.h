
#ifndef BUTTONS_H_
#define BUTTONS_H_
#pragma once

const int Buttons = 19;               //number of inputs using internal Pullup resistor. (short to ground to trigger)
int ButtonMap[] = {1,2,3,4,5,6,7,10,11,12,13,14,15,24,27,28,29};

void readButtons (void);

#endif
