
from construct import Struct, Enum, Byte, Float32l, Int32ul, Int16ul, BitStruct, Bit, Padding, Flag

from construct import Container, RawCopy

# Define an empty Container
empty_container = Container()

# Define enums
AlarmCode = Enum(Byte,
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
    Alarm_AlarmMax = 17
)

CoordSystemId = Enum(Byte,
    CoordinateSystem_G54 = 0,
    CoordinateSystem_G55 = 1,
    CoordinateSystem_G56 = 2,
    CoordinateSystem_G57 = 3,
    CoordinateSystem_G58 = 4,
    CoordinateSystem_G59 = 5,
    CoordinateSystem_G28 = 9,
    CoordinateSystem_G30 = 10,
    CoordinateSystem_G92 = 11
)

ScreenMode = Enum(Byte,
    DEFAULT = 0,
    JOGGING = 1,
    HOMING = 2,
    RUN = 3,
    HOLD = 4,
    JOG_MODIFY = 5,
    TOOL_CHANGE = 6,
    ALARM = 7
)

# Define bit structures
CoolantState = BitStruct(
    "flood" / Flag,
    "mist" / Flag,
    "shower" / Flag,
    "trough_spindle" / Flag,
    Padding(4)
)

SpindleStop = BitStruct(
    "enabled" / Flag,
    "initiate" / Flag,
    "restore" / Flag,
    "restore_cycle" / Flag,
    Padding(4)
)

# Define machine status packet structure
MachineStatusPacket = Struct(
    "address" / Int16ul,
    "machine_state" / Byte,
    "machine_substate" / Byte,
    "home_state" / Byte,
    "feed_override" / Int16ul,
    "spindle_override" / Int16ul,
    "spindle_stop" / Byte,
    "spindle_state" / Byte,
    "spindle_rpm" / Int32ul,
    "feed_rate" / Float32l,
    "coolant_state" / Byte,
    "jog_mode" / Byte,  # You might need to split this into mode and modifier if necessary
    "control_signals" / Int32ul,
    "jog_stepsize" / Float32l,
    "current_wcs" / Byte,
    "axes_limits" / Byte,
    "status_code" / Byte,
    "machine_mode" / Byte,
    "x_coordinate" / Float32l,
    "y_coordinate" / Float32l,
    "z_coordinate" / Float32l,
    "a_coordinate" / Float32l,    
    "message_type" / Byte
)

