"""
Script to retrieve TM from S/C and initialize the DT accordingly.
Uses the receiveTM method from COmmandSEnder class.
"""
import ast
import datetime
import os
import time

import Configuration.IF2COSMOS as if2c
# from COSMOS_Control.Configuration import IF2COSMOS as if2c
import numpy as np

tm_file = '../Configuration/config_files/telemetry_config.txt'
config_file = '../Configuration/config_files/satellite_config.json'

screenshot_time = datetime.datetime(2025, 10, 18, 8, 45, 21) # start of sim
start_test = 86 #-2
start_time = datetime.datetime.utcnow()

test_folder = 'init_DT_test7'

# Part 0: enable subsystems
imu = if2c.IF2COSMOS(config_file, 'GENERIC_IMU_DEBUG')
st = if2c.IF2COSMOS(config_file, 'GENERIC_STAR_TRACKER_DEBUG')
mag = if2c.IF2COSMOS(config_file, 'GENERIC_MAG_DEBUG')
gps = if2c.IF2COSMOS(config_file, 'NOVATEL_OEM615_DEBUG')
gpsCnt = 0
imuCnt = 0
magCnt = 0
stCnt = 0
enableCnt = 0
allSubsystemsEnabled = 0

while allSubsystemsEnabled != 1:
    mag.sendCommand('GENERIC_MAG_ENABLE_CC')
    gps.sendCommand('NOVATEL_OEM615_ENABLE_CC')
    imu.sendCommand('GENERIC_IMU_ENABLE_CC')
    st.sendCommand('GENERIC_STAR_TRACKER_ENABLE_CC')

    try:
        magResponse = mag.receiveTelemetry('GENERIC_MAG_HK_TLM', 'DEVICE_ENABLED')
        gpsResponse = gps.receiveTelemetry('NOVATEL_OEM615_HK_TLM', 'DEVICE_ENABLED')
        imuResponse = imu.receiveTelemetry('GENERIC_IMU_HK_TLM', 'DEVICE_ENABLED')
        stResponse = st.receiveTelemetry('GENERIC_STAR_TRACKER_HK_TLM', 'DEVICE_ENABLED')

        print(magResponse['DEVICE_ENABLED'])
        print(gpsResponse['DEVICE_ENABLED'])
        print(imuResponse['DEVICE_ENABLED'])
        print(stResponse['DEVICE_ENABLED'])

        if imuResponse['DEVICE_ENABLED'] == 'ENABLED':
            imuCnt = 1
        if gpsResponse['DEVICE_ENABLED'] == 'ENABLED':
            gpsCnt = 1
        if magResponse['DEVICE_ENABLED'] == 'ENABLED':
            magCnt = 1
        if stResponse['DEVICE_ENABLED'] == 'ENABLED':
            stCnt = 1
    except:
        print('Could not receive telemetry for sending comands to enable subsystems!')

    if imuCnt + gpsCnt + magCnt + stCnt == 4:
        allSubsystemsEnabled = 1
        print('All subsystems enabled!')
    else:
        if imuCnt + gpsCnt + magCnt + stCnt == 2 and enableCnt > 10:
            allSubsystemsEnabled = 1
            print('Two of four subsystems enabled!')

    enableCnt = enableCnt + 1
    time.sleep(2)

# Check elapsed time and wait until tot seconds have passed since simulation start
rw = if2c.IF2COSMOS(config_file, 'GENERIC_REACTION_WHEEL_DEBUG')
while True:
    responseFile = rw.receiveTelemetry('GENRW_HK_TLM_T', 'CCSDS_SECONDS')
    current_sim_time = int(responseFile['CCSDS_SECONDS'])
    print('CUrrent sim time: ', current_sim_time)
    if current_sim_time >= start_test:
        break
    time.sleep(0.5)  # Sleep for a second before checking again






# Part 3: receive test tm after the right amount of seconds past initialization.

print('START TEST DT')

waitForNextRequest = 1
numberOfDataPointsLimit = 3  # number of received TLM to skip. the higher, the higher the distance between data points.

with open(tm_file, 'r') as tf:
    tms = tf.readlines()
    for subs in tms:
        subs = subs.strip().split(' - ')
        subsystem_name = subs[0]
        tmPacket = subs[1]
        parameters_list = ast.literal_eval(subs[2])
        subsystem = if2c.IF2COSMOS(config_file, subsystem_name)
        # Retrieve TM for this subsystem
        cnt = 0
        numberOfData = 400  # equal to the number of items per file that you want
        fileNames = [f'./{test_folder}/{subsystem_name}/{nome}_DT.txt' for nome in parameters_list]
        # Create subsystem folder if not existing already
        if not os.path.exists(f'./{test_folder}/{subsystem_name}'):
            os.makedirs(f'./{test_folder}/{subsystem_name}')

        while cnt != numberOfData:
            time.sleep(waitForNextRequest)

            dataIndex = 0

            while dataIndex < numberOfDataPointsLimit:
                if subsystem_name == 'NOVATEL_OEM615_DEBUG':
                    subsystem.sendCommand('NOVATEL_OEM615_REQ_DATA')
                responseFile = subsystem.receiveTelemetry(tmPacket, parameters_list)
                # # CHECK TO TAKE ONLY VALID STAR TRACKER DATA
                # if subsystem_name == 'GENERIC_STAR_TRACKER_DEBUG':
                #     if responseFile['STAR_TRACKER_IS_VALID'] == 0:
                #         pass
                #     else:
                #         dataIndex += 1
                # else:
                #     dataIndex += 1
                dataIndex += 1

            SIMtimenow = int(rw.receiveTelemetry('GENRW_HK_TLM_T', 'CCSDS_SECONDS')['CCSDS_SECONDS'])
            timetowrite = screenshot_time + datetime.timedelta(seconds=SIMtimenow)
            timetowrite = timetowrite.strftime('%Y-%m-%d %H:%M:%S.%f')
            cnt += 1
            if cnt == 1:
                tosave = {k: [timetowrite, v] for k, v in responseFile.items()} # TODO THIS IS NOT PERFECT
            else:
                for k, v in tosave.items():
                    tosave[k].append([timetowrite, responseFile[k]])
        print('tosave', tosave)
        subsystem.tm2File(tosave, tmPacket, parameters_list, fileNames)
        print(f'Finished for the subsystem {subsystem_name}')

