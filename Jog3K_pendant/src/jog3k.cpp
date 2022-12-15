#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <iostream>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "Adafruit_NeoPixel.hpp"
#include "ss_oled.h"
#include "hardware/flash.h"

#include "jog3k.h"

// Variables
double xpos;
double ypos;
double zpos;
unsigned int tool;
int command_error = 0;
int jog_count_x = 0;
int jog_count_y = 0;
int jog_count_z = 0;
int jog_scale_x_slow = 1;
int jog_scale_x_medium = 10;
int jog_scale_x_fast = 100;
int jog_scale_y_slow = 1;
int jog_scale_y_medium = 10;
int jog_scale_y_fast = 100;
int jog_scale_z_slow = 1;
int jog_scale_z_medium = 10;
int jog_scale_z_fast = 100;

// Counter for LCNC watchdog
uint8_t comm_watchdog_counter = 0;

// Enums & structs
JogSpeed jog_speed = JOG_DISABLED;
ScreenMode screenmode;
ScreenMode previous_screenmode;
Jogmode current_jogmode;
Jogmode previous_jogmode;
Jogmodify current_jogmodify;
Jogmodify previous_jogmodify;
Jogger_status_packet status_packet;
Jogger_status_packet *packet = &status_packet;
Jogger_status_packet prev_packet;
Jogger_status_packet *previous_packet = &prev_packet;

// Flags
uint8_t update_leds = 0;
uint8_t erase_display = 0;
uint8_t lcnc_disconnected_ack = 0;
uint8_t lcnc_connected_ack = 0;

uint8_t halt_pressed = 0;
uint8_t hold_pressed = 0;
uint8_t cycle_start_pressed = 0;
uint8_t spindle_pressed = 0;
uint8_t mist_pressed = 0;
uint8_t flood_pressed = 0;
uint8_t home_pressed = 0;
uint8_t spindle_over_down_pressed = 0;
uint8_t spindle_over_reset_pressed = 0;
uint8_t spindle_over_up_pressed = 0;
uint8_t feed_over_down_pressed = 0;
uint8_t feed_over_reset_pressed = 0;
uint8_t feed_over_up_pressed = 0;
uint8_t jog_sel_pressed = 0;
uint8_t jog_speed_pressed = 0;
uint8_t dpad_left_pressed = 0;
uint8_t dpad_right_pressed = 0;
uint8_t dpad_up_pressed = 0;
uint8_t dpad_down_pressed = 0;
uint8_t raise_pressed = 0;
uint8_t lower_pressed = 0;

uint8_t alt_halt_pressed = 0;
uint8_t alt_hold_pressed = 0;
uint8_t alt_cycle_start_pressed = 0;
uint8_t alt_spindle_pressed = 0;
uint8_t alt_home_pressed = 0;
uint8_t alt_flood_pressed = 0;
uint8_t alt_mist_pressed = 0;
uint8_t alt_up_pressed = 0;
uint8_t alt_down_pressed = 0;
uint8_t alt_left_pressed = 0;
uint8_t alt_right_pressed = 0;
uint8_t alt_raise_pressed = 0;
uint8_t alt_lower_pressed = 0;

uint8_t lcnc_connected = 0;
uint8_t machine_on = 0;
uint8_t estop_active = 0;
uint8_t machine_homed = 0;
uint8_t program_paused = 0;
uint8_t spindle_on = 0;
uint8_t coolant_on = 0;
uint8_t mist_on = 0;
uint8_t program_running = 0;
// uint8_t jog_speed = 0;

// LEDs
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// LED colour presets
int led_red = pixels.Color(255, 0, 0);
int led_orange = pixels.Color(255, 140, 0);
int led_yellow = pixels.Color(255, 240, 0);
int led_green = pixels.Color(0, 255, 0);
int led_blue = pixels.Color(0, 0, 255);
int led_indigo = pixels.Color(149, 0, 255);
int led_violet = pixels.Color(138, 43, 226);
int led_white = pixels.Color(255, 255, 255);
int led_off = pixels.Color(0, 0, 0);

// LED colour variables
int jogled_colour = led_white;
int raiseled_colour = led_white;
int haltled_colour = led_white;
int feedoverled_colour = led_white;
int spinoverled_colour = led_white;
int coolled_colour = led_white;
int holdled_colour = led_white;
int runled_colour = led_white;
int spindleled_colour = led_white;
int homeled_colour = led_white;

// Display
int rc;
SSOLED oled;
static uint8_t ucBuffer[1024];
char buf[8];
bool screenflip = false;
uint8_t previous_state = 0;

// Flash storage
const uint8_t *flash_target_contents = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

