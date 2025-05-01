import os
import matplotlib.pyplot as plt
import datetime

def read_data_from_file(filename):
    with open(filename, 'r') as file:
        lines = file.readlines()[2:]
    timestamps = []
    values = []
    for line in lines:
        line = line.strip()
        if line:
            timestamp_str, value_str = line.strip("[]").split(', ')
            timestamp = datetime.datetime.strptime(timestamp_str.strip("'"), '%Y-%m-%d %H:%M:%S.%f')
            value = float(value_str)
            timestamps.append(timestamp)
            values.append(value)
    return timestamps, values

def plot_parameter(data_folder, parameter):
    normal_file = os.path.join(data_folder, f'{parameter}.txt')
    dt_file = os.path.join(data_folder, f'{parameter}_DT.txt')

    # Read data from files
    normal_timestamps, normal_values = read_data_from_file(normal_file)
    dt_timestamps, dt_values = read_data_from_file(dt_file)

    # Plot data
    plt.figure(figsize=(10, 6))
    plt.plot(normal_timestamps, normal_values, label=f'{parameter} (Normal)', color='blue')
    plt.plot(dt_timestamps, dt_values, label=f'{parameter} (DT)', color='red')
    plt.xlabel('Time')
    plt.ylabel('Value')
    plt.title(f'{parameter} Time Series')
    plt.legend()
    plt.grid(True)

    # Save the plot
    plot_filename = os.path.join(data_folder, f'{parameter}.png')
    plt.savefig(plot_filename)
    plt.close()

# Define the base data folder
base_data_folder = ('init_DT_test7')

# Define the parameters and their corresponding folders
parameters = {
    'GENERIC_IMU_DEBUG': [
        'X_LINEAR_ACCELERATION', 'Y_LINEAR_ACCELERATION', 'Z_LINEAR_ACCELERATION',
        'X_ANGULAR_ACCELERATION', 'Y_ANGULAR_ACCELERATION', 'Z_ANGULAR_ACCELERATION'
    ],
    # 'GENERIC_MAG_DEBUG': [
    #     'RAW_MAG_X', 'RAW_MAG_Y', 'RAW_MAG_Z'
    # ],
    'GENERIC_REACTION_WHEEL_DEBUG': [
        'CCSDS_SECONDS', 'MOMENTUM_NMS_0', 'MOMENTUM_NMS_1', 'MOMENTUM_NMS_2'
    ],
    # 'GENERIC_STAR_TRACKER_DEBUG': [
    #     'STAR_TRACKER_Q0', 'STAR_TRACKER_Q1', 'STAR_TRACKER_Q2', 'STAR_TRACKER_Q3', 'STAR_TRACKER_IS_VALID'
    # ],
    'NOVATEL_OEM615_DEBUG': [
        'ECEF_X', 'ECEF_Y', 'ECEF_Z', 'VEL_X', 'VEL_Y', 'VEL_Z',
        'ECI_X', 'ECI_Y', 'ECI_Z', 'VEL_I_X', 'VEL_I_Y', 'VEL_I_Z'
    ],
    'SIM_42_TRUTH': [
        'QN_1', 'QN_2', 'QN_3', 'QN_4', 'WN_X_DPS', 'WN_Y_DPS', 'WN_Z_DPS', 'GEOCENTRIC_LATITUDE'
    ],
    'SAMPLE_DEBUG': [
        'AMBIENT_TEMPERATURE', 'BOXES_CURRENT', 'BATTERY_CURRENT_A', 'THERMAL_CONTROL_UNIT_HEATER'
    ]
}

# Plot each parameter
for subsystem, params in parameters.items():
    data_folder = os.path.join(base_data_folder, subsystem)
    for parameter in params:
        plot_parameter(data_folder, parameter)
