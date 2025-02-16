#ifndef __I2C_JOGGER_H__
#define __I2C_JOGGER_H__

#include <Roboto_Condensed_Light_8.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/Picopixel.h>

// 20250216 DJF - add a board revision define
// moved to platformio.ini BOARD_REVISION - set to 2 or 3
//
//#define BOARD_REVISION 2
//#define BOARD_REVISION 3

#define PROTOCOL_VERSION 1

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        21 // On Trinket or Gemma, suggest changing this to 1
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 20 // Popular NeoPixel ring size
#define DELAYVAL 200 // Time (in milliseconds) to pause between pixels
#define CYCLEDELAY 12 // Time to wait after a cycle
#define NEO_BRIGHTNESS 10

#define NUMBER_OF_AXES 3

//flash defines
#define FLASH_TARGET_OFFSET (256 * 1024)
//extern const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
extern const uint8_t *flash_target_contents;

//neopixel led locations
// 20250214 DJF - reallocate neopixels for A3 board
#if BOARD_REVISION < 3
    #define SYSLED 0
    #define SELLED 1
    #define LEFTLED 2
    #define DOWNLED 3
    #define LOWERLED 4
    #define RIGHTLED 5
    #define RAISELED 6
    #define UPLED 7
    #define HOMELED 8
    #define SPINDLELED 9
    #define SEL2LED 10
    #define SPINLED 11
    #define SPINLED1 12
    #define MISTLED 13
    #define HOLDLED 14
    #define HALTLED 15
    #define RUNLED 16
    #define COOLED 17
    #define FEEDLED 18
    #define FEEDLED1 19

    #define NUMLEDS 20 // on A2 board

#else // Board Revision is 3 (or greater)
    #define SYSLED 0
    #define UPLED 1
    #define LEFTLED 2
    #define DOWNLED 3
    #define LOWERLED 4
    #define RIGHTLED 5
    #define RAISELED 6
    #define SPINLED 7
    #define SPINDLELED 8
    #define MISTLED 9
    #define FEEDHOLDLED 10
    #define HOLDLED 10 // DJF Note - I thinkl this is the same as FEEDHOLDLED?
    #define HALTLED 11
    #define RUNLED 12
    #define FLOODLED 13
    #define COOLED 13 // DJF Note - Not sure if COOLED is the same as FLOODLED?
    #define HOMELED 14
    #define FEEDLED 15

    #define NUMLEDS 16  // On A3 board
#endif // BOARD_REVISION

//button GPIO numbers.
#define HALTBUTTON 13
#define RUNBUTTON 14
#define HOLDBUTTON 12

#define JOG_SELECT 24
#define JOG_SELECT2 7
#define UPBUTTON 2
#define DOWNBUTTON 1
#define LEFTBUTTON 29
#define RIGHTBUTTON 3
#define LOWERBUTTON 4
#define RAISEBUTTON 5

#define FEEDOVER_RESET 10

#define SPINOVER_RESET 27

#define SPINDLEBUTTON 6
#define FLOODBUTTON 15
#define MISTBUTTON 11
#define HOMEBUTTON 28

//button masks
#define halt_pressed = 0;
#define hold_pressed = 1;
#define cycle_start_pressed = 2;
#define alt_hold_pressed = 16;
#define alt_halt_pressed = 15;
#define alt_cycle_start_pressed = 17;

#define spindle_over_reset_pressed = 7;
#define feed_over_reset_pressed = 8;
#define alt_spindle_over_reset_pressed = 22;
#define alt_feed_over_reset_pressed = 23;

#define spindle_pressed = 3;
#define mist_pressed = 4;
#define flood_pressed = 5;
#define home_pressed = 6;
#define alt_spindle_pressed = 18;
#define alt_mist_pressed = 19;
#define alt_flood_pressed = 20;
#define alt_home_pressed = 21;

#define up_pressed = 9;
#define down_pressed = 10;
#define left_pressed = 11;
#define right_pressed = 12;
#define raise_pressed = 13;
#define lower_pressed = 14;

#define alt_up_pressed = 24;
#define alt_down_pressed = 25;
#define alt_left_pressed = 26;
#define alt_right_pressed = 27;
#define alt_lower_pressed = 28;
#define alt_raise_pressed = 29;

#define TICK_TIMER_PERIOD 5

#define LED_UPDATE_PERIOD 100 / TICK_TIMER_PERIOD
#define SCREEN_UPDATE_PERIOD 30 / TICK_TIMER_PERIOD
#define KPSTR_DELAY 50 / TICK_TIMER_PERIOD