int main()
{
    board_init();
    stdio_init_all();

    // config data from flash
    read_flash_config();

    // configure button GPIOs
    gpio_init(PIN_DPAD_UP);
    gpio_set_dir(PIN_DPAD_UP, GPIO_IN);
    gpio_set_pulls(PIN_DPAD_UP, true, false);

    gpio_init(PIN_DPAD_DOWN);
    gpio_set_dir(PIN_DPAD_DOWN, GPIO_IN);
    gpio_set_pulls(PIN_DPAD_DOWN, true, false);

    gpio_init(PIN_DPAD_LEFT);
    gpio_set_dir(PIN_DPAD_LEFT, GPIO_IN);
    gpio_set_pulls(PIN_DPAD_LEFT, true, false);

    gpio_init(PIN_DPAD_RIGHT);
    gpio_set_dir(PIN_DPAD_RIGHT, GPIO_IN);
    gpio_set_pulls(PIN_DPAD_RIGHT, true, false);

    gpio_init(PIN_BUTTON_FEEDOVR_UP);
    gpio_set_dir(PIN_BUTTON_FEEDOVR_UP, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_FEEDOVR_UP, true, false);

    gpio_init(PIN_BUTTON_HALT);
    gpio_set_dir(PIN_BUTTON_HALT, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_HALT, true, false);

    gpio_init(PIN_BUTTON_LOWER);
    gpio_set_dir(PIN_BUTTON_LOWER, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_LOWER, true, false);

    gpio_init(PIN_BUTTON_RAISE);
    gpio_set_dir(PIN_BUTTON_RAISE, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_RAISE, true, false);

    gpio_init(PIN_BUTTON_FEEDOVR_DN);
    gpio_set_dir(PIN_BUTTON_FEEDOVR_DN, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_FEEDOVR_DN, true, false);

    gpio_init(PIN_BUTTON_FEEDOVR_RST);
    gpio_set_dir(PIN_BUTTON_FEEDOVR_RST, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_FEEDOVR_RST, true, false);

    gpio_init(PIN_BUTTON_ALT);
    gpio_set_dir(PIN_BUTTON_ALT, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_ALT, true, false);

    gpio_init(PIN_BUTTON_SPINOVR_UP);
    gpio_set_dir(PIN_BUTTON_SPINOVR_UP, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_SPINOVR_UP, true, false);

    gpio_init(PIN_BUTTON_SPINOVR_DN);
    gpio_set_dir(PIN_BUTTON_SPINOVR_DN, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_SPINOVR_DN, true, false);

    gpio_init(PIN_BUTTON_FLOOD);
    gpio_set_dir(PIN_BUTTON_FLOOD, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_FLOOD, true, false);

    gpio_init(PIN_BUTTON_MIST);
    gpio_set_dir(PIN_BUTTON_MIST, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_MIST, true, false);

    gpio_init(PIN_BUTTON_FEED_HOLD);
    gpio_set_dir(PIN_BUTTON_FEED_HOLD, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_FEED_HOLD, true, false);

    gpio_init(PIN_BUTTON_CYCLE_START);
    gpio_set_dir(PIN_BUTTON_CYCLE_START, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_CYCLE_START, true, false);

    gpio_init(PIN_BUTTON_SPINDLE);
    gpio_set_dir(PIN_BUTTON_SPINDLE, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_SPINDLE, true, false);

    gpio_init(PIN_BUTTON_SPINOVR_RST);
    gpio_set_dir(PIN_BUTTON_SPINOVR_RST, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_SPINOVR_RST, true, false);

    gpio_init(PIN_BUTTON_HOME);
    gpio_set_dir(PIN_BUTTON_HOME, GPIO_IN);
    gpio_set_pulls(PIN_BUTTON_HOME, true, false);

    pixels.begin(); // INITIALIZE NeoPixel strip object
    pixels.setBrightnessFunctions(adjust, adjust, adjust, adjust);

    if (gpio_get(PIN_BUTTON_ALT))
    {
        // Fn pressed at startup. Enter settings interface.
        // settings_ui();

        // Just flips screen for now.
        screenflip = !screenflip;
        write_flash_config();
    }

    // init display
    rc = oledInit(&oled, OLED_128x64, 0x3c, screenflip, 0, 0, PIN_DISP_SDA, PIN_DISP_SCL, PIN_DISP_RESET, OLED_CLOCK);
    oledSetBackBuffer(&oled, ucBuffer);
    oledFill(&oled, 0, 1);
    oledDumpBuffer(&oled, expatria_logo);
    oledWriteString(&oled, 0, 18, 3, (char *)"Expatria", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 44, 7, (char *)"Technologies", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 88, 0, (char *)FW_VERSION, FONT_8x8, 0, 1);
    sleep_ms(1500); // Leave message on screen for a bit before moving on.
    oledFill(&oled, 0, 1);
    draw_main_screen();

    // launch USB tasks on core 1
    multicore_launch_core1(core1_main);

    while (1)
    {
        // Do stuff here on core 0
        button_task();
        ui_task();
    }

    return 0;
}

void core1_main()
{
    tusb_init();

    while (1)
    {
        // Do stuff here on core 1
        hid_task();
        cdc_task();
        tud_task(); // tinyusb device task
        comm_watchdog();
    }
}

//--------------------------------------------------------------------+
// Task declarations
//--------------------------------------------------------------------+

