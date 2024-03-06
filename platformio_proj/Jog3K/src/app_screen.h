#ifndef APP_SCREEN_H_
#define APP_SCREEN_H_
#pragma once

#include <DisplayUtils.h>

/**
 * @brief Follows the convention of linuxcnc
 */
enum Flood_e : uint8_t {
  FLOOD_OFF, FLOOD_ON
};

/**
 * @brief Follows the convention of linuxcnc
 */
enum Mist_e : uint8_t {
  MIST_OFF, MIST_ON
};

/**
 * @brief Follows the convention of linuxcnc (except INIT)
 */
enum Task_state_e : uint8_t {
  STATE_INIT, STATE_ESTOP, STATE_ESTOP_RESET, STATE_OFF, STATE_ON
};

/**
 * @brief Follows the convention of linuxcnc (except UNKNOWN)
 */
enum Task_mode_e : uint8_t {
  MODE_UNKNOWN, MODE_MANUAL, MODE_AUTO, MODE_MDI
};

/**
 * @brief A list of axis + 'none'
 * 
 */
enum Axis_e : int8_t {
  AXIS_NONE=-1, AXIS_X, AXIS_Y, AXIS_Z, AXIS_A, AXIS_B, AXIS_C, AXIS_U, AXIS_V, AXIS_W
};

/**
 * @brief Jog velocity can be toggled between high & low range
 * 
 */
enum JogRange_e : uint8_t {
  JOG_RANGE_LOW, JOG_RANGE_HIGH
  };

/**
 * @brief Map the axis position (0-8) to its well-known name
 * 
 */
const char AXIS_NAME[] = "XYZABCUVW";

enum Screen_e : uint8_t {
  SCREEN_INIT, SCREEN_SPLASH, SCREEN_ESTOP, SCREEN_ESTOP_RESET, SCREEN_MANUAL, SCREEN_MDI, SCREEN_AUTO, SCREEN_OFFSET_KEYPAD
};

/**
 * @brief Gives context to ButtonType_e
 */
enum ButtonRow_e : uint8_t {
  BUTTON_ROW_NONE, BUTTON_ROW_DEFAULT, BUTTON_ROW_MANUAL, BUTTON_ROW_AUTO, BUTTON_ROW_MDI, BUTTON_ROW_SPINDLE_START, BUTTON_ROW_SPINDLE_STOP, BUTTON_ROW_PROGRAM_START, BUTTON_ROW_G5X_OFFSET
};

/**
 * @brief type of button. Should behave based on context of ButtonRow.
 */
enum ButtonType_e : uint8_t {
  BUTTON_NONE, BUTTON_HALT, BUTTON_STOP, BUTTON_CANCEL, BUTTON_TICK, BUTTON_TOUCHOFF, BUTTON_HALF, BUTTON_PLAY, BUTTON_PAUSE, BUTTON_ONE_STEP, BUTTON_COOLANT
};

enum ButtonState_e : uint8_t {
  BUTTON_STATE_NONE, BUTTON_STATE_ACTIVE, BUTTON_STATE_INACTIVE, BUTTON_STATE_ON /*implied active*/, BUTTON_STATE_OFF /*implied active*/
};

enum ErrorMessage_e : uint8_t {
    ERRMSG_NONE,
    ERRMSG_NOT_HOMED,
};

    struct DisplayAreas_s {
      DisplayArea axes;
      DisplayArea axesMarkers;
      DisplayArea axesLabels;
      DisplayArea axesCoords;
      DisplayArea encoderLabel[3];
      DisplayArea encoderValue[3];
      DisplayArea debugRow;
    };

    struct DrawnState_s {
      Axis_e currentAxis = AXIS_NONE;
      Axis_e joystickAxis = AXIS_NONE;
      uint8_t homed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      float displayedAxisValues[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      uint8_t displayedAxes = 4;
      uint8_t displayedCoordSystem = 2;
      uint8_t g5xIndex = 1;
      float spindleOverride = 1;
      float spindleSpeed = 0; // Commanded spindle speed (multiplied by override to give rpm)
      float spindleRpm = 0; // Actual spindale speed
      int8_t spindleDirection = 0; //Indicates whether spindle is on or off 1=fwd, -1=rev, 0=stopped
      float rapidrate = 1;
      float rapidSpeed = 0;
      float rapid_vel = 0; //locally calculated for display based on current_vel & motion_type
      float feed_vel = 0; //locally calculated for display based on current_vel & motion_type
      float feedrate = 1;
      float jogVelocity[2] = { 180, 3000 }; //Sent to serial (as mm/min) but does not update gmoccapy
      bool pulseDrawn = false;
      JogRange_e jogVelocityRange = JOG_RANGE_HIGH;
      uint8_t currentJogIncrement = 3;
      ButtonRow_e buttonRow = BUTTON_ROW_NONE;
      Task_state_e task_state = STATE_INIT;
      Screen_e screen = SCREEN_INIT;
      Task_mode_e task_mode = MODE_UNKNOWN;
      Flood_e flood = FLOOD_OFF;
      Mist_e mist = MIST_OFF;
      ErrorMessage_e errorMessage = ERRMSG_NONE;
    };

#endif