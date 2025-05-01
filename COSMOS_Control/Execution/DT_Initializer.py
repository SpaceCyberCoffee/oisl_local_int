"""
Script to retrieve TM from S/C and initialize the DT accordingly.
Uses the receiveTM method from COmmandSEnder class.
"""
import ast
import datetime
import os
import time
import xml.etree.ElementTree as ET
from fileinput import FileInput
import re 
import Configuration.IF2COSMOS as if2c
# from COSMOS_Control.Configuration import IF2COSMOS as if2c
import numpy as np
from scipy.spatial.transform import Rotation

def compute_argument_of_latitude(position_vector, velocity_vector):
    position_vector = position_vector / 1000
    velocity_vector = velocity_vector / 1000
    # Calculate the nodal vector (pointing towards ascending node so z is 0!)
    K = np.array([0, 0, 1])

    # Calculate the specific angular momentum vector
    h_vector = np.cross(position_vector, velocity_vector)

    # Calculate the nodal vector
    n_vector = np.cross(K, h_vector)
    n_norm = np.linalg.norm(n_vector)
    n_unit = n_vector / n_norm

    # Calculate the unit position vector
    r_norm = np.linalg.norm(position_vector)
    r_unit = position_vector / r_norm

    # Calculate the argument of latitude (u)
    dot_product = np.dot(n_unit, r_unit)
    # dot_product = np.clip(dot_product, -1.0, 1.0)  # Ensure the value is within the valid range for arccos
    u = np.arccos(dot_product)

    # Adjust the argument of latitude based on the z-component of the position vector
    if position_vector[2] < 0:
        u = 2 * np.pi - u

    return np.degrees(u)

def compute_argument_of_latitude_with_time(time_elapsed):
    # Compute orbital period
    a_m = (6371 + 400) * 1000
    mu = 3.986e14
    orbital_period = 2 * np.pi * np.sqrt(a_m ** 3 / mu)
    # Calculate the mean motion
    mean_motion = 360 / orbital_period

    # Calculate the true anomaly
    true_anomaly = mean_motion * time_elapsed

    return true_anomaly

def update_true_anomaly(filename, new_value):
    # Read the file contents
    with open(filename, 'r') as file:
        lines = file.readlines()

    # Modify the line containing "True Anomaly"
    for i, line in enumerate(lines):
        if "True Anomaly" in line:
            parts = line.split()
            parts[0] = "{:.4f}".format(new_value)
            lines[i] = ' '.join(parts) + '\n'
            break

    # Write the updated contents back to the file
    with open(filename, 'w') as file:
        file.writelines(lines)


def update_wheel_imu(filename, initial_moments, imu_acc):
    """
    Update the Initial Momentum of wheels in the given file.

    Parameters:
    - filename: str, the path to the file to be modified
    - initial_moments: list of float, the new Initial Momentum values for the wheels
    - imu_acc: list of float, the new Initial bias values for the IMU
    """
    # Read the file contents
    with open(filename, 'r') as file:
        lines = file.readlines()

    # Track the number of wheels updated
    wheel_count = 0
    imu_count = 0

    # Modify the lines containing "Initial Momentum" and IMU INitial bias
    for i, line in enumerate(lines):
        if "Initial Momentum" in line:
            if wheel_count < len(initial_moments):
                lines[i] = f"{initial_moments[wheel_count]}                         ! Initial Momentum, N-m-sec\n"
                wheel_count += 1
        if "Initial Bias (m/s^2)" in line:
            if imu_count < len(imu_acc):
                lines[i] = f"{imu_acc[imu_count]}                         ! Initial Bias (m/s^2)\n"
                imu_count += 1

    # Write the updated contents back to the file
    with open(filename, 'w') as file:
        file.writelines(lines)


def update_time_utc(filename, seconds_to_add):
    """
    """
    # Parse the XML file
    tree = ET.parse(filename)
    root = tree.getroot()

    # Find the <start-time> element and update its value
    for elem in root.iter('start-time'):
        # Get the current start time as a float
        current_start_time = float(elem.text)

        # Add the specified number of seconds
        new_start_time = current_start_time + seconds_to_add

        # Update the text of the <start-time> element
        elem.text = "{:.1f}".format(new_start_time)
        break

    # Write the updated XML back to the file
    tree.write(filename, encoding='utf-8', xml_declaration=True)

