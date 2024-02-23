#ifndef __I2C_JOGGER_H__
#define __I2C_JOGGER_H__

#define PROTOCOL_VERSION 1

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        21 // On Trinket or Gemma, suggest changing this to 1
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 20 // Popular NeoPixel ring size
#define DELAYVAL 200 // Time (in milliseconds) to pause between pixels
#define CYCLEDELAY 12 // Time to wait after a cycle
#define NEO_BRIGHTNESS 10

//flash defines
#define FLASH_TARGET_OFFSET (256 * 1024)
//extern const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
extern const uint8_t *flash_target_contents;

#define STATE_ALARM         1 //!< In alarm state. Locks out all g-code processes. Allows settings access.
#define STATE_CYCLE         2 //!< Cycle is running or motions are being executed.
#define STATE_HOLD          3 //!< Active feed hold
#define STATE_TOOL_CHANGE   4 //!< Manual tool change, similar to #STATE_HOLD - but stops spindle and allows jogging.
#define STATE_IDLE          5 //!< Must be zero. No flags.
#define STATE_HOMING        6 //!< Performing homing cycle
#define STATE_JOG           7 //!< Jogging mode.

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

//neopixel led locations
#define RAISELED 0
#define JOGLED 1
#define SPINLED 2
#define FEEDLED 3
#define HALTLED 4
#define HOLDLED 5
#define RUNLED 6
#define SPINDLELED 7
#define COOLED 8
#define HOMELED 9

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

// Alarm executor codes. Valid values (1-255). Zero is reserved.
typedef enum {
    Alarm_None = 0,
    Alarm_HardLimit = 1,
    Alarm_SoftLimit = 2,
    Alarm_AbortCycle = 3,
    Alarm_ProbeFailInitial = 4,
    Alarm_ProbeFailContact = 5,
    Alarm_HomingFailReset = 6,
    Alarm_HomingFailDoor = 7,
    Alarm_FailPulloff = 8,
    Alarm_HomingFailApproach = 9,
    Alarm_EStop = 10,
    Alarm_HomingRequried = 11,
    Alarm_LimitsEngaged = 12,
    Alarm_ProbeProtect = 13,
    Alarm_Spindle = 14,
    Alarm_HomingFailAutoSquaringApproach = 15,
    Alarm_SelftestFailed = 16,
    Alarm_MotorFault = 17,
    Alarm_AlarmMax = Alarm_MotorFault
} alarm_code_t;

enum Jogmode {FAST = 0,
              SLOW = 1,
              STEP = 2};

extern enum Jogmode current_jogmode;

enum Jogmodify {
    JogModify_1 = 0,
    JogModify_01  = 1,
    JogModify_001 = 2
};
extern enum Jogmodify current_jogmodify;

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

// Define spindle stop override control states.
typedef union {
    uint8_t value;
    struct {
        uint8_t enabled       :1,
                initiate      :1,
                restore       :1,
                restore_cycle :1,
                unassigned    :4;
    };
} spindle_stop_t;

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

enum ScreenMode{
    DEFAULT = 0,
    JOGGING,
    HOMING,
    RUN,
    HOLD,
    JOG_MODIFY,
    TOOL_CHANGE,
    ALARM
};

typedef struct Machine_status_packet {
uint8_t address;
uint8_t machine_state;
uint8_t alarm;
uint8_t home_state;
uint8_t feed_override;
uint8_t spindle_override;
uint8_t spindle_stop;
uint8_t spindle_load;
float spindle_power;
int spindle_rpm;
float feed_rate;
coolant_state_t coolant_state;
uint8_t jog_mode;  //includes both modifier as well as mode
float jog_stepsize;
coord_system_id_t current_wcs;  //active WCS or MCS modal state
float x_coordinate;
float y_coordinate;
float z_coordinate;
float a_coordinate;
} Machine_status_packet;

#define OVERRIDE_INCREMENT 10
typedef struct Pendant_count_packet {
int32_t uptime;
uint8_t feed_over;
uint8_t spindle_over;
uint8_t rapid_over;
uint32_t buttons;
int32_t x_axis;
int32_t y_axis;
int32_t z_axis;
int32_t a_axis;
} Pendant_count_packet;

//maximum jogging accelerations
typedef struct Pendant_accel_packet {
float x_axis;
float y_axis;
float z_axis;
float a_axis;
} Pendant_accel_packet;

typedef struct Pendant_memory_map {
    Machine_status_packet statuspacket;
    Pendant_count_packet countpacket;
    Pendant_accel_packet accelpacket;
    char toolcomment[48];
    //last 4 bytes are reserved for version info
} Pendant_memory_map;

extern Pendant_memory_map * pendant_memory_ptr;

extern ScreenMode screenmode;
extern Machine_status_packet *packet;
extern Machine_status_packet prev_packet;
extern Jogmode previous_jogmode;
extern Jogmodify previous_jogmodify;
extern ScreenMode previous_screenmode;

extern bool screenflip;
extern int command_error;
extern float step_calc;

//extern Adafruit_NeoPixel pixels;


void draw_string(char * str);
void draw_main_screen(Machine_status_packet *previous_packet, Machine_status_packet *packet, bool force);
void update_neopixels(Machine_status_packet *previous_packet, Machine_status_packet *packet);
void init_screen (void);
void init_neopixels (void);
void write_nvs (void);

#if defined(_LINUX_) && defined(__cplusplus)
}
#endif // _LINUX_

#endif // 