void button_task(void)
{
    // Poll every 5ms
    const uint32_t interval_ms = 5;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return; // not enough time; move on
    start_ms += interval_ms;

    // poll buttons
    if (gpio_get(PIN_BUTTON_HALT) && !gpio_get(PIN_BUTTON_ALT))
    {
        halt_pressed = 1;
        pixels.setPixelColor(HALTLED, led_off);
    }
    else
    {
        halt_pressed = 0;
        pixels.setPixelColor(HALTLED, haltled_colour);
    }

    if (gpio_get(PIN_BUTTON_FEED_HOLD) && !gpio_get(PIN_BUTTON_ALT))
    {
        hold_pressed = 1;
        pixels.setPixelColor(HOLDLED, led_off);
    }
    else
    {
        hold_pressed = 0;
        pixels.setPixelColor(HOLDLED, holdled_colour);
    }

    if (gpio_get(PIN_BUTTON_CYCLE_START) && !gpio_get(PIN_BUTTON_ALT))
    {
        cycle_start_pressed = 1;
        pixels.setPixelColor(RUNLED, led_off);
    }
    else
    {
        cycle_start_pressed = 0;
        pixels.setPixelColor(RUNLED, runled_colour);
    }

    if (gpio_get(PIN_BUTTON_SPINDLE) && !gpio_get(PIN_BUTTON_ALT))
    {
        spindle_pressed = 1;
        pixels.setPixelColor(SPINDLELED, led_off);
    }
    else
    {
        spindle_pressed = 0;
        pixels.setPixelColor(SPINDLELED, spindleled_colour);
    }

    if (gpio_get(PIN_BUTTON_MIST) && !gpio_get(PIN_BUTTON_ALT))
    {
        mist_pressed = 1;
    }
    else
    {
        mist_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_FLOOD) && !gpio_get(PIN_BUTTON_ALT))
    {
        flood_pressed = 1;
    }
    else
    {
        flood_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_HOME) && !gpio_get(PIN_BUTTON_ALT))
    {
        home_pressed = 1;
        pixels.setPixelColor(HOMELED, led_off);
    }
    else
    {
        home_pressed = 0;
        pixels.setPixelColor(HOMELED, homeled_colour);
    }

    if (gpio_get(PIN_BUTTON_SPINOVR_DN) && !gpio_get(PIN_BUTTON_ALT))
    {
        spindle_over_down_pressed = 1;
    }
    else
    {
        spindle_over_down_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_SPINOVR_RST) && !gpio_get(PIN_BUTTON_ALT))
    {
        spindle_over_reset_pressed = 1;
    }
    else
    {
        spindle_over_reset_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_SPINOVR_UP) && !gpio_get(PIN_BUTTON_ALT))
    {
        spindle_over_up_pressed = 1;
    }
    else
    {
        spindle_over_up_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_FEEDOVR_DN) && !gpio_get(PIN_BUTTON_ALT))
    {
        feed_over_down_pressed = 1;
    }
    else
    {
        feed_over_down_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_FEEDOVR_RST) && !gpio_get(PIN_BUTTON_ALT))
    {
        feed_over_reset_pressed = 1;
    }
    else
    {
        feed_over_reset_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_FEEDOVR_UP) && !gpio_get(PIN_BUTTON_ALT))
    {
        feed_over_up_pressed = 1;
    }
    else
    {
        feed_over_up_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_ALT))
    {
        jog_sel_pressed = 1;

        // shift functions, shift + button
        if (gpio_get(PIN_BUTTON_HALT))
        {
            alt_halt_pressed = 1; // shift-halt will be used to power on machine
        }
        else
        {
            alt_halt_pressed = 0;
        }

        if (gpio_get(PIN_BUTTON_FEED_HOLD))
        {
            alt_hold_pressed = 1; // shift-halt will be used to power on machine
        }
        else
        {
            alt_hold_pressed = 0;
        }

        if (gpio_get(PIN_BUTTON_HOME))
        {
            alt_home_pressed = 1;
        }
        else
        {
            alt_home_pressed = 0;
        }

        if (gpio_get(PIN_BUTTON_CYCLE_START))
        {
            alt_cycle_start_pressed = 1;
        }
        else
        {
            alt_cycle_start_pressed = 0;
        }

        if (gpio_get(PIN_BUTTON_SPINDLE))
        {
            alt_spindle_pressed = 1;
        }
        else
        {
            alt_spindle_pressed = 0;
        }

        if (gpio_get(PIN_BUTTON_FLOOD))
        {
            alt_flood_pressed = 1;
        }
        else
        {
            alt_flood_pressed = 0;
        }

        if (gpio_get(PIN_BUTTON_MIST))
        {
            alt_mist_pressed = 1;
        }
        else
        {
            alt_mist_pressed = 0;
        }

        if (gpio_get(PIN_DPAD_UP))
        {
            alt_up_pressed = 1;
        }
        else
        {
            alt_up_pressed = 0;
        }

        if (gpio_get(PIN_DPAD_DOWN))
        {
            alt_down_pressed = 1;
        }
        else
        {
            alt_down_pressed = 0;
        }

        if (gpio_get(PIN_DPAD_LEFT))
        {
            alt_left_pressed = 1;
        }
        else
        {
            alt_left_pressed = 0;
        }

        if (gpio_get(PIN_DPAD_RIGHT))
        {
            alt_right_pressed = 1;
        }
        else
        {
            alt_right_pressed = 0;
        }

        if (gpio_get(PIN_BUTTON_RAISE))
        {
            alt_raise_pressed = 1;
        }
        else
        {
            alt_raise_pressed = 0;
        }

        if (gpio_get(PIN_BUTTON_LOWER))
        {
            alt_lower_pressed = 1;
        }
        else
        {
            alt_lower_pressed = 0;
        }

        if (gpio_get(PIN_BUTTON_FEEDOVR_UP))
        {
            screenmode = JOG_MODIFY;
            // Increase jog speed
            if (jog_speed == JOG_DISABLED && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_001;
            }
            else if (jog_speed == JOG_STEP_001 && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_01;
            }
            else if (jog_speed == JOG_STEP_01 && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_1;
            }
            else if (jog_speed == JOG_STEP_1 && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_10;
            }
            else if (jog_speed == JOG_STEP_10 && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_100;
            }
            else if (jog_speed == JOG_STEP_100 && !jog_speed_pressed)
            {
                jog_speed = JOG_SLOW;
            }
            else if (jog_speed == JOG_SLOW && !jog_speed_pressed)
            {
                jog_speed = JOG_MEDIUM;
            }
            else if (jog_speed == JOG_MEDIUM && !jog_speed_pressed)
            {
                jog_speed = JOG_FAST;
            }
            // do nothing if jog speed is already FAST (max)
            jog_speed_pressed = 1;
        }
        else if (gpio_get(PIN_BUTTON_FEEDOVR_RST))
        {
            screenmode = JOG_MODIFY;
            // Reset jog speed to single step
            jog_speed = JOG_STEP_001;
        }
        else if (gpio_get(PIN_BUTTON_FEEDOVR_DN))
        {
            screenmode = JOG_MODIFY;
            // Decrease jog speed
            if (jog_speed == JOG_FAST && !jog_speed_pressed)
            {
                jog_speed = JOG_MEDIUM;
            }
            else if (jog_speed == JOG_MEDIUM && !jog_speed_pressed)
            {
                jog_speed = JOG_SLOW;
            }
            else if (jog_speed == JOG_SLOW && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_100;
            }
            else if (jog_speed == JOG_STEP_100 && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_1;
            }
            else if (jog_speed == JOG_STEP_10 && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_1;
            }
            else if (jog_speed == JOG_STEP_1 && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_01;
            }
            else if (jog_speed == JOG_STEP_01 && !jog_speed_pressed)
            {
                jog_speed = JOG_STEP_001;
            }
            else if (jog_speed == JOG_STEP_001 && !jog_speed_pressed)
            {
                jog_speed = JOG_DISABLED;
            }
            // do nothing if jog speed is already DISABLED (min)
            jog_speed_pressed = 1;
        }
        else
        {
            // button released
            jog_speed_pressed = 0;
        }
    }
    else
    {
        screenmode = RUN;
        jog_sel_pressed = 0;
        alt_halt_pressed = 0;
        alt_hold_pressed = 0;
        alt_home_pressed = 0;
        alt_spindle_pressed = 0;
        alt_flood_pressed = 0;
        alt_mist_pressed = 0;
        alt_cycle_start_pressed = 0;
        alt_up_pressed = 0;
        alt_down_pressed = 0;
        alt_left_pressed = 0;
        alt_right_pressed = 0;
        alt_raise_pressed = 0;
        alt_lower_pressed = 0;
    }

    if (gpio_get(PIN_DPAD_DOWN))
    {
        if (jog_speed != JOG_DISABLED)
        // Only jog when jog is enabled
        {
            if (!dpad_down_pressed)
            {
                // button was not pressed last iteration; button not held. Only step when not held.
                switch (jog_speed)
                {
                case JOG_STEP_001:
                    jog_count_y -= 1;
                    break;
                case JOG_STEP_01:
                    jog_count_y -= 10;
                    break;
                case JOG_STEP_1:
                    jog_count_y -= 100;
                    break;
                case JOG_STEP_10:
                    jog_count_y -= 1000;
                    break;
                case JOG_STEP_100:
                    jog_count_y -= 10000;
                    break;
                default:
                    break;
                }
            }
            else
            {
                // button held
                switch (jog_speed)
                {
                case JOG_SLOW:
                    jog_count_y -= jog_scale_y_slow; // simplified for now while working out config interface and flash storage; look into using hardware timer(s) once implemented.
                    break;
                case JOG_MEDIUM:
                    jog_count_y -= jog_scale_y_medium;
                    break;
                case JOG_FAST:
                    jog_count_y -= jog_scale_y_fast;
                    break;
                default:
                    break;
                }
            }
        }

        dpad_down_pressed = 1;
    }
    else
    {
        dpad_down_pressed = 0;
    }

    if (gpio_get(PIN_DPAD_UP))
    {
        if (jog_speed != JOG_DISABLED)
        // Only jog when jog is enabled
        {
            if (!dpad_up_pressed)
            {
                // button was not pressed last iteration; button not held. Only step when not held.
                switch (jog_speed)
                {
                case JOG_STEP_001:
                    jog_count_y += 1;
                    break;
                case JOG_STEP_01:
                    jog_count_y += 10;
                    break;
                case JOG_STEP_1:
                    jog_count_y += 100;
                    break;
                case JOG_STEP_10:
                    jog_count_y += 1000;
                    break;
                case JOG_STEP_100:
                    jog_count_y += 10000;
                    break;
                default:
                    break;
                }
            }
            else
            {
                // button held
                switch (jog_speed)
                {
                case JOG_SLOW:
                    jog_count_y += jog_scale_y_slow;
                    break;
                case JOG_MEDIUM:
                    jog_count_y += jog_scale_y_medium;
                    break;
                case JOG_FAST:
                    jog_count_y += jog_scale_y_fast;
                    break;
                default:
                    break;
                }
            }
        }

        dpad_up_pressed = 1;
    }
    else
    {
        dpad_up_pressed = 0;
    }

    if (gpio_get(PIN_DPAD_LEFT))
    {
        if (jog_speed != JOG_DISABLED)
        // Only jog when jog is enabled
        {
            if (!dpad_left_pressed)
            {
                // button was not pressed last iteration; button not held. Only step when not held.
                switch (jog_speed)
                {
                case JOG_STEP_001:
                    jog_count_x -= 1;
                    break;
                case JOG_STEP_01:
                    jog_count_x -= 10;
                    break;
                case JOG_STEP_1:
                    jog_count_x -= 100;
                    break;
                case JOG_STEP_10:
                    jog_count_x -= 1000;
                    break;
                case JOG_STEP_100:
                    jog_count_x -= 10000;
                    break;
                default:
                    break;
                }
            }
            else
            {
                // button held
                switch (jog_speed)
                {
                case JOG_SLOW:
                    jog_count_x -= jog_scale_y_slow;
                    break;
                case JOG_MEDIUM:
                    jog_count_x -= jog_scale_y_medium;
                    break;
                case JOG_FAST:
                    jog_count_x -= jog_scale_y_fast;
                    break;
                default:
                    break;
                }
            }
        }

        dpad_left_pressed = 1;
    }
    else
    {
        dpad_left_pressed = 0;
    }

    if (gpio_get(PIN_DPAD_RIGHT))
    {
        if (jog_speed != JOG_DISABLED)
        // Only jog when jog is enabled
        {
            if (!dpad_right_pressed)
            {
                // button was not pressed last iteration; button not held. Only step when not held.
                switch (jog_speed)
                {
                case JOG_STEP_001:
                    jog_count_x += 1;
                    break;
                case JOG_STEP_01:
                    jog_count_x += 10;
                    break;
                case JOG_STEP_1:
                    jog_count_x += 100;
                    break;
                case JOG_STEP_10:
                    jog_count_x += 1000;
                    break;
                case JOG_STEP_100:
                    jog_count_x += 10000;
                    break;
                default:
                    break;
                }
            }
            else
            {
                // button held
                switch (jog_speed)
                {
                case JOG_SLOW:
                    jog_count_x += jog_scale_y_slow;
                    break;
                case JOG_MEDIUM:
                    jog_count_x += jog_scale_y_medium;
                    break;
                case JOG_FAST:
                    jog_count_x += jog_scale_y_fast;
                    break;
                default:
                    break;
                }
            }
        }

        dpad_right_pressed = 1;
    }
    else
    {
        dpad_right_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_RAISE))
    {
        if (jog_speed != JOG_DISABLED)
        // Only jog when jog is enabled
        {
            if (!raise_pressed)
            {
                // button was not pressed last iteration; button not held. Only step when not held.
                switch (jog_speed)
                {
                case JOG_STEP_001:
                    jog_count_z += 1;
                    break;
                case JOG_STEP_01:
                    jog_count_z += 10;
                    break;
                case JOG_STEP_1:
                    jog_count_z += 100;
                    break;
                case JOG_STEP_10:
                    jog_count_z += 1000;
                    break;
                case JOG_STEP_100:
                    jog_count_z += 10000;
                    break;
                default:
                    break;
                }
            }
            else
            {
                // button held
                switch (jog_speed)
                {
                case JOG_SLOW:
                    jog_count_z += jog_scale_y_slow;
                    break;
                case JOG_MEDIUM:
                    jog_count_z += jog_scale_y_medium;
                    break;
                case JOG_FAST:
                    jog_count_z += jog_scale_y_fast;
                    break;
                default:
                    break;
                }
            }
        }

        raise_pressed = 1;
    }
    else
    {
        raise_pressed = 0;
    }

    if (gpio_get(PIN_BUTTON_LOWER))
    {
        if (jog_speed != JOG_DISABLED)
        // Only jog when jog is enabled
        {
            if (!lower_pressed)
            {
                // button was not pressed last iteration; button not held. Only step when not held.
                switch (jog_speed)
                {
                case JOG_STEP_001:
                    jog_count_z -= 1;
                    break;
                case JOG_STEP_01:
                    jog_count_z -= 10;
                    break;
                case JOG_STEP_1:
                    jog_count_z -= 100;
                    break;
                case JOG_STEP_10:
                    jog_count_z -= 1000;
                    break;
                case JOG_STEP_100:
                    jog_count_z -= 10000;
                    break;
                default:
                    break;
                }
            }
            else
            {
                // button held
                switch (jog_speed)
                {
                case JOG_SLOW:
                    jog_count_z -= jog_scale_y_slow;
                    break;
                case JOG_MEDIUM:
                    jog_count_z -= jog_scale_y_medium;
                    break;
                case JOG_FAST:
                    jog_count_z -= jog_scale_y_fast;
                    break;
                default:
                    break;
                }
            }
        }

        lower_pressed = 1;
    }
    else
    {
        lower_pressed = 0;
    }

    // Buttons that share an LED
    if (raise_pressed || lower_pressed)
    {
        pixels.setPixelColor(RAISELED, led_off);
    }
    else
    {
        pixels.setPixelColor(RAISELED, raiseled_colour);
    }

    if (mist_pressed || flood_pressed)
    {
        pixels.setPixelColor(COOLLED, led_off);
    }
    else
    {
        pixels.setPixelColor(COOLLED, coolled_colour);
    }

    if (dpad_left_pressed || dpad_right_pressed || dpad_up_pressed || dpad_down_pressed)
    {
        pixels.setPixelColor(JOGLED, led_off);
    }
    else
    {
        pixels.setPixelColor(JOGLED, jogled_colour);
    }

    if (spindle_over_down_pressed || spindle_over_reset_pressed || spindle_over_up_pressed)
    {
        pixels.setPixelColor(SPINOVERLED, led_off);
    }
    else
    {
        pixels.setPixelColor(SPINOVERLED, spinoverled_colour);
    }

    if (feed_over_down_pressed || feed_over_reset_pressed || feed_over_up_pressed)
    {
        pixels.setPixelColor(FEEDOVERLED, led_off);
    }
    else
    {
        pixels.setPixelColor(FEEDOVERLED, feedoverled_colour);
    }
}