//ACTION DEFINES
#define MACROUP 0x18
#define MACRODOWN 0x19
#define MACROLEFT 0x1B
#define MACRORIGHT 0x1A
#define MACROLOWER  0x7D
#define MACRORAISE 0x7C
#define MACROHOME  0x8E
#define RESET  0x7F
#define UNLOCK 0x80
#define SPINON 0x81

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3
#define RAISE 4
#define LOWER 5

//keycode mappings:
#define JOG_XR   0b000010
#define JOG_XL   0b001000
#define JOG_YF   0b000100
#define JOG_YB   0b000001
#define JOG_ZU   0b010000
#define JOG_ZD   0b100000
#define JOG_XRYF JOG_XR | JOG_YB
#define JOG_XRYB JOG_XR | JOG_YF
#define JOG_XLYF JOG_XL | JOG_YB
#define JOG_XLYB JOG_XL | JOG_YF
/*#define JOG_XRZU JOG_XR | JOG_ZU
#define JOG_XRZD JOG_XR | JOG_ZD
#define JOG_XLZU JOG_XL | JOG_ZU
#define JOG_XLZD JOG_XL | JOG_ZD*/

// The slave implements a 256 byte memory. To write a series of bytes, the master first
// writes the memory address, followed by the data. The address is automatically incremented
// for each byte transferred, looping back to 0 upon reaching the end. Reading is done
// sequentially from the current memory address.
#define I2C_TIMEOUT_VALUE 100

typedef enum {
    CoordinateSystem_G54 = 0,                       //!< 0 - G54 (G12)
    CoordinateSystem_G55,                           //!< 1 - G55 (G12)
    CoordinateSystem_G56,                           //!< 2 - G56 (G12)
    CoordinateSystem_G57,                           //!< 3 - G57 (G12)
    CoordinateSystem_G58,                           //!< 4 - G58 (G12)
    CoordinateSystem_G59,                           //!< 5 - G59 (G12)
#if COMPATIBILITY_LEVEL <= 1
    CoordinateSystem_G59_1,                         //!< 6 - G59.1 (G12) - availability depending on #COMPATIBILITY_LEVEL <= 1
    CoordinateSystem_G59_2,                         //!< 7 - G59.2 (G12) - availability depending on #COMPATIBILITY_LEVEL <= 1
    CoordinateSystem_G59_3,                         //!< 8 - G59.3 (G12) - availability depending on #COMPATIBILITY_LEVEL <= 1
#endif
    N_WorkCoordinateSystems,                        //!< 9 when #COMPATIBILITY_LEVEL <= 1, 6 otherwise
    CoordinateSystem_G28 = N_WorkCoordinateSystems, //!< 9 - G28 (G0) when #COMPATIBILITY_LEVEL <= 1, 6 otherwise
    CoordinateSystem_G30,                           //!< 10 - G30 (G0) when #COMPATIBILITY_LEVEL <= 1, 7 otherwise
    CoordinateSystem_G92,                           //!< 11 - G92 (G0) when #COMPATIBILITY_LEVEL <= 1, 8 otherwise
    N_CoordinateSystems                             //!< 12 when #COMPATIBILITY_LEVEL <= 1, 9 otherwise
} coord_system_id_t;


//typedef uint8_t msg_type_t;
//typedef uint8_t machine_state_t;

enum msg_type_t {
    MachineMsg_None = 0,
// 1-127 reserved for message string length
    MachineMsg_Comment = 252,
    MachineMsg_Overrides = 253,
    MachineMsg_WorkOffset = 254,
    MachineMsg_ClearMessage = 255,
};

#define STATE_DISCONNECTED  0
#define STATE_ALARM         1 //!< In alarm state. Locks out all g-code processes. Allows settings access.
#define STATE_CYCLE         2 //!< Cycle is running or motions are being executed.
#define STATE_HOLD          3 //!< Active feed hold
#define STATE_TOOL_CHANGE   4 //!< Manual tool change, similar to #STATE_HOLD - but stops spindle and allows jogging.
#define STATE_IDLE          5 //!< Must be zero. No flags.
#define STATE_HOMING        6 //!< Performing homing cycle
#define STATE_JOG           7 //!< Jogging mode.

enum machine_state_t {
    MachineState_Disconnected = 0,
    MachineState_Alarm = 1,
    MachineState_Cycle = 2,
    MachineState_Hold = 3,
    MachineState_ToolChange = 4,
    MachineState_Idle = 5,
    MachineState_Homing = 6,
    MachineState_Jog = 7,
    MachineState_Other = 254
};

//static_assert(sizeof(msg_type_t) == 1, "msg_type_t too large for I2C display interface");
//static_assert(sizeof(machine_state_t) == 1, "machine_state_t too large for I2C display interface");
//static_assert(sizeof(coord_system_id_t) == 1, "coord_system_id_t too large for I2C display interface");

