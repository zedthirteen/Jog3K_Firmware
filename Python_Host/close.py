from pySerialTransfer import pySerialTransfer as txfer

link = txfer.SerialTransfer('COM16')

link.open()
link.close()