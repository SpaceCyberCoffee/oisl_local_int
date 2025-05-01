"""
Script to check for anomaly detected execute the commands.
TO BE RUN WITHIN CLOCK VM CONSTANTLY.
"""
import os
import shutil
import time
import subprocess
import datetime
import ast

configFile = '../Configuration/config_files/satellite_config.json'

def command_executer(subsystem_name, n_outliers):
    "TODO DEFINE STRUCTURE, NOW SIMPLY SEND COMMAND TO SWITCH CLOCK"
    sample = if2c.IF2COSMOS(configFile, 'SAMPLE_DEBUG')
    sample.sendCommand('SAMPLE_SWITCH_CLOCK')
    print('AAAAAAAA SAMPLE SWITCH CLOCK SENT!!!!')
    # try:
    #     result = subprocess.run(['python3', 'get_screenshot.py'], check=True, capture_output=True, text=True)
    #     print(result.stdout)  # Print the output of the script
    # except subprocess.CalledProcessError as e:
    #     print(f"Error occurred while running get_screenshot.py: {e}")
    #     print(e.stderr)
    commands_to_send = [['SAMPLE', 'SAMPLE_SWITCH_CLOCK', 20, 1], ['GENERIC_MAG_DEBUG', 'GENERIC_MAG_ENABLE_CC', 20, 1], ['GENERIC_FSS_DEBUG', 'GENERIC_FSS_ENABLE_CC', 20, 1]]
    get_screenshot(commands_to_send)


def get_screenshot(commands: list):
    """
    Takes as input the list of commands to send to the DT and adds it to the screen as a new dict entry
    """
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
            if subsystem_name == 'NOVATEL_OEM615_DEBUG':
                subsystem.sendCommand('NOVATEL_OEM615_REQ_DATA')
                time.sleep(3)
            responseFile = subsystem.receiveTelemetry(tmPacket, parameters_list)
            print('Received everything from:', subsystem_name)
            current_tm[subsystem_name] = responseFile

    print('Screenshot obtained: ', current_tm)

    seconds_since_start = int(current_tm['GENERIC_REACTION_WHEEL_DEBUG']['CCSDS_SECONDS'])
    screenshot_time = normal_start_time + datetime.timedelta(seconds=seconds_since_start)
    print('Screenshot time: ', screenshot_time)
    print('Sim seconds at screen time:', seconds_since_start)

    # Add the commands to the screenshot
    current_tm['COMMANDS'] = commands

    # WRITE THE SCREENSHOTS
    with open(f'{screenshot_folder}/screenshot.txt', 'w') as file:
        file.write(str(current_tm))

def execute_command(folder_path, archive_folder):
    # Check if the folder exists
    if not os.path.exists(folder_path):
        print(f"Folder '{folder_path}' does not exist.")
        return

    # Check if the folder is empty
    if not os.listdir(folder_path):
        print(f"Folder '{folder_path}' is empty. Sleeping for 1 minute...")
    else:
        print('Anomaly detected, about to execute command.')
        # TODO BETTER THIS, NOW 3 FILES FOR CLOCK, THEN MAYBE 3 CLOCK, ONE BATTERY AND SO ON
        for file in os.listdir(folder_path):
            cmd_file = os.path.join(folder_path, file)
            with open(cmd_file, 'r') as f:
                lines = f.readlines()
                subsystem_name = lines[0]
                n_outliers = lines[1]
                command_executer(subsystem_name, n_outliers)
                shutil.move(cmd_file, archive_folder)
                print(f"Archived '{cmd_file}' to '{archive_folder}'")





if __name__ == "__main__":
    import Configuration.IF2COSMOS as if2c

    config_file = '../Configuration/config_files/satellite_config.json'
    tm_file = '../Configuration/config_files/telemetry_config.txt'
    screenshot_folder = '..'

    while True:
        # Specify the folder to check
        folder_to_check = '/home/jstar/Desktop/github-nos3/COSMOS_Control/Execution/cmd_athmos'
        archive_folder = f'{folder_to_check}_archive'

        # Check the folder
        execute_command(folder_to_check, archive_folder)
        time.sleep(120)



