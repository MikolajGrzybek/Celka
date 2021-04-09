from struct import *

import time

class fletcher16:
    __c0=0;
    __c1=0;
    def reset(self):
        self.__c0 = 0x12
        self.__c1 = 0x34

    def compute(self, oneByte):
        #fletcher16(const uint8_t *data, size_t len) from wikipedia
        #//this work up to 5000 bytes. Above just call somewhere crcGet to calculate modulo.
        self.__c0 = self.__c0 + oneByte;
        self.__c1 = self.__c1 + self.__c0;

    def get(self):
        self.__c0 = self.__c0 % 255;
        self.__c1 = self.__c1 % 255;
        crc =bytearray([self.__c0, self.__c1]);
        return crc

    def packetCRC(self, data, dataId, dataLen):
        self.reset()
        self.compute(dataId)
        self.compute(dataLen)
        for byte in data:
            self.compute(byte)
        return self.get()


class Decoder:
    __id = bytearray()
    __ok = False
    __crc = fletcher16()
    __byteArray=bytearray()
    __START_OF_PACKET = bytearray([0xC0, 0xFE])  # //coffe
    __END_OF_PACKET = bytearray([0xED])  # //EnD
    __DATA_COMMAND_TYPE= bytearray(1)
    __DATA_LEN_TYPE = bytearray(1)
    __DATA_CRC_TYPE = bytearray(2)
    __bytesSend=0
    __parsePacketCallback=0
    __getUartData=0



    #bytesSend(bytesarray)
    #parsePacketCallback(dataIdInt, byteArray[headerSize : headerSize + dataLenInt]);
    def __init__(self, sendUartData, getUartData, parsePacketCallback):
        self.__bytesSend = sendUartData
        self.__getUartData = getUartData
        self.__parsePacketCallback = parsePacketCallback

    def put(self, data):
        if type( data) != type(bytearray() ):
            raise " use bytearray() to convert data "
        self.__byteArray += data

    #dataid: 0-255 data: bytes or bytearray 0-255bytes. Use pack(">c", data)
    def sendMsg(self, dataId, data):
        if dataId > 255:
            raise " dataId > 255"
        if len(data) > 255:
            raise " len(data) > 255:"
        if type( data) != type(bytearray() ):
            if type( data) != type(pack("I",0 )):
                print ("datatype" + str(type( data)) + "excepted" + str(type(bytearray())))
                raise " use pack() to serialize data"

        dataLen=len(data);
        crc = self.__crc.packetCRC(data, dataId, dataLen);
        bufferOffset =0;
        serialData = self.__START_OF_PACKET
        serialData = serialData + bytearray([dataId])
        serialData = serialData + bytearray([dataLen])
        serialData = serialData + crc
        serialData = serialData + data
        serialData = serialData + self.__END_OF_PACKET;
        #https://docs.python.org/3/library/struct.html
        #bufferTx = pack(">c", dataId);
        #bufferTx = pack(">c", dataLen);
       # bufferTx = pack(">H",  crc);
        self.__bytesSend(serialData)

    def parsePackets(self):
        headerSize = len(self.__START_OF_PACKET) + len (self.__DATA_COMMAND_TYPE) + len (self.__DATA_LEN_TYPE) + len (self.__DATA_CRC_TYPE)
        minPacketLen = headerSize + len(self.__END_OF_PACKET)
        #iterate over packets
        while (len(self.__byteArray) > minPacketLen):
            #find start sequence
            while(True):
                if(len(self.__byteArray) <= minPacketLen):
                    return
                    #buffer almost empty
                header = self.__byteArray[0: len (self.__START_OF_PACKET)]
                if (header == self.__START_OF_PACKET):
                    arrayIndex = len(self.__START_OF_PACKET);
                    dataId = self.__byteArray[arrayIndex: arrayIndex + len (self.__DATA_COMMAND_TYPE)]
                    arrayIndex += len(self.__DATA_COMMAND_TYPE)
                    dataLen = self.__byteArray[arrayIndex: arrayIndex + len (self.__DATA_LEN_TYPE)]
                    arrayIndex += len(self.__DATA_LEN_TYPE)
                    dataCRC = self.__byteArray[arrayIndex: arrayIndex + len (self.__DATA_CRC_TYPE)]
                    #arrayIndex += len(self.DATA_CRC_TYPE)
                    #dataId, dataLen, crc = unpack(">ccH", byteArray[len(START_OF_PACKET): len(START_OF_PACKET) + 4]);
                    break;
                else:
                    #print(int(self.__byteArray[0]))
                    del self.__byteArray[0]#shift buffer
            dataLenInt = int.from_bytes(dataLen, byteorder='big')
            dataIdInt  = int.from_bytes(dataId,  byteorder='big')
            if len(self.__byteArray) < headerSize + dataLenInt+1:
                return; #wait for more data
            endPacket = self.__byteArray[headerSize + dataLenInt: headerSize + dataLenInt + len(self.__END_OF_PACKET)]
            if endPacket != self.__END_OF_PACKET:
                del self.__byteArray[0];
                print("parse packet " +str(dataIdInt)+" len " + str(dataLenInt) + " tail error");
                return;
            computedCRC= self.__crc.packetCRC(self.__byteArray [headerSize: headerSize + dataLenInt], dataIdInt, dataLenInt);
            if dataCRC != computedCRC:
                del self.__byteArray[0];
                print("parse packet " +str(dataIdInt)+" len " + str(dataLenInt) + " crc error");
                return;

            #parse complette, do some work with data
            dataByteArray = self.__byteArray[headerSize: headerSize + dataLenInt]
            if dataIdInt == 255:
                if (dataByteArray == self.__id):
                    self.__ok = True
            else:
                self.__parsePacketCallback(dataIdInt, dataByteArray);
            del self.__byteArray[0: headerSize + dataLenInt + 1];

        #send data and wait for response at command = 255. Return true / false after some tries
    def sendAndConfirm(self, dataid, data):
        self.__ok = False
        timeout = 0;
        self.__id = bytearray([dataid])
        while timeout < 100:
            #self.sendMsg(255, self.__id)
            self.sendMsg(dataid, data)
            timeout += 1
            for x in range(0, 200):
                time.sleep(0.005)
                self.__getUartData()
                if self.__ok == True:
                    print( "command: > " + str(dataid) + " < sent and confirmed")
                    return True
            print( "uart fail, command: " + str(dataid) + "\ttries: " + str(timeout))
        return False


