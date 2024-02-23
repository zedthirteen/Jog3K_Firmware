Note that the Adafruit Neopixel library defaults to claiming PIO0.  This is hard-coded and conflicts with the encoder library.  Must change Adafruit_NeoPixel.h manually.

PIO pio = pio1;