void ui_task(void)
{
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return; // not enough time; move on
    start_ms += interval_ms;

    // update_state();

    // set LED colours based on state
    switch (packet->machine_state)
    {
    case STATE_NOT_CONNECTED:
        jogled_colour = led_white;
        raiseled_colour = led_white;
        haltled_colour = led_white;
        feedoverled_colour = led_white;
        spinoverled_colour = led_white;
        coolled_colour = led_white;
        holdled_colour = led_white;
        runled_colour = led_white;
        spindleled_colour = led_white;
        homeled_colour = led_white;
        break;
    case STATE_IDLE:
        runled_colour = led_green;
        holdled_colour = led_orange;
        haltled_colour = led_red;
        break;
    case STATE_HOLD:
        // no jog during hold to jog colors
        runled_colour = led_green;
        holdled_colour = led_orange;
        haltled_colour = led_red;
        jogled_colour = led_off;
        raiseled_colour = led_off;
        break;
    case STATE_TOOL_CHANGE:
        // no change to jog colors
        runled_colour = led_green;
        holdled_colour = led_orange;
        haltled_colour = led_red;
        break;
    case STATE_JOG:
        // Indicate jog in progress
        runled_colour = led_green;
        holdled_colour = led_orange;
        haltled_colour = led_red;
        jogled_colour = led_orange;
        raiseled_colour = led_orange;
        break;
    case STATE_HOMING:
        // No jogging during homing
        runled_colour = led_green;
        holdled_colour = led_orange;
        haltled_colour = led_red;
        jogled_colour = led_off;
        raiseled_colour = led_off;
        break;
    case STATE_CYCLE:
        // No jogging during job
        runled_colour = led_green;
        holdled_colour = led_orange;
        haltled_colour = led_red;
        jogled_colour = led_off;
        raiseled_colour = led_off;
        break;
    case STATE_ALARM:
        jogled_colour = led_red;
        raiseled_colour = led_red;
        haltled_colour = led_red;
        feedoverled_colour = led_red;
        spinoverled_colour = led_red;
        coolled_colour = led_red;
        holdled_colour = led_red;
        runled_colour = led_red;
        spindleled_colour = led_red;
        homeled_colour = led_red;
        break;
    case STATE_MACHINE_OFF:
        jogled_colour = led_yellow;
        raiseled_colour = led_yellow;
        haltled_colour = led_yellow;
        feedoverled_colour = led_yellow;
        spinoverled_colour = led_yellow;
        coolled_colour = led_yellow;
        holdled_colour = led_yellow;
        runled_colour = led_yellow;
        spindleled_colour = led_yellow;
        homeled_colour = led_yellow;
        break;
    case STATE_NOT_HOMED:
        jogled_colour = led_orange;
        raiseled_colour = led_orange;
        haltled_colour = led_red;
        feedoverled_colour = led_off;
        spinoverled_colour = led_off;
        coolled_colour = led_off;
        holdled_colour = led_off;
        runled_colour = led_off;
        spindleled_colour = led_off;
        homeled_colour = led_orange;
        break;
        /* case RUN:
             switch (jog_speed)
             {
             case JOG_DISABLED:
                 jogled_colour = led_blue;
                 raiseled_colour = led_blue;
                 break;
             case JOG_STEP_001:
                 jogled_colour = led_indigo;
                 raiseled_colour = led_indigo;
                 break;
             case JOG_SLOW:
                 jogled_colour = led_orange;
                 raiseled_colour = led_orange;
                 break;
             case JOG_MEDIUM:
                 jogled_colour = led_yellow;
                 raiseled_colour = led_yellow;
                 break;
             case JOG_FAST:
                 jogled_colour = led_red;
                 raiseled_colour = led_red;
                 break;
             }

             if (spindle_on)
             {
                 spindleled_colour = led_red;
             }
             else
             {
                 spindleled_colour = led_blue;
             }

             if (program_running)
             {
                 runled_colour = led_orange;
             }
             else if (program_paused)
             {
                 holdled_colour = led_green;
                 runled_colour = led_off;
             }
             else
             {
                 runled_colour = led_green;
                 holdled_colour = led_yellow;
             }

             if (coolant_on)
             {
                 coolled_colour = led_orange;
             }
             else if (mist_on)
             {
                 coolled_colour = led_blue;
             }
             else
             {
                 coolled_colour = led_red;
             }

             haltled_colour = led_red;
             feedoverled_colour = led_white;
             spinoverled_colour = led_white;
             homeled_colour = led_green;
             break; */
    }

    draw_main_screen();
    pixels.show(); // Send the updated pixel colors to the LEDs.
}