#define JOGMODE_FAST 0
#define JOGMODE_SLOW 1
#define JOGMODE_STEP 2
#define JOGMODE ROTATE 3
#define JOGMODE_MAX 2
#define JOGMODIFY_MAX 3


typedef union {
    uint8_t value;
    struct {
        uint8_t modifier :4,
                mode     :4;
    };
} jog_mode_t;

typedef union {
    uint8_t value;
    struct {
        uint8_t diameter       :1,
                mpg            :1,
                homed          :1,
                tlo_referenced :1,
                mode           :3; // from machine_mode_t setting
    };
} machine_modes_t;

typedef union {
    float values[4];
    struct {
        float x;
        float y;
        float z;
        float a;
    };
} machine_coords_t;

typedef union {
    uint8_t mask;
    uint8_t value;
    struct {
        uint8_t x :1,
                y :1,
                z :1,
                a :1,
                b :1,
                c :1,
                u :1,
                v :1;
    };
} axes_signals_t;

typedef union {
    uint8_t value;
    uint8_t mask;
    struct {
        uint8_t on               :1,
                ccw              :1,
                pwm              :1, //!< NOTE: only used for PWM inversion setting
                reserved         :1,
                override_disable :1,
                encoder_error    :1,
                at_speed         :1, //!< Spindle is at speed.
                synchronized     :1;
    };
} spindle_state_t;

typedef union {
    uint8_t value;                 //!< Bitmask value
    uint8_t mask;                  //!< Synonym for bitmask value
    struct {
        uint8_t flood          :1, //!< Flood coolant.
                mist           :1, //!< Mist coolant, optional.
                shower         :1, //!< Shower coolant, currently unused.
                trough_spindle :1, //!< Through spindle coolant, currently unused.
                unused         :4;
    };
} coolant_state_t;

typedef union {
    uint16_t value;
    uint16_t mask;
    struct {
        uint16_t reset              :1,
                 feed_hold          :1,
                 cycle_start        :1,
                 safety_door_ajar   :1,
                 block_delete       :1,
                 stop_disable       :1, //! M1
                 e_stop             :1,
                 probe_disconnected :1,
                 motor_fault        :1,
                 motor_warning      :1,
                 limits_override    :1,
                 single_block       :1,
                 unassigned         :1,
                 probe_overtravel   :1, //! used for probe protection
                 probe_triggered    :1, //! used for probe protection
                 deasserted         :1; //! this flag is set if signals are deasserted. Note: do NOT pass on to the control_interrupt_handler if set.
    };
} control_signals_t;

// Define Grbl status codes. Valid values (0-255)
typedef enum {
    Status_OK = 0,
    Status_ExpectedCommandLetter = 1,
    Status_BadNumberFormat = 2,
    Status_InvalidStatement = 3,
    Status_NegativeValue = 4,
    Status_HomingDisabled = 5,
    Status_SettingStepPulseMin = 6,
    Status_SettingReadFail = 7,
    Status_IdleError = 8,
    Status_SystemGClock = 9,
    Status_SoftLimitError = 10,
    Status_Overflow = 11,
    Status_MaxStepRateExceeded = 12,
    Status_CheckDoor = 13,
    Status_LineLengthExceeded = 14,
    Status_TravelExceeded = 15,
    Status_InvalidJogCommand = 16,
    Status_SettingDisabledLaser = 17,
    Status_Reset = 18,
    Status_NonPositiveValue = 19,

    Status_GcodeUnsupportedCommand = 20,
    Status_GcodeModalGroupViolation = 21,
    Status_GcodeUndefinedFeedRate = 22,
    Status_GcodeCommandValueNotInteger = 23,
    Status_GcodeAxisCommandConflict = 24,
    Status_GcodeWordRepeated = 25,
    Status_GcodeNoAxisWords = 26,
    Status_GcodeInvalidLineNumber = 27,
    Status_GcodeValueWordMissing = 28,
    Status_GcodeUnsupportedCoordSys = 29,
    Status_GcodeG53InvalidMotionMode = 30,
    Status_GcodeAxisWordsExist = 31,
    Status_GcodeNoAxisWordsInPlane = 32,
    Status_GcodeInvalidTarget = 33,
    Status_GcodeArcRadiusError = 34,
    Status_GcodeNoOffsetsInPlane = 35,
    Status_GcodeUnusedWords = 36,
    Status_GcodeG43DynamicAxisError = 37,
    Status_GcodeIllegalToolTableEntry = 38,
    Status_GcodeValueOutOfRange = 39,
    Status_GcodeToolChangePending = 40,
    Status_GcodeSpindleNotRunning = 41,
    Status_GcodeIllegalPlane = 42,
    Status_GcodeMaxFeedRateExceeded = 43,
    Status_GcodeRPMOutOfRange = 44,
    Status_LimitsEngaged = 45,
    Status_HomingRequired = 46,
    Status_GCodeToolError = 47,
    Status_ValueWordConflict = 48,
    Status_SelfTestFailed = 49,
    Status_EStop = 50,
    Status_MotorFault = 51,
    Status_SettingValueOutOfRange = 52,
    Status_SettingDisabled = 53,
    Status_GcodeInvalidRetractPosition = 54,
    Status_IllegalHomingConfiguration = 55,
    Status_GCodeCoordSystemLocked = 56,

// Some error codes as defined in bdring's ESP32 port
    Status_SDMountError = 60,
    Status_SDReadError = 61,
    Status_SDFailedOpenDir = 62,
    Status_SDDirNotFound = 63,
    Status_SDFileEmpty = 64,

    Status_BTInitError = 70,

//
    Status_ExpressionUknownOp = 71,
    Status_ExpressionDivideByZero = 72,
    Status_ExpressionArgumentOutOfRange = 73,
    Status_ExpressionInvalidArgument = 74,
    Status_ExpressionSyntaxError = 75,
    Status_ExpressionInvalidResult = 76,

    Status_AuthenticationRequired = 77,
    Status_AccessDenied = 78,
    Status_NotAllowedCriticalEvent = 79,

    Status_FlowControlNotExecutingMacro = 80,
    Status_FlowControlSyntaxError = 81,
    Status_FlowControlStackOverflow = 82,
    Status_FlowControlOutOfMemory = 83,

    Status_Unhandled, // For internal use only
    Status_StatusMax = Status_Unhandled
} status_code_t;

