#!/usr/bin/python3


import time
from datetime import datetime, timedelta
import os

if __name__ == "__main__":
    import Configuration.IF2COSMOS as if2c
    import Utilities.Conversion as conv

    command_frequency = 30
    configFile = '../Configuration/config_files/satellite_config.json'

    tmDataFolder = f'tm_data/LUCA/CLOCK'  

    sample = if2c.IF2COSMOS(configFile, 'SAMPLE_DEBUG')
    print('Loaded config file!')

    numberOfRuns = 10000
    numberOfDataPointsLimit = 498
    cnt = 0

    waitForNextRequest = 60
    waitForCheckingDataCount = 60

    normal_start_time = datetime(year=2025, month=10, day=18, hour=8, minute=30)

    while True:  # cnt != numberOfRuns:
        time.sleep(waitForNextRequest)

        dataIndex = 0
        dataSaved = False

        while not dataSaved:

            response = sample.sendCommand('SAMPLE_REQ_HK')
            responseFile = sample.receiveTelemetry('SAMPLE_HK_TLM', ['TIME_STEPS_CNT', 'BATT_CURR_A_DATA',
                                                                     'TH_CONT_UH_DATA', 'BOXES_CURR_DATA',
                                                                     'AMBIENT_T_DATA', 'BATT_CURR_A_DATA_2',
                                                                     'TH_CONT_UH_DATA_2', 'BOXES_CURR_DATA_2',
                                                                     'AMBIENT_T_DATA_2'])

            # timetowrite = current_sim_time.strftime('%Y-%m-%d %H:%M:%S.%f')
            print(responseFile['TIME_STEPS_CNT'])
            # print(responseFile['SOC_DATA'])
            previous_dataIndex = dataIndex
            dataIndex = responseFile['TIME_STEPS_CNT']

            if dataIndex > numberOfDataPointsLimit and dataIndex >= previous_dataIndex:

                seconds_since_start = sample.receiveTelemetry('SAMPLE_DATA_TLM', ['CCSDS_SECONDS'])['CCSDS_SECONDS']
                current_sim_time = normal_start_time + timedelta(seconds=seconds_since_start)

                convertedData = conv.Conversion.convertDataDict(responseFile, 'SAMPLE_HK_TLM', sample.config,
                                                                ['BATT_CURR_A_DATA', 'TH_CONT_UH_DATA',
                                                                 'BOXES_CURR_DATA', 'AMBIENT_T_DATA', 'BATT_CURR_A_DATA_2',
                                                                 'TH_CONT_UH_DATA_2', 'BOXES_CURR_DATA_2', 'AMBIENT_T_DATA_2'])
                for k, v in convertedData.items():
                    list = []
                    conto = numberOfDataPointsLimit
                    for i in convertedData[k][1:-1]:
                        timetowrite = current_sim_time - timedelta(seconds=(conto * command_frequency))
                        conto -= 1
                        list.append(f'{timetowrite.strftime("%Y-%m-%d %H:%M:%S.%f")},{i}')

                    convertedData[k] = list

                current_datetime = datetime.now()
                # Convert the date and time to a string
                timestamp_str = current_datetime.strftime('%Y-%m-%d')

                sample.tm2File(convertedData, 'SAMPLE_HK_TLM',
                               ['BATT_CURR_A_DATA', 'TH_CONT_UH_DATA', 'BOXES_CURR_DATA', 'AMBIENT_T_DATA',
                                     'BATT_CURR_A_DATA_2', 'TH_CONT_UH_DATA_2', 'BOXES_CURR_DATA_2', 'AMBIENT_T_DATA_2'],
                               [f'./{tmDataFolder}/BATT_CURR_A_DATA_{timestamp_str}_{cnt}.txt',
                                f'./{tmDataFolder}/TH_CONT_UH_DATA_{timestamp_str}_{cnt}.txt',
                                f'./{tmDataFolder}/BOXES_CURR_DATA_{timestamp_str}_{cnt}.txt',
                                f'./{tmDataFolder}/AMBIENT_T_DATA_{timestamp_str}_{cnt}.txt',
                                f'./{tmDataFolder}/BATT_CURR_A_DATA_2_{timestamp_str}_{cnt}.txt',
                                f'./{tmDataFolder}/TH_CONT_UH_DATA_2_{timestamp_str}_{cnt}.txt',
                                f'./{tmDataFolder}/BOXES_CURR_DATA_2_{timestamp_str}_{cnt}.txt',
                                f'./{tmDataFolder}/AMBIENT_T_DATA_2_{timestamp_str}_{cnt}.txt'
                                ])

                sample.sendCommand('SAMPLE_RESET_DATA_CNT_CC')
                dataSaved = True

            time.sleep(waitForCheckingDataCount)

        cnt = cnt + 1
        print(f'Run number: {cnt}')