void comm_watchdog()
{
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return; // not enough time; move on
    start_ms += interval_ms;

    // decrement watchdog counter, check status
    comm_watchdog_counter--;
    if (comm_watchdog_counter <= 0 && packet->machine_state != STATE_NOT_CONNECTED)
    {
        packet->machine_state = STATE_NOT_CONNECTED;
        erase_display = 1;
    }
}

void hid_task(void)
{
    // Poll every 1ms
    const uint32_t interval_ms = 1;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return; // not enough time; move on
    start_ms += interval_ms;
    button_report.buttons = halt_pressed | (hold_pressed << 1) | (cycle_start_pressed << 2) | (spindle_pressed << 3) | (mist_pressed << 4) | (flood_pressed << 5) | (home_pressed << 6) | (spindle_over_down_pressed << 7) | (spindle_over_reset_pressed << 8) | (spindle_over_up_pressed << 9) | (feed_over_down_pressed << 10) | (feed_over_reset_pressed << 11) | (feed_over_up_pressed << 12) | (alt_halt_pressed << 13) | (alt_hold_pressed << 14) | (alt_home_pressed << 15) | (alt_cycle_start_pressed << 16) | (alt_spindle_pressed << 17) | (alt_flood_pressed << 18) | (alt_mist_pressed << 19) | (alt_up_pressed << 20) | (alt_down_pressed << 21) | (alt_left_pressed << 22) | (alt_right_pressed << 23) | (alt_raise_pressed << 24) | (alt_lower_pressed << 25);
    button_report.joy0 = 128 + (dpad_right_pressed * 127) - (dpad_left_pressed * 128);
    button_report.joy1 = 128 + (dpad_up_pressed * 127) - (dpad_down_pressed * 128);
    button_report.joy2 = 128 + (raise_pressed * 127) - (lower_pressed * 128);
    button_report.joy3 = jog_speed;
    encoder_report.x_encoder_counts = jog_count_x;
    encoder_report.y_encoder_counts = jog_count_y;
    encoder_report.z_encoder_counts = jog_count_z;

    // Remote wakeup
    if (tud_suspended())
    {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
    }
    else
    {
        // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
        send_hid_report(REPORT_ID_BUTTONS);
    }

    if (tud_hid_ready())
    {
        //        tud_hid_n_report(0x00, 0x01, &button_report, sizeof(button_report));
        // Second report gets sent in callback tud_hid_report_complete_cb
    }
}

