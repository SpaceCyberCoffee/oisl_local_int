# SCRIPT TO BE RUNNED FROM WITHIN THE VM OF THIS SAT

import os
import shutil
import json
import time
import Configuration.IF2COSMOS as if2c
import datetime 

def command_executer(subsystem_name, command_name, parameter_name, parameter_value):
    # Initialize the IF2COSMOS object
    subsystem = if2c.IF2COSMOS(config_file, subsystem=subsystem_name)
    # Send the command with the parameter and its value
    subsystem.sendCommand(commandName=command_name, parameterNames=parameter_name, parameterValues=parameter_value)

def check_folder(folder_path):
    # Check if the folder exists
    if not os.path.exists(folder_path):
        print(f"Folder '{folder_path}' does not exist.")
        return

    # Check if the folder is empty
    if not os.listdir(folder_path):
        pass
    else:
        print('File received. Next action.')
        
        for file in os.listdir(folder_path):
            received_file = os.path.join(folder_path, file)
            
            with open(received_file, 'r') as f:
                try:
                    # Parse JSON content from the file
                    file_content = json.load(f)

                     # Extract file type and create if else 
                    file_type = file_content["TYPE"]
                    # CMD
                    if file_type == "CMD":
                        archive_folder = archive_folder_cmd
                        # Read subsystem and command
                        subsystem_name = file_content["CONTENT"]["SUBSYSTEM"]
                        command_name = file_content["CONTENT"]["COMMAND_NAME"]
                        parameter_name = file_content["CONTENT"]["PARAMETER"]
                        if parameter_name == "":
                            parameter_name = []
                        parameter_value = file_content["CONTENT"]["PARAMETER_VALUE"]
                        if parameter_value == "":
                            parameter_value = []
                        # Execute the command
                        print("Received the command {} for the subsystem {}. About to execute it", command_name, subsystem_name)
                        command_executer(subsystem_name, command_name, parameter_name, parameter_value)
                    # TM
                    else: 
                        archive_folder = archive_folder_tm
                        # Process telemetry data
                        telemetry_data = file_content["telemetry_data"]
                        instructions = file_content["instructions"]

                        # Log received telemetry data
                        for subsystem_name, parameters in telemetry_data.items():
                            for parameter_name, parameter_value in parameters.items():
                                print(f"Received the TM Parameter {parameter_name} for the subsystem {subsystem_name}. The value is {parameter_value}")

                        # Perform diagnostics if specified
                        if "diagnostics" in instructions:
                            compare_sat_id = instructions["diagnostics"]["compare_sat_id"]
                            diagnostics_type = instructions["diagnostics"]["diagnostics_type"]
                            print(f"Performing diagnostics against satellite {compare_sat_id} for {diagnostics_type}.")
                            # Here you would implement the actual diagnostics logic

                        # Relay telemetry to ground station if specified
                        if "relay" in instructions:
                            ground_station_id = instructions["relay"]["ground_station_id"]
                            relay_window_start = instructions["relay"]["relay_window_start"]
                            relay_window_end = instructions["relay"]["relay_window_end"]
                            print(f"Preparing to relay TM to ground station {ground_station_id} between {relay_window_start} and {relay_window_end}.")
                            # Here you would implement the actual relaying logic

                        # DL tm to ground station if specified
                        if "DL" in instructions:
                            ground_station_id = instructions["DL"]["ground_station_name"]
                            print(f"Preparing to relay TM to ground station {ground_station_id}.")
                            # Send DL command
                            subsystem_name = "OISL_DEBUG"
                            command_name = "OISL_SEND_FILE"
                            parameter_name = ["TARGET_SAT", "FILE_NAME"]
                            parameter_value = ["GROUND_STATION", "/home/jstar/Desktop/github-nos3/components/oisl/fsw/src/fileInput/OGS.txt"]
                            # Execute the command
                            print("Received the command {} for the subsystem {}. About to execute it".format(command_name, subsystem_name))
                            # MOVE THE FILE TO A FOLDER THAT CAN BE READ: from "/home/jstar/Desktop/github-nos3/COSMOS_Control/Execution/OISL/files_received/OGS.txt" to /home/jstar/Desktop/github-nos3/components/oisl/fsw/src/fileInput/OGS.txt"
                            print("Hallooo", fileInput_dir + file)
                            shutil.move(received_file, fileInput_dir + file)
                            command_executer(subsystem_name, command_name, parameter_name, parameter_value)

                    # Move the command file to the archive folder
                    time = file_content["header"]["timestamp"]
                    # Extract filename without extension and add timestamp
                    file_base = received_file.split('/')[-1][:-4]  # Removing '.txt' from the file name
                    new_file_name = f"{file_base}_{time}.txt"

                    # Full destination path in the archive folder
                    archive_path = f"{archive_folder}/{new_file_name}"

                    # Move and rename the file
                    try:
                        shutil.move(received_file, archive_path)
                    except: # The file has been moved during the if "DL" 
                        shutil.copyfile(fileInput_dir + file, archive_path)
                    print(f"Archived '{received_file}' to '{archive_path}'")
                    
                except json.JSONDecodeError:
                    print(f"Error: Invalid JSON format in '{received_file}'. I WILL ASSUME IT IS LARGE FILE TRANSFER NOW.")
                    # 13.01.2025: Assume is large file transfer: send it as a OGS DL command.
                    # Copy the file to the fileInput folder of the receiver
                    receiver_folder_OISL = "/home/jstar/Desktop/github-nos3/components/oisl/fsw/src/fileInput/receivedFile.txt"
                    shutil.copyfile(received_file, receiver_folder_OISL)
                    # Send DL command
                    subsystem_name = "OISL_DEBUG"
                    command_name = "OISL_SEND_FILE"
                    parameter_name = ["TARGET_SAT", "FILE_NAME"]
                    parameter_value = ["GROUND_STATION", "/home/jstar/Desktop/github-nos3/components/oisl/fsw/src/fileInput/receivedFile.txt"]
                    # Execute the command
                    print("Received the command {} for the subsystem {}. About to execute it".format(command_name, subsystem_name))
                    # MOVE THE FILE TO A FOLDER THAT CAN BE READ: from "/home/jstar/Desktop/github-nos3/COSMOS_Control/Execution/OISL/files_received/OGS.txt" to /home/jstar/Desktop/github-nos3/components/oisl/fsw/src/fileInput/OGS.txt"
                    print("Hallooo", fileInput_dir + file)
                    shutil.move(received_file, fileInput_dir + file)
                    command_executer(subsystem_name, command_name, parameter_name, parameter_value)
                    # Move the command file to the archive folder
                    archive_folder = archive_folder_tm
                    time = file_content["header"]["timestamp"]
                    # Extract filename without extension and add timestamp
                    file_base = received_file.split('/')[-1][:-4]  # Removing '.txt' from the file name
                    new_file_name = f"{file_base}_{time}.txt"
                    # Full destination path in the archive folder
                    archive_path = f"{archive_folder}/{new_file_name}"
                    # Move and rename the file
                    try:
                        shutil.move(received_file, archive_path)
                    except: # The file has been moved during the if "DL" 
                        shutil.copyfile(fileInput_dir + file, archive_path)
                    print(f"Archived '{received_file}' to '{archive_path}'")

                except KeyError as e:
                    print(f"Error: Missing key in JSON: {e}")

if __name__ == "__main__":
    config_file = '/home/jstar/Desktop/github-nos3/COSMOS_Control/Configuration/config_files/satellite_config.json'

    while True:
        # Specify the folder to check
        folder_to_check = '/home/jstar/Desktop/github-nos3/COSMOS_Control/Execution/OISL/files_received'
        archive_folder_cmd = '/home/jstar/Desktop/github-nos3/COSMOS_Control/Execution/OISL/commands_archived'
        archive_folder_tm = '/home/jstar/Desktop/github-nos3/COSMOS_Control/Execution/OISL/tm_archived'
        fileInput_dir = "/home/jstar/Desktop/github-nos3/components/oisl/fsw/src/fileInput/"

        # Check the folder
        check_folder(folder_to_check)
        time.sleep(1)
