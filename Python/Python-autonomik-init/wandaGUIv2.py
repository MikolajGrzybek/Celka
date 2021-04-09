import tkinter as tk
from tkinter import scrolledtext
import time
import serial
import datetime

from struct import *
from decoder.serial_protocol_decoder import *

import cstruct

window = tk.Tk()
pitch_val = None
roll_val = None
yaw_val = None
battery_val = None
us_right_val = None
us_left_val = None
class IMU_Data(cstruct.CStruct):
    __byte_order__ = cstruct.LITTLE_ENDIAN
    __struct__ = """
		uint16_t imu_roll;
		uint16_t imu_pitch;
		uint16_t imu_yaw;
		uint16_t imu_acc_x;
		uint16_t imu_acc_y;
		uint16_t imu_acc_z;
    """

class battery_Data(cstruct.CStruct):
    __byte_order__ = cstruct.LITTLE_ENDIAN
    __struct__ = """
        uint8_t battery_level;
"""

class US_Data(cstruct.CStruct):
    __byte_order__ = cstruct.LITTLE_ENDIAN
    __struct__ = """
    uint16_t ultrasonic_left;
    uint16_t ultrasonic_right;
    """
class US_Data_2_0(cstruct.CStruct):
    __byte_order__ = cstruct.LITTLE_ENDIAN
    __struct__ = """
    uint8_t us_left_raw;      // [cm]
	uint8_t us_left;          // [cm]
	uint8_t us_right_raw;     // [cm]
	uint8_t us_right;         // [cm]
	uint8_t stick_left_raw;   // [cm]
	uint8_t stick_left;       // [cm]
	uint8_t stick_right_raw;  // [cm]
	uint8_t stick_right;      // [cm]
    """

#decoder
def getUartData():
    len = ser.in_waiting
    print('uart rx: ',  len)
    if len > 0 :
		
        inputData = ser.read(len)
        binaryInputData = bytearray(inputData)
        decoder.put(binaryInputData)
        decoder.parsePackets()

#to się wykonuje gdy odbierzemy poprawny pakiet
#data - bytearray
#https://docs.python.org/3/library/struct.html
def parsePacketCallback(command,data):
    global roll_val 
    global pitch_val 
    global yaw_val
    global battery_val
    global us_left_val
    global us_right_val
    imu_data = IMU_Data()
    bat_data = battery_Data() 
    us_data = US_Data_2_0()
    if command == 4:
        #time = unpack("<I", data)
        #print ("stm:" + str(data.decode()).rstrip('\x00'))# remove null termination of string)
        bat_data.unpack(data)
        battery_val.set(str(bat_data.battery_level))
    if command == 1:
        #time = unpack("<I", data)
        imu_data.unpack(data)
        #print ("stm: imu: Roll "+str(imu_data.imu_roll) + " pitch "+str(imu_data.imu_pitch) + " pitch "+str(imu_data.imu_yaw) )# remove null termination of string)
        pitch_val.set(str(imu_data.imu_pitch))
        roll_val.set(str(imu_data.imu_roll))
        yaw_val.set(str(imu_data.imu_yaw))

    if command == 50:
        #time = unpack("<I", data)
        print ("stm:" + str(data.decode()).rstrip('\x00'))# remove null termination of string)

    if command == 6:
        us_data.unpack(data)
        us_left_val.set(str(us_data.us_left))
        us_right_val.set(str(us_data.us_right))


def uartSend(bytes):
    #ser.flush()
    ser.write(bytes)

decoder = Decoder(uartSend, getUartData, parsePacketCallback)

def serial_ports():
    ports = ['COM%s' % (i + 1) for i in range(256)]
    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

def logTime():
    hour = time.strftime("%H")
    minute = time.strftime("%M")
    second = time.strftime("%S")
    varTime = "{}:{}:{}".format(hour, minute, second)
    return varTime