typedef struct __attribute__((packed)) {
    //uint16_t address;  //address is only used for I2C transactions, only present in GRBLHAL plugin
    machine_state_t machine_state;
    uint8_t machine_substate;
    axes_signals_t home_state;
    uint16_t feed_override; // size changed in latest version!
    uint16_t spindle_override;
    uint8_t spindle_stop;
    spindle_state_t spindle_state;
    int spindle_rpm;
    float feed_rate;
    coolant_state_t coolant_state;
    jog_mode_t jog_mode;
    control_signals_t signals;
    float jog_stepsize;
    coord_system_id_t current_wcs;  //active WCS or MCS modal state
    axes_signals_t limits;
    status_code_t status_code;
    machine_modes_t machine_modes;
    machine_coords_t coordinate;
    msg_type_t msgtype; //<! 1 - 127 -> msg[] contains a string msgtype long
    uint8_t msg[128];
} machine_status_packet_t;

#define MINVAL -9999.99
#define MAXVAL 9999.99

typedef struct __attribute__((packed)) {
uint32_t uptime;
jog_mode_t jog_mode;
uint32_t feed_over;
uint32_t spindle_over;
uint32_t rapid_over;
uint32_t buttons;
float feedrate; 
float spindle_rpm; 
float x_axis;
float y_axis;
float z_axis;
float a_axis;
} pendant_count_packet_t;

typedef struct
{
    uint8_t mem[1024];
    uint16_t mem_address;
    bool mem_address_written;
} status_context_t;

enum ScreenMode{
    none,
    JOGGING,
    HOMING,
    RUN,
    HOLD,
    JOG_MODIFY,
    TOOL_CHANGE,
    ALARM,
    DISCONNECTED = 255,
};

enum CurrentJogAxis{
    X,
    Y,
    Z,
    A,
    FOVER,
    SOVER,
    NONE = 255,
};

extern ScreenMode screenmode;
extern ScreenMode previous_screenmode;

//extern machine_status_packet_t *statuspacket;
//extern machine_status_packet_t prev_statuspacket;

//extern pendant_count_packet_t prev_countpacket;
//extern pendant_count_packet_t *countpacket;

extern bool screenflip;
extern int command_error;
extern float step_calc;

extern ScreenMode screenmode;
extern jog_mode_t current_jogmode;
extern jog_mode_t previous_jogmode;
extern ScreenMode previous_screenmode;

//extern Adafruit_NeoPixel pixels;

//device specific variables
extern CurrentJogAxis current_jog_axis;
extern CurrentJogAxis previous_jog_axis;
extern uint8_t simulation_mode;

void draw_string(char * str);
void draw_main_screen(machine_status_packet_t *prev_statuspacket, machine_status_packet_t *statuspacket);
void update_neopixels(machine_status_packet_t *prev_statuspacket, machine_status_packet_t *statuspacket);
void init_screen (void);
void init_neopixels (void);
void write_nvs (void);

#if defined(_LINUX_) && defined(__cplusplus)
}
#endif // _LINUX_

#endif // 