void cdc_task(void)
{
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_connected() )
    // {
    // connected and there are data available
    if (tud_cdc_available())
    {
        // read data
        uint8_t buffer[64];
        uint32_t bufsize = tud_cdc_read(buffer, sizeof(buffer));

        if (bufsize >= 38)
        // Status data. 38 bytes of data, could receive up to 40 when padded. Last two bytes unimportant if transmitted.
        {

            int i = 0;
            prev_packet = *packet;
            *packet = unpackStatus(buffer, &i);
            if (packet->machine_state != previous_packet->machine_state)
            {
                erase_display = 1;
            }
            reset_comm_watchdog();
        }

        // Echo back
        // Note: Skip echo by commenting out write() and write_flush()
        // for throughput test e.g
        //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000
        //  tud_cdc_write(buf, count);
        // tud_cdc_write_flush();
    }
    // }
}

//--------------------------------------------------------------------+
// Function declarations
//--------------------------------------------------------------------+

void con_panic(uint16_t errcode)
{
    button_report.buttons = errcode;
    while (1)
    {
        tud_task(); // tinyusb device task
        // Remote wakeup
        if (tud_suspended())
        {
            // Wake up host if we are in suspend mode
            // and REMOTE_WAKEUP feature is enabled by host
            tud_remote_wakeup();
        }

        if (tud_hid_ready())
        {
            tud_hid_n_report(0x00, 0x01, &button_report, sizeof(button_report));
        }
    }
}

uint8_t adjust(uint8_t value)
{
    if (NEO_BRIGHTNESS == 0)
        return value;
    return ((value * neopixels_gamma8(NEO_BRIGHTNESS)) >> 8);
};

void reset_comm_watchdog()
{
    // Allow 10 iterations of the watchdog task (100ms) before firing
    comm_watchdog_counter = 10;
}

Jogger_status_packet unpackStatus(const uint8_t *buf, int *i)
{
    Jogger_status_packet p;
    memcpy(&p, buf, sizeof(Jogger_status_packet));
    return p;
}

unsigned int packInt(const uint8_t *buf, int *i)
{
    unsigned int b;
    memcpy(&buf, &b, 4);
    return b;
}

char *uitoa(uint32_t n) // Converts an uint32 variable to string.
{
    char *bptr = buf + sizeof(buf);

    *--bptr = '\0';

    if (n == 0)
        *--bptr = '0';
    else
        while (n)
        {
            *--bptr = '0' + (n % 10);
            n /= 10;
        }

    return bptr;
}

static char *map_coord_system(coord_system_id_t id)
{
    uint8_t g5x = id + 54;

    strcpy(buf, uitoa((uint32_t)(g5x > 59 ? 59 : g5x)));
    if (g5x > 59)
    {
        strcat(buf, ".");
        strcat(buf, uitoa((uint32_t)(g5x - 59)));
    }

    return buf;
}