def update_ang_velocities(filename, ang_velocities):
    """
    Updates the angular velocities in the XML file.

    Parameters:
    - filename: The path to the XML file.
    - ang_velocities: A list of three floats representing the x, y, and z components of the angular velocity.
    """
    # Parse the XML file
    tree = ET.parse(filename)
    root = tree.getroot()

    # Find the <tipoff_x>, <tipoff_y>, and <tipoff_z> elements and update their values
    for elem in root.findall('.//tipoff_x'):
        elem.text = str(np.round(ang_velocities[0], decimals=2))

    for elem in root.findall('.//tipoff_y'):
        elem.text = str(np.round(ang_velocities[1], decimals=2))

    for elem in root.findall('.//tipoff_z'):
        elem.text = str(np.round(ang_velocities[2], decimals=2))

    # Write the updated XML back to the file
    tree.write(filename, encoding='utf-8', xml_declaration=True)

def update_attitude(filename, euler_angles, quaternion):
    # Read the file contents
    with open(filename, 'r') as file:
        lines = file.readlines()


    # Modify the lines containing "Initial Momentum"
    for i, line in enumerate(lines):
        if "Angles (deg) & Euler Sequence" in line:
            lines[i] = f"{np.round(euler_angles[0],decimals=2)}  {np.round(euler_angles[1],decimals=2)}  {np.round(euler_angles[2],decimals=2)}    213    ! Angles (deg) & Euler Sequence\n"
        # Move from Euler angles t Quaternion
        if "Ang Vel wrt [NL], Att [QA] wrt [NLF]" in line:
            lines[i] = 'NQN                         ! Ang Vel wrt [NL], Att [QA] wrt [NLF]\n'
        # Change the quaternion
        if "Quaternion" in line:
            lines[i] = f"{np.round(quaternion[0],decimals=3)}  {np.round(quaternion[1],decimals=3)}  {np.round(quaternion[2],decimals=3)}   {np.round(quaternion[3],decimals=3)}    ! Quaternion\n"


    # Write the updated contents back to the file
    with open(filename, 'w') as file:
        file.writelines(lines)

def replace_clock_counter(filename, index_hk, index_data):
    with open(filename, 'r') as file:
        lines = file.readlines()
    
    for i, line in enumerate(lines):
        if "// Initialization of clockindex" in line:
            # Replace both occurrences of 0 with the new_value
            lines[i+1] = lines[i+1].replace("_currentClockIndex_data = 0;", f"_currentClockIndex_data = {index_data};")
            lines[i+2] = lines[i+2].replace("_currentClockIndex_hk = 0;", f"_currentClockIndex_hk = {index_hk};")
            break

    # Write the modified lines back to the file
    with open(filename, 'w') as file:
        file.writelines(lines)
      
    


tm_file = '../Configuration/config_files/telemetry_config.txt'
config_file = '../Configuration/config_files/satellite_config.json'

sleep_time = 120  # Sleep for 2 min
# test_time = 120  # 2 minutes of receiving tm to test.
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
    time.sleep(10)


# Part 1: obtain the screenshot at the init moment.
current_tm = {}
normal_start_time = datetime.datetime(year=2025, month=10, day=18, hour=8, minute=30)
with open(tm_file, 'r') as tf:
    tms = tf.readlines()
    for subs in tms:
        subs = subs.strip().split(' - ')
        subsystem_name = subs[0]
        tmPacket = subs[1]
        parameters_list = ast.literal_eval(subs[2])
        if len(tmPacket) < 1 or len(parameters_list) < 1:
            raise Exception('Problems reading the telemetry config file.')
        subsystem = if2c.IF2COSMOS(config_file, subsystem_name)
        tosave = []
        if subsystem_name == 'NOVATEL_OEM615_DEBUG':
            subsystem.sendCommand('NOVATEL_OEM615_REQ_DATA')
        responseFile = subsystem.receiveTelemetry(tmPacket, parameters_list)
        print('Received everything from:', subsystem_name)
        current_tm[subsystem_name] = responseFile

print('Screenshot obtained: ', current_tm)

seconds_since_start = int(current_tm['GENERIC_REACTION_WHEEL_DEBUG']['CCSDS_SECONDS'])
screenshot_time = normal_start_time + datetime.timedelta(seconds=seconds_since_start)
print('Screenshot time: ', screenshot_time)
print('Sim seconds at screen time:', seconds_since_start)

# WRITE THE SCREENSHOT
current_tm['screentime'] = screenshot_time
with open(f'{test_folder}/screenshot.txt', 'w') as file:
    file.write(str(current_tm))