def updateOut():

    #save do loga
    try:
        getUartData()
        decoder.parsePackets()
        decoder.sendMsg(0, pack("<hh", int(left_power.get()), int(right_power.get()), ))  # h - int16_t
        print("sended")
    except Exception as e:
        print('Handling UART com port error', e, "\n try select com and connect od reconnect cable\n")

    #https://docs.python.org/3/library/struct.html


    window.after(200, updateOut)








def connectuart():
    global comPort
    global connectionStatus
    global ser

    linelog = ""

    if optvariable.get() != 'COM#':
        if comPort != optvariable.get():
            comPort = optvariable.get()
            connectionStatus = 0
            ser = serial.Serial(comPort)
            ser.baudrate = 115200
            print("connect to: " + comPort)
            linelog = "connected to {} SUCCESS".format(comPort)

    else:
        linelog = "connected to {} FAILED".format(comPort)
        print("set propel COM port")



def serial_ports_get(self):
    opt['menu'].delete(0, 'end')
    comList = serial_ports()
    #print(comList)
    for x in comList:
        add_comport(x)

def _print(var, string):
    var.set(string)

def add_comport(portName):
    opt['menu'].add_command(label=portName, command=lambda: _print(optvariable, portName))

comPort = 'COM#'
connectionStatus = 0
ser = serial.Serial()
setPoint = 90


window.title("WANDA GUI v2")
window.geometry("320x190")

#ENGINE POWER
left_power = tk.Scale(window, from_=-1000, to=1000)
left_power.place(x=50, y=80)

right_power = tk.Scale(window, from_=-1000, to=1000)
right_power.place(x=10, y=80)

# COM przycisk i lista dostępnych
buttonCOM = tk.Button(window, text='Connect', width=10, bg="snow",command=connectuart)
buttonCOM.place(x=200, y=150)

#BATTERY

battery_str = "BATTERY"
Battery= tk.Label( window, text = battery_str )
Battery.place(x=120, y=10)

battery_val = tk.StringVar()
battery_val.set("0")
battery_l = tk.Label( window, textvariable = battery_val )
battery_l.place(x=180, y=10)


#ULTRASONIC
us_left_str = "LEFT"
us_left= tk.Label( window, text = us_left_str )
us_left.place(x=120, y=30)

us_left_val = tk.StringVar()
us_left_val.set("0")
us_left_l = tk.Label( window, textvariable = us_left_val )
us_left_l.place(x=180, y=30)

us_right_str = "RIGHT"
us_right= tk.Label( window, text = us_right_str )
us_right.place(x=120, y=50)

us_right_val = tk.StringVar()
us_right_val.set("0")
us_right_l = tk.Label( window, textvariable = us_right_val )
us_right_l.place(x=180, y=50)

#IMU 
pitch_str = "PITCH"
ImuPitch = tk.Label( window, text = pitch_str )
ImuPitch.place(x=30, y=10)
roll_str = "ROLL"
ImuRoll = tk.Label( window, text = roll_str )
ImuRoll.place(x=30, y=30)
yaw_str = "YAW"
ImuYaw = tk.Label( window, text = yaw_str )
ImuYaw.place(x=30, y=50)

#IMU VALUES

pitch_val = tk.StringVar()
pitch_val.set("0")
ImuPitch_val = tk.Label( window, textvariable = pitch_val )
ImuPitch_val.place(x=70, y=10)

roll_val = tk.StringVar()
roll_val.set("0")
ImuRoll_val = tk.Label( window, textvariable = roll_val )
ImuRoll_val.place(x=70, y=30)

yaw_val = tk.StringVar()
yaw_val.set("0")
ImuYaw_val = tk.Label( window, textvariable = yaw_val )
ImuYaw_val.place(x=70, y=50)

#serial port

COMList = ["COM#",]
optvariable = tk.StringVar(window)
optvariable.set(COMList[0])
opt = tk.OptionMenu(window, optvariable, *COMList, command=connectuart)
opt.config(width=7, bg="snow")
opt.place(x=200, y=80)
opt.bind("<Button-1>", serial_ports_get)


window.after(1000,updateOut)

window.mainloop()