void read_flash_config()
{
    memcpy(&config_data, flash_target_contents, sizeof(config_data));
    screenflip = config_data.screen_flip;
}

void write_flash_config()
{
    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    config_data.screen_flip = screenflip;
    //    config_data.jog_speed_x = 6000;
    //    config_data.jog_speed_y = 6000;
    //    config_data.jog_speed_z = 2000;
    flash_range_program(FLASH_TARGET_OFFSET, (uint8_t *)&config_data, FLASH_PAGE_SIZE);
    restore_interrupts(interrupts);
}

void draw_main_screen()
{
#define JOGLINE 7
#define TOPLINE 2
#define INFOLINE 0
#define BOTTOMLINE 7

    char charbuf[32];

    if (screenmode != previous_screenmode)
    {
        oledFill(&oled, 0, 1); // clear screen when mode changes
    }

    switch (screenmode)
    {
    case JOG_MODIFY:
        oledWriteString(&oled, 0, 10, 2, (char *)"JOG SPEED", FONT_12x16, 0, 1);
        switch (jog_speed)
        {
        case JOG_DISABLED:
            oledWriteString(&oled, 0, 5, 5, (char *)" DISABLED ", FONT_12x16, 0, 1);
            break;
        case JOG_STEP_001:
            oledWriteString(&oled, 0, 5, 5, (char *)"STEP 0.001", FONT_12x16, 0, 1);
            break;
        case JOG_STEP_01:
            oledWriteString(&oled, 0, 5, 5, (char *)"STEP 0.010", FONT_12x16, 0, 1);
            break;
        case JOG_STEP_1:
            oledWriteString(&oled, 0, 5, 5, (char *)"STEP 0.100", FONT_12x16, 0, 1);
            break;
        case JOG_STEP_10:
            oledWriteString(&oled, 0, 5, 5, (char *)"STEP 1.000", FONT_12x16, 0, 1);
            break;
        case JOG_STEP_100:
            oledWriteString(&oled, 0, 5, 5, (char *)"STEP 10.00", FONT_12x16, 0, 1);
            break;
        case JOG_SLOW:
            oledWriteString(&oled, 0, 5, 5, (char *)"   SLOW   ", FONT_12x16, 0, 1);
            break;
        case JOG_MEDIUM:
            oledWriteString(&oled, 0, 5, 5, (char *)"  MEDIUM  ", FONT_12x16, 0, 1);
            break;
        case JOG_FAST:
            oledWriteString(&oled, 0, 5, 5, (char *)"   FAST   ", FONT_12x16, 0, 1);
            break;
        }
        break;
    case JOGGING:
    case RUN:
    case DEFAULT:

        // clear screen when needed
        if (erase_display)
        {
            oledFill(&oled, 0, 1);
            erase_display = 0;
        }

        switch (packet->machine_state)
        {
        case STATE_NOT_CONNECTED:
            oledWriteString(&oled, 0, 42, 2, (char *)"NOT", FONT_12x16, 0, 1);
            oledWriteString(&oled, 0, 0, 5, (char *)"CONNECTED!", FONT_12x16, 0, 1);
            previous_packet->machine_state = STATE_NOT_CONNECTED; // overwrite previous state so we don't keep erasing the display. We're not connected, so there will be no more packets.
            previous_state = STATE_NOT_CONNECTED;
            break;

        case STATE_HOMING:
            oledWriteString(&oled, 0, 0, 0, (char *)"****************", FONT_8x8, 0, 1);
            oledWriteString(&oled, 0, 28, 3, (char *)"HOMING", FONT_12x16, 0, 1);
            oledWriteString(&oled, 0, 0, 7, (char *)"****************", FONT_8x8, 0, 1);
            previous_state = STATE_HOMING;
            break;

        case STATE_ALARM:
            sprintf(charbuf, "Code: %d ", packet->alarm);
            oledWriteString(&oled, 0, 0, 0, (char *)"****************", FONT_8x8, 0, 1);
            oledWriteString(&oled, 0, 30, 2, (char *)"ALARM", FONT_12x16, 0, 1);
            oledWriteString(&oled, 0, 18, 4, charbuf, FONT_12x16, 0, 1);
            oledWriteString(&oled, 0, 0, 7, (char *)"****************", FONT_8x8, 0, 1);
            previous_state = STATE_ALARM;
            break;

        case STATE_JOG:
        case STATE_IDLE:
        case STATE_CYCLE:
        case STATE_HOLD:
        case STATE_TOOL_CHANGE:
            // WCS
            oledWriteString(&oled, 0, 96, 2, (char *)"G", FONT_6x8, 0, 1);
            oledWriteString(&oled, 0, -1, -1, map_coord_system(packet->current_wcs), FONT_6x8, 0, 1);

            // Coordinates
            sprintf(charbuf, "X %8.3F", packet->x_coordinate);
            oledWriteString(&oled, 0, 0, 2, charbuf, FONT_8x8, 0, 1);
            sprintf(charbuf, "Y %8.3F", packet->y_coordinate);
            oledWriteString(&oled, 0, 0, 3, charbuf, FONT_8x8, 0, 1);
            sprintf(charbuf, "Z %8.3F", packet->z_coordinate);
            oledWriteString(&oled, 0, 0, 4, charbuf, FONT_8x8, 0, 1);
            if (packet->a_coordinate != 0xFFFFFFFF)
            {
                sprintf(charbuf, "A %8.3F", packet->a_coordinate);
                oledWriteString(&oled, 0, 0, 5, charbuf, FONT_8x8, 0, 1);
            }

            // Spindle RPM
            sprintf(charbuf, "%5d", packet->spindle_rpm);
            oledWriteString(&oled, 0, 102, 6, (char *)"RPM", FONT_6x8, 0, 1);
            oledWriteString(&oled, 0, 96, BOTTOMLINE, charbuf, FONT_6x8, 0, 1);

            switch (packet->machine_state)
            {
            case STATE_JOG:
                // State
                oledWriteString(&oled, 0, 96, 4, (char *)"JOG ", FONT_6x8, 0, 1);
                // Jog rate and mode
                display_jog_mode(charbuf);
                // Spindle and Feed overrides
                display_overrides(charbuf);
                previous_state = STATE_JOG;
                break;
            case STATE_IDLE:
                // State
                oledWriteString(&oled, 0, 96, 4, (char *)"IDLE", FONT_6x8, 0, 1);
                // Jog rate and mode
                display_jog_mode(charbuf);
                // Spindle and Feed overrides
                display_overrides(charbuf);
                previous_state = STATE_IDLE;
                break;
            case STATE_CYCLE:
                // State
                oledWriteString(&oled, 0, 96, 4, (char *)"RUN  ", FONT_6x8, 0, 1);
                // Feedrate
                sprintf(charbuf, "%3.3f ", packet->feed_rate);
                oledWriteString(&oled, 0, 0, INFOLINE, (char *)"RUN FEED: ", FONT_8x8, 0, 1);
                oledWriteString(&oled, 0, -1, INFOLINE, charbuf, FONT_8x8, 0, 1);
                // Spindle and Feed overrides
                display_overrides(charbuf);
                previous_state = STATE_CYCLE;
                break;
            case STATE_HOLD:
                // Title line
                oledWriteString(&oled, 0, 36, INFOLINE, (char *)"HOLDING", FONT_8x8, 0, 1);
                // State we came from/will go back to
                switch (previous_state)
                {
                case STATE_CYCLE:
                    oledWriteString(&oled, 0, 96, 4, (char *)"RUN  ", FONT_6x8, 0, 1);
                    break;
                case STATE_JOG:
                    oledWriteString(&oled, 0, 96, 4, (char *)"JOG ", FONT_6x8, 0, 1);
                    break;
                case STATE_IDLE:
                    oledWriteString(&oled, 0, 96, 4, (char *)"IDLE", FONT_6x8, 0, 1);
                    break;
                }
                // Spindle and Feed overrides
                display_overrides(charbuf);
                previous_state = STATE_HOLD;
                break;
            case STATE_TOOL_CHANGE:
                // State we came from/will go back to
                switch (previous_state)
                {
                case STATE_CYCLE:
                    oledWriteString(&oled, 0, 96, 4, (char *)"RUN  ", FONT_6x8, 0, 1);
                    break;
                case STATE_JOG:
                    oledWriteString(&oled, 0, 96, 4, (char *)"JOG ", FONT_6x8, 0, 1);
                    break;
                case STATE_IDLE:
                    oledWriteString(&oled, 0, 96, 4, (char *)"IDLE", FONT_6x8, 0, 1);
                    break;
                }
                // Jog rate and mode
                display_jog_mode(charbuf);

                // Bottom line
                oledWriteString(&oled, 0, 0, BOTTOMLINE, (char *)" TOOL CHANGE", FONT_8x8, 0, 1);
                previous_state = STATE_TOOL_CHANGE;
                break;
            }
        }
    }
    // prev_packet = *packet;
    previous_jogmode = current_jogmode;
    previous_jogmodify = current_jogmodify;
    previous_screenmode = screenmode;
}

