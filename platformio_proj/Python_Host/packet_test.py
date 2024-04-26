from time import sleep
from pySerialTransfer import pySerialTransfer as txfer
from MachineStatusPacket import MachineStatusPacket, example_binary_data

x = MachineStatusPacket.parse(bytes(MachineStatusPacket.sizeof()))

x.address = 6283
x.machine_state = 1
x.machine_substate = 0
x.home_state = 0xFF
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
# Convert parsedata back into the format of example_binary_data
converted = [hex(byte) for byte in packed_byte_array]

if __name__ == '__main__':
    try:
        testStruct = packed_byte_array
        print(testStruct)
        
        
        link = txfer.SerialTransfer('COM16')
        
        link.open()
        #sleep(1)
    

        sendSize = 0
        print(sendSize)
        
        for index in range(MachineStatusPacket.sizeof()):
            link.txBuff[index] = testStruct[index]
            
        #sendSize = link.tx_obj(testStruct, start_pos=sendSize)
        #sendSize = link.tx_obj(testStruct.y, start_pos=sendSize)
        sendSize = MachineStatusPacket.sizeof()
        print(sendSize)
        
        link.send(sendSize)
        
    except KeyboardInterrupt:
        link.close()