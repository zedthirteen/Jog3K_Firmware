from MachineStatusPacket import MachineStatusPacket, example_binary_data

x = MachineStatusPacket.parse(bytes(MachineStatusPacket.sizeof()))

x.address = 62
x.machine_state = 1
x.machine_substate = 0
x.axes_signals = 0xFF
x.feed_override = 125
x.spindle_override = 78
x.spindle_stop = 0
x.spindle_state = 0
x.spindle_rpm = 1235
x.feed_rate = 150.65
x.coolant_state = 1
x.jog_mode = 0
x.control_signals = 0
x.jog_stepsize = 1500
x.current_wcs = 0
x.axes_limits = 0
x.status_code = 0
x.machine_mode = 0
x.x_coordinate = 100.35
x.y_coordinate = 6587.12
x.z_coordinate = -1.25
x.a_coordinate = 0
x.message_type = 0

packed_byte_array = MachineStatusPacket.build(x)

print(packed_byte_array)
print(bytes(packed_byte_array))