void settings_ui()
{

    // erase display
    oledFill(&oled, 0, 1);

    // draw menu
    oledWriteString(&oled, 0, 32, 0, (char *)"SETTINGS", FONT_8x8, 0, 1);
    oledWriteString(&oled, 0, 0, 2, (char *)"Flip Display", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 3, (char *)"Jog Speed Slow", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 4, (char *)"Jog Speed Med", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 5, (char *)"Jog Speed Fast", FONT_6x8, 0, 1);
    while (1)
    {
        oledWriteString(&oled, 0, 90, 2, (char *)"1", FONT_6x8, 0, 1);    // display flip
        oledWriteString(&oled, 0, 90, 3, (char *)"0010", FONT_6x8, 0, 1); // Jog Speed Slow
        oledWriteString(&oled, 0, 90, 4, (char *)"0500", FONT_6x8, 0, 1); // Jog Speed Med
        oledWriteString(&oled, 0, 90, 5, (char *)"5000", FONT_6x8, 0, 1); // Jog Speed Fast
    }
}

void display_jog_mode(char *buf)
{
    // Jog rates and mode
    switch (current_jogmode)
    {
    case FAST:
    case SLOW:
        oledWriteString(&oled, 0, 0, INFOLINE, (char *)"JOG FEED", FONT_8x8, 0, 1);
        break;
    case STEP:
        oledWriteString(&oled, 0, 0, INFOLINE, (char *)"JOG STEP", FONT_8x8, 0, 1);
        break;
    default:
        break;
    }
    sprintf(buf, "%3.3f ", packet->feed_rate);
    oledWriteString(&oled, 0, -1, INFOLINE, buf, FONT_8x8, 0, 1);
}

void display_overrides(char *buf)
{
    sprintf(buf, "S:%3d  F:%3d", packet->spindle_override, packet->feed_override);
    oledWriteString(&oled, 0, 0, BOTTOMLINE, buf, FONT_6x8, 0, 1);
}

//--------------------------------------------------------------------+
// HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id)
{
    // skip if hid is not ready yet
    if (!tud_hid_ready())
        return;

    switch (report_id)
    {
    case REPORT_ID_BUTTONS:
        tud_hid_n_report(0x00, REPORT_ID_BUTTONS, &button_report, sizeof(button_report));
        break;

    case REPORT_ID_ENCODERS:
        tud_hid_n_report(0x00, REPORT_ID_ENCODERS, &encoder_report, sizeof(encoder_report));
        break;
    }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
TU_ATTR_WEAK void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint8_t len)
{
    if (report[0] == 1)
    {
        // first report sent; send second
        send_hid_report(2);
    }
    else
    {
        // second report send; send first
        send_hid_report(1);
    }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    // TODO - Implment this. Doesn't seem to ever actually get called though.
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    if (report_id == 2 && report_type == HID_REPORT_TYPE_OUTPUT && bufsize >= 38)
    // Status data. 38 bytes of data, could receive up to 40 when padded. Last two bytes unimportant if transmitted.
    {

        int i = 0;
        prev_packet = *packet;
        *packet = unpackStatus(buffer, &i);
        if (packet->machine_state != previous_packet->machine_state)
        {
            erase_display = 1;
        }
        reset_comm_watchdog();
    }
    // echo back anything we received from host
    tud_hid_report(0, buffer, bufsize);
}
