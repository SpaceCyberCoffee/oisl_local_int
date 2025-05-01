import struct
import Utilities.InputOutput as io

printOut = 'y'

class Conversion:
    @staticmethod
    def convertDataDict(data, tmpacket, configFile, keys=[]):
        if isinstance(keys, str):
            keys = [keys]

        if not keys:
            for k, v in data.items():
                keys.append(k)

        convertedData = {}
        for k, v in data.items():
            if k in keys:
                print(k)
                convertToType = configFile['tm'][tmpacket][k]['convert_to']
                convertFromType = configFile['tm'][tmpacket][k]['unit']

                convertedData[k] = Conversion.dataTypeConversion(v, convertFromType, convertToType)
                print(convertedData[k])
        return convertedData

    @staticmethod
    def dataTypeConversion(dataToConvert, convertFromType, convertToType):
        convertedData = []

        #decodedList = decodeHex(dataToConvert)
        io.prnt('This is what I get', printOut)
        #io.prnt(dataToConvert, printOut)
        if convertToType == 'double':
            if convertFromType == 'uint32':
                rawData = Conversion.__decodeHex(dataToConvert)
                values = []
                for i in range(0, len(rawData), 4):
                    sublist = []
                    for j in range(0, 4):
                        sublist.append(rawData[i + j])
                    values.append(''.join(sublist[::-1]))
                convertedData = Conversion.__convertUint32ToDouble(values)
        elif convertToType == 'uint32':
            if convertFromType == 'uint32':
                if type(dataToConvert) != list:
                    convertedData = dataToConvert
                else:
                    rawData = Conversion.__decodeHex(dataToConvert)
                    io.prnt("Conversion started!")
                    io.prnt(rawData)

                    convertedData = Conversion.__convertHexDataInt(rawData)
        # Added more cases here
        #elif convertToType == 'int':

        else:
            print(f"Conversion to type {convertToType} not implemented!")

        return convertedData

    @staticmethod
    def __convertUint32ToDouble(receivedIntegers):
        doubles = []
        #print('This are the received integers:')
        #print(receivedIntegers)
        for integer in receivedIntegers:

            int_conv = int(integer, 16)
            if (int_conv) == 0:
                doubles.append(0.0)
            else:
                sign = int(str(int_conv)[0])
                number = int(str(int_conv)[1:])
                float_conv = number / 100000.0
                if sign == 1:
                    float_conv *= -1.0
                doubles.append(float_conv)
        io.prnt(doubles, printOut)
        return doubles

    @staticmethod
    def __convertHexDataInt(dataList):

        byteList = bytes.fromhex(''.join(dataList))
        format = '<' + 'I' * (len(byteList) // 4)
        decodedHex = struct.unpack_from(format, byteList)

        return decodedHex

    @staticmethod
    def __decodeHex(data):
        # format = '%dB' % len(data)
        # decodedHex = struct.unpack_from(format, data)
        # print(decodedHex)

        hex_list = [f"{x:02x}" for x in data]

        io.prnt(hex_list, printOut)

        return hex_list





# Press the green button in the gutter to run the script.
if __name__ == '__main__':


    x = 1



[8, 0, 0, 0, 10, 0, 0, 0, 12, 0, 0, 0, 14, 0, 0, 0, 16, 0, 0, 0, 18, 0, 0, 0, 20, 0, 0, 0, 22, 0, 0, 0, 24, 0, 0, 0, 26, 0, 0, 0, 28, 0, 0, 0, 30, 0, 0, 0, 32, 0, 0, 0, 34, 0, 0, 0, 36, 0, 0, 0, 38, 0, 0, 0, 40, 0, 0, 0, 42, 0, 0, 0, 44, 0, 0, 0, 46, 0, 0, 0, 48, 0, 0, 0, 50, 0, 0, 0, 52, 0, 0, 0, 54, 0, 0, 0, 56, 0, 0, 0]