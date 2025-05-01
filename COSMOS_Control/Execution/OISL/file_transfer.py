# SCRIPT TO BE RUNNED FROM WITHIN THE VM OF THIS SAT

import os
import shutil
import json
import time
import Configuration.IF2COSMOS as if2c
import datetime 


config_file = '/home/jstar/Desktop/github-nos3/COSMOS_Control/Configuration/config_files/satellite_config.json'
file_to_send = '/home/jstar/Desktop/github-nos3/components/oisl/fsw/src/files_Test/plainText.txt'

def command_executer(subsystem_name, command_name, parameter_name, parameter_value):
    # Initialize the IF2COSMOS object
    subsystem = if2c.IF2COSMOS(config_file, subsystem=subsystem_name)
    # Send the command with the parameter and its value
    subsystem.sendCommand(commandName=command_name, parameterNames=parameter_name, parameterValues=parameter_value)


# Send the "send file" CMD to the OISL subsystem
subsystem_name = "OISL_DEBUG"
command_name = "OISL_SEND_FILE"
parameter_name = ["TARGET_SAT", "FILE_NAME"]
parameter_value = ["GROUND_STATION", file_to_send]
# Execute the command
print("Received the command {} for the subsystem {}. About to execute it", command_name, subsystem_name)
command_executer(subsystem_name, command_name, parameter_name, parameter_value)
                   

                   

