
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

x = MachineStatusPacket

# Example usage:
# Example binary data (This needs to match the expected format and length of your structure)
example_binary_data = [
    0x00, 0x00, # address
    0x00, # machine_state
    0x00, # machine_substate
    0x00, # axes_signals
    0x00, 0x00, # feed_override (100)
    0x00, 0x00, # spindle_override (100)
    0x00, # spindle_stop (example value)
    0x00, # spindle_state
    0x00, 0x00, 0x00, 0x00, # spindle_rpm (0)
    0x00, 0x00, 0x00, 0x00, # feed_rate (0.0)
    0x00, # coolant_state (example value)
    0x00, # jog_mode
    0x00, 0x00, 0x00, 0x00, # control_signals (0)
    0x00, 0x00, 0x00, 0x00, # jog_stepsize (0.0)
    0x00, # current_wcs
    0x00, # axes_limits
    0x00, # status_code
    0x00, # machine_mode
    0x00, 0x00, 0x00, 0x00, # x_coordinate (0.0)
    0x00, 0x00, 0x00, 0x00, # y_coordinate (0.0)
    0x00, 0x00, 0x00, 0x00, # z_coordinate (0.0)
    0x00, 0x00, 0x00, 0x00, # a_coordinate (0.0)
    0x00  # message_type    
]

parsedata = bytes(example_binary_data)

# Convert parsedata back into the format of example_binary_data
converted = [format(byte, '02X') for byte in parsedata]

# Parsing the binary data
#parsed_data = x.parse(parsedata)

# Print parsed data to see the result
#print(parsed_data)
#parsed_data.address = 5878
#print(parsed_data)
# Pack the modified data back into a byte array
#packed_byte_array = x.build(parsed_data)
#print(packed_byte_array)

#parsed_data2 = x.parse(packed_byte_array)
#print(parsed_data2)