# Part 2: extract the info and modify the right files, then sleep for two minutes.
orbit_file = '/home/jstar/Desktop/github-nos3/cfg/InOut/Orb_LEO.txt'
sc_file = '/home/jstar/Desktop/github-nos3/cfg/InOut/SC_NOS3.txt'
time_file = '/home/jstar/Desktop/github-nos3/cfg/nos3-mission.xml'
ang_vel_file = '/home/jstar/Desktop/github-nos3/cfg/sc-full-config.xml'
clock_file = '/home/jstar/Desktop/github-nos3/components/sample/sim/src/sample_hardware_model.cpp'
# Get the argument of latitude (true anomaly) from GPS and from simple time computation
position_vector = np.array([current_tm['NOVATEL_OEM615_DEBUG']['ECI_X'], current_tm['NOVATEL_OEM615_DEBUG']['ECI_Y'], current_tm['NOVATEL_OEM615_DEBUG']['ECI_Z']])
velocity_vector = np.array([current_tm['NOVATEL_OEM615_DEBUG']['VEL_I_X'], current_tm['NOVATEL_OEM615_DEBUG']['VEL_I_Y'], current_tm['NOVATEL_OEM615_DEBUG']['VEL_I_Z']])
theta = compute_argument_of_latitude(position_vector, velocity_vector)
theta2 = compute_argument_of_latitude_with_time(seconds_since_start)
update_true_anomaly(orbit_file, new_value=theta)
# Get the current wheel momentums
rw_mom0 = float(current_tm['GENERIC_REACTION_WHEEL_DEBUG']['MOMENTUM_NMS_0'])
rw_mom1 = float(current_tm['GENERIC_REACTION_WHEEL_DEBUG']['MOMENTUM_NMS_1'])
rw_mom2 = float(current_tm['GENERIC_REACTION_WHEEL_DEBUG']['MOMENTUM_NMS_2'])
# Get the current IMU linear acceleration
imu_acc_x = float(current_tm['GENERIC_IMU_DEBUG']['X_LINEAR_ACCELERATION'])
imu_acc_y = float(current_tm['GENERIC_IMU_DEBUG']['Y_LINEAR_ACCELERATION'])
imu_acc_z = float(current_tm['GENERIC_IMU_DEBUG']['Z_LINEAR_ACCELERATION'])
update_wheel_imu(sc_file, initial_moments=[rw_mom0, rw_mom1, rw_mom2], imu_acc=[imu_acc_x, imu_acc_y, imu_acc_z])
# Get the newsim start
update_time_utc(time_file, seconds_since_start)
# Get attitude
quat = np.array([current_tm['SIM_42_TRUTH']['QN_1'], current_tm['SIM_42_TRUTH']['QN_2'], current_tm['SIM_42_TRUTH']['QN_3'], current_tm['SIM_42_TRUTH']['QN_4']])
rot = Rotation.from_quat(quat)
rot_euler = rot.as_euler('YXZ', degrees=True) # 213 INTRINSIC CAUSE THE EULER ANGLES ARE IN BODY FRAME
angular_velocities = np.array([current_tm['SIM_42_TRUTH']['WN_X_DPS'], current_tm['SIM_42_TRUTH']['WN_Y_DPS'], current_tm['SIM_42_TRUTH']['WN_Z_DPS']])
update_attitude(sc_file, rot_euler, quat)
update_ang_velocities(ang_vel_file, angular_velocities)
# Get Clock index
sample = if2c.IF2COSMOS(config_file, 'SAMPLE_DEBUG')
clock_index_data = int(sample.receiveTelemetry('SAMPLE_DATA_TLM', 'CLOCK_COUNTER')['CLOCK_COUNTER'])
clock_index_hk = int(sample.receiveTelemetry('SAMPLE_HK_TLM', 'TIME_STEPS_CNT')['TIME_STEPS_CNT'])
replace_clock_counter(clock_file, clock_index_hk, clock_index_data)
# Sleep
print('Sleep for 2 minutes')
time.sleep(sleep_time)

# Part 3: receive test tm after waking up, then save these tm on a test folder.

rw = if2c.IF2COSMOS(config_file, 'GENERIC_REACTION_WHEEL_DEBUG')
responseFile = rw.receiveTelemetry('GENRW_HK_TLM_T', 'CCSDS_SECONDS')
start_test_DT = int(responseFile['CCSDS_SECONDS']) - seconds_since_start # NUMBER OF SECONDS AFTER THE INIT OF THE DT AFTER WHICH YOU START THE TEST. EX: Screenshot 100 seconds into the sim. then sleep and the sim time is now 300 seconds, meaning that the DT starts the test at 200 seconds of its sim time
print('START TEST DT at this sim time: ', start_test_DT)

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
        fileNames = [f'./{test_folder}/{subsystem_name}/{nome}.txt' for nome in parameters_list]
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
            timetowrite = screenshot_time + datetime.timedelta(seconds=(SIMtimenow - seconds_since_start))
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

