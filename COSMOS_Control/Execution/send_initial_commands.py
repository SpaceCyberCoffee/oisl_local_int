"""
Script to be runned by the startDT service after make launch. 
It reads the commands_schedule written from the screenshot and executes those commands once.
"""
from Configuration.CommandSender import CommandScheduler, tm_file
import os

command_file = '../Configuration/config_files/commands_schedule.txt'
config_file = '../Configuration/config_files/satellite_config.json'

print(os.getcwd())

schedule = CommandScheduler(configuration_file=config_file, schedule_file=command_file, telemetry_file=tm_file)
schedule.runs = 1
schedule.run_scheduler()

