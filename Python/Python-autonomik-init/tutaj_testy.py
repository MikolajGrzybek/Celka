import tkinter as tk
from tkinter import scrolledtext
import tk_tools
import time
import serial
import datetime

from struct import *
from decoder.serial_protocol_decoder import *

import cstruct

window = tk.Tk()

leak_led = None
temp_err_led = None
overhead_led = None
dry_run_led = None
timeout_led = None
var1 = None

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
    if command == 50:
        #time = unpack("<I", data)
        print ("stm:" + str(data.decode()).rstrip('\x00'))# remove null termination of string)
    
    if command == 8:
        if( data[0] == 2 ): dry_run_led.to_red()
        if( data[0] == 1 ): temp_err_led.to_red()
        if( data[0] == 3 ): leak_led.to_red()
        if( data[0] == 0 ): overhead_led.to_red()
        if( data[0] == 5 ): timeout_led.to_red()



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
        dry_run_led.to_green()
        temp_err_led.to_green()
        leak_led.to_green()
        overhead_led.to_green()
        timeout_led.to_green()

        getUartData()
        decoder.parsePackets()
        if(var1.get() == 0):
            decoder.sendMsg(6, pack("<hhhh", int(temperature_1.get()), int(temperature_2.get()), input_flow.get(), output_flow.get(),))  # h - int16_t
        #decoder.sendMsg(7, pack("<hhhh", int(temperature_1.get()), int(temperature_2.get()), input_flow.get(), output_flow.get(),))
        print("sended")
    except Exception as e:
        print('Handling UART com port error', e, "\n try select com and connect od reconnect cable\n")

    #https://docs.python.org/3/library/struct.html


    window.after(500, updateOut)




def pump_on():
    global pump_mode
    decoder.sendMsg(7, pack("<h", 0,))
    pump_mode.set("ON")
def pump_off():
    global pump_mode
    decoder.sendMsg(7, pack("<h", 1,))
    pump_mode.set("OFF")
def pump_auto():
    global pump_mode
    decoder.sendMsg(7, pack("<h", 2,))
    pump_mode.set("AUTO")


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
window.geometry("400x190")

#TEMPERATURE
temperature_1 = tk.Scale(window, from_=0, to=100)
temperature_1.place(x=10, y=80)
t1= tk.Label( window, text = "T1" )
t1.place(x=10, y=70)

temperature_2 = tk.Scale(window, from_=0, to=100)
temperature_2.place(x=50, y=80)
t2= tk.Label( window, text = "T2" )
t2.place(x=50, y=70)

#FLOW
input_flow = tk.Scale(window, from_=0, to=100)
input_flow.place(x=90, y=80)
flow_in= tk.Label( window, text = "IN" )
flow_in.place(x=90, y=70)

output_flow = tk.Scale(window, from_=0, to=100)
output_flow.place(x=130, y=80)
flow_out= tk.Label( window, text = "OUT" )
flow_out.place(x=130, y=70)

# COM przycisk i lista dostępnych
buttonON = tk.Button(window, text='PUMP ON', width=10, bg="snow",command=pump_on)
buttonON.place(x=20, y=40)

# COM przycisk i lista dostępnych
buttonOFF = tk.Button(window, text='PUMP OFF', width=10, bg="snow",command=pump_off)
buttonOFF.place(x=110, y=40)

# COM przycisk i lista dostępnych
buttonAUTO = tk.Button(window, text='PUMP AUTO', width=10, bg="snow",command=pump_auto)
buttonAUTO.place(x=200, y=40)

# COM przycisk i lista dostępnych
buttonCOM = tk.Button(window, text='Connect', width=10, bg="snow",command=connectuart)
buttonCOM.place(x=200, y=150)

#PUMP

pump_str = "PUMP MODE:"
Battery= tk.Label( window, text = pump_str )
Battery.place(x=20, y=10)

pump_mode = tk.StringVar()
pump_mode.set("AUTO")
pump_l = tk.Label( window, textvariable = pump_mode )
pump_l.place(x=100, y=10)

#serial port

COMList = ["COM#",]
optvariable = tk.StringVar(window)
optvariable.set(COMList[0])
opt = tk.OptionMenu(window, optvariable, *COMList, command=connectuart)
opt.config(width=7, bg="snow")
opt.place(x=200, y=80)
opt.bind("<Button-1>", serial_ports_get)

#leds
leak_txt= tk.Label( window, text = "LEAK" )
leak_txt.place(x=330, y=135)
leak_led = tk_tools.Led(window, size=20)
leak_led.place(x=300, y=130)

temp_err_txt= tk.Label( window, text = "TEMP ERR" )
temp_err_txt.place(x=330, y=105)
temp_err_led = tk_tools.Led(window, size=20)
temp_err_led.place(x=300, y=100)

overheat_txt= tk.Label( window, text = "OVERHEAT" )
overheat_txt.place(x=330, y=75)
overhead_led = tk_tools.Led(window, size=20)
overhead_led.place(x=300, y=70)

dry_run_txt= tk.Label( window, text = "DRY RUN" )
dry_run_txt.place(x=330, y=45)
dry_run_led = tk_tools.Led(window, size=20)
dry_run_led.place(x=300, y=40)

timeout_txt= tk.Label( window, text = "TIMEOUT" )
timeout_txt.place(x=330, y=15)
timeout_led = tk_tools.Led(window, size=20)
timeout_led.place(x=300, y=10)

var1 = tk.IntVar()
stop_send = tk.Checkbutton(window, text='Don\'t send',variable=var1, onvalue=1, offvalue=0 )
stop_send.place(x = 150, y = 10)

window.after(1000,updateOut)

window.mainloop()
