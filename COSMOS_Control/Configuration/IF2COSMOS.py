import requests
import json
import sys

from Utilities import InputOutput as io


P2S = 'y' #Global variable for output in shell
class IF2COSMOS:
    def __init__(self, configurationFile, subsystem):
        configData = io.readJson(configurationFile)
        self.subsystem = subsystem
        self.config = configData['subsystems'][self.subsystem]
        self.url = "http://localhost:2900/openc3-api/api"
        self.headers = {'content-type': 'application/json', 'Authorization': 'cst4r'}
        self.payload = {
            "jsonrpc": "2.0",
            "method": "",
            "params": [],
            "id": 1, "keyword_params": {"scope": "DEFAULT"}
        }


    def sendCommand(self, commandName, parameterNames=[], parameterValues=[]):
        io.prnt(f'Sending command {commandName} for {self.subsystem}', P2S)
        if isinstance(parameterNames, str):
                print("1")
        if type(parameterValues) != list:
                print("2")    
        if commandName not in self.config['tc']:
                raise TypeError('Command not specified in json file!')   
        if parameterNames:
                print("3")
                for para in parameterNames:
                    print(self.config['tc'][commandName])
                    if para not in self.config['tc'][commandName]['parameter_names']:
                        raise TypeError(f'Command parameter {para} not specified or wrong written')
        try:
            if isinstance(parameterNames, str):
                print("1")
                parameterNames = [parameterNames]

            if type(parameterValues) != list:
                print("2")
                parameterValues = [parameterValues]

            if commandName not in self.config['tc']:
                raise TypeError('Command not specified in json file!')
            if parameterNames:
                print("3")
                for para in parameterNames:
                    print(self.config['tc'][commandName])
                    if para not in self.config['tc'][commandName]['parameter_names']:
                        raise TypeError(f'Command parameter {para} not specified or wrong written')

        except Exception as e:
            if type(e).__name__ == 'TypeError':
                print('Error: Command is missing or name of command is wrong.')
                sys.exit(27)
            else:
                print('Unknown Error in function IF2COSMOS/sendCommand')
                sys.exit(1)

        else:

            self.payload['method'] = "cmd"
            if parameterNames:
                parameterDict = {}
                for i in range(len(self.config['tc'][commandName]['parameter_names'])):
                    if parameterValues:
                        parameterDict[self.config['tc'][commandName]['parameter_names'][i]] = parameterValues[i]
                    else:
                        parameterDict[self.config['tc'][commandName]['parameter_names'][i]] = self.config['tc'][commandName]['parameter_values'][i]

                self.payload['params'] = [f"{self.subsystem}", f"{commandName}", parameterDict]
            else:
                self.payload['params'] = [f"{self.subsystem}", f"{commandName}"]

            response = self.__getResponse(self.payload)

            self.payload['method'] = ""
            self.payload['params'] = []
            io.prnt(f'Command {commandName} for {self.subsystem} sent', P2S)
            return response

    def receiveTelemetry(self, tmPacket, parameter=[]):
        io.prnt(f'Receiving TM for {tmPacket} of {self.subsystem}', P2S)

        if isinstance(parameter, str):
            parameter = [parameter]

        if not parameter:
            for k, v in self.config['tm'].items():
                parameter.append(k)
        try:
            if tmPacket not in self.config['tm']:
                raise TypeError('TM packet not specified in json file!')
            for para in parameter:
                io.prnt(para)
                if para not in self.config['tm'][tmPacket]:
                    raise TypeError(f'TM parameter {para} not specified in json config file!')

        except Exception as e:
            if type(e).__name__ == 'TypeError':
                print('Error: TM packet or parameter is missing or name is wrong.')
                sys.exit(27)
            else:
                print('Unknown Error in function IF2COSMOS/receiveTelemetry')
                sys.exit(1)

        else:

            self.payload['method'] = "tlm"

            response = {}

            for para in parameter:
                self.payload['params'] = [f"{self.subsystem}", f"{tmPacket}", f"{para}"]
                receivedData = self.__getResponse(self.payload)
                if 'error' in receivedData:
                    print('DIOBONO', receivedData['error'])
                else:
                    if self.config['tm'][tmPacket][para]['type'].lower() == 'block':
                        response[para] = self.__getResponse(self.payload)['result']['raw']
                    else:
                        response[para] = self.__getResponse(self.payload)['result']
            self.payload['method'] = ""
            self.payload['params'] = []

            io.prnt(f'Received TM for {para} of {tmPacket} of {self.subsystem}', P2S)
            return response
    def __getResponse(self, payload):
        response = requests.post(self.url, data=json.dumps(payload), headers=self.headers).json()
        return response

    def tm2File(self, data, tmpacket, keys=[], fileNames=[], mode='w'):
        io.prnt(f'Saving data to file(s)!', P2S)
        if isinstance(fileNames, str):
            fileNames = [fileNames]

        if isinstance(keys, str):
            keys = [keys]

        if not keys:
            for k, v in data.items():
                keys.append(k)
                fileNames.append(k + '.txt')

        keysToFilenames = {}


        if fileNames:
            for i in range(len(keys)):
                keysToFilenames[keys[i]] = fileNames[i]
        else:
            for i in range(len(keys)):
                fileNames.append(keys[i] + '.txt')
                keysToFilenames[keys[i]] = fileNames[i]

        print(keysToFilenames)
        convertedData = {}
        for k, v in data.items():
            if k in keys:
                #print(k)
                #convertToType = self.config['tm'][tmpacket][k]['convert_to']
                #convertFromType = self.config['tm'][tmpacket][k]['unit']

                #convertedData[k] = Conversion.Conversion.dataTypeConversion(v, convertFromType, convertToType)
                #print(convertedData[k])
                #io.saveDataToFile(convertedData[k], keysToFilenames[k], mode)
                io.saveDataToFile(v, keysToFilenames[k], mode)
        io.prnt('Data saved!', P2S)

# Press the green button in the gutter to run the script.
if __name__ == '__main__':


    dict = {'BIROS_DATA': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]}

    #tm2File(self, data, tmpacket, keys=[], fileName=[], mode='w'):




