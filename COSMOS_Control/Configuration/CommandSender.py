"""
Class to send commands according to a configurable schedule.
Initialized by:
-json config file of the s/c
-schedule file with the commands, their frequencies and priorities
-telemetry file containing the parameters to download for every subsystem
"""

import ast
import os
import sched, time
import Configuration.IF2COSMOS as if2c
from Utilities import InputOutput as io


config_file = './config_files/satellite_config.json'
command_file = './config_files/commands_schedule.txt'
tm_file = './config_files/telemetry_config.txt'
tmDataFolder = f'../Execution/tm_data/LUCA/'
sleep_time = 10  # seconds

class CommandScheduler:
    def __init__(self, configuration_file, schedule_file, telemetry_file):
        self.config_file = configuration_file
        self.schedule_file = schedule_file
        self.telemetry_file = telemetry_file
        self.s = sched.scheduler(time.time, time.sleep)
        self.runs = 3  # In final version it will be a max number of runs and a constant while loop
        self.cnt = 0
        self.numberOfDataPointsLimit = 3  # number of received TLM to skip. the higher, the higher the distance between data points.
        self.waitForNextRequest = 1

    def command_sender(self, subsystem_name: str, command_name: str) -> None:
        """
        Method to send commands to the subsystems through the IF2COSMOS interface.
        :param subsystem_name: name of the subsystem
        :param command_name: name of the command
        :return: None.
        """
        subsystem = if2c.IF2COSMOS(self.config_file, subsystem_name)
        command = subsystem.sendCommand(command_name)

    def telemetry_receiver(self, subsystem_name='ALL') -> None:
        """
        Method to receive telemetry from the subsystems through the IF2COSMOS interface.
        :param subsystem_name: name of the subsystem. If ALL then it iterates through every subsystem in the config.
        :return: None.
        """
        tmPacket = ''
        parameters_list = []
        # Read the tm_config.txt file and find the subsystems telemetry info
        if subsystem_name != 'ALL':
            with open(self.telemetry_file, 'r') as tf:
                tms = tf.readlines()
                for subs in tms:
                    subs = subs.strip().split(' - ')
                    if subs[0] == subsystem_name:
                        tmPacket= subs[1]
                        parameters_list = ast.literal_eval(subs[2])
            if len(tmPacket) < 1 or len(parameters_list) < 1:
                raise Exception('Problems reading the telemetry config file.')

            fileNames = [f'./{tmDataFolder}/{subsystem_name}/{nome}.txt' for nome in parameters_list]
            if not os.path.exists(f'./{tmDataFolder}/{subsystem_name}'):
                os.makedirs(f'./{tmDataFolder}/{subsystem_name}')
            subsystem = if2c.IF2COSMOS(self.config_file, subsystem_name)
            tosave = []
            responseFile = []
            while self.cnt != self.runs:
                time.sleep(self.waitForNextRequest)
                dataIndex = 0
                while dataIndex < self.numberOfDataPointsLimit:
                    responseFile = subsystem.receiveTelemetry(tmPacket, parameters_list)
                    dataIndex += 1
                self.cnt += 1
                if self.cnt == 1:
                    tosave = {k: [v] for k, v in responseFile.items()}
                else:
                    for k, v in tosave.items():
                        tosave[k].append(responseFile[k])
            subsystem.tm2File(tosave, tmPacket, parameters_list, fileNames)

        else:
            with open(self.telemetry_file, 'r') as tf:
                tms = tf.readlines()
                for subs in tms:
                    subs = subs.strip().split(' - ')
                    subsystem_name = subs[0]
                    tmPacket = subs[1]
                    parameters_list = ast.literal_eval(subs[2])
                    if len(tmPacket) < 1 or len(parameters_list) < 1:
                        raise Exception('Problems reading the telemetry config file.')

                    fileNames = [f'./{tmDataFolder}/{subsystem_name}/{nome}.txt' for nome in parameters_list]
                    if not os.path.exists(f'./{tmDataFolder}/{subsystem_name}'):
                        os.makedirs(f'./{tmDataFolder}/{subsystem_name}')
                    subsystem = if2c.IF2COSMOS(self.config_file, subsystem_name)
                    tosave = []
                    responseFile = []
                    while self.cnt != self.runs:
                        time.sleep(self.waitForNextRequest)
                        dataIndex = 0
                        while dataIndex < self.numberOfDataPointsLimit:
                            responseFile = subsystem.receiveTelemetry(tmPacket, parameters_list)
                            dataIndex += 1
                        self.cnt += 1
                        if self.cnt == 1:
                            tosave = {k: [v] for k, v in responseFile.items()}
                        else:
                            for k, v in tosave.items():
                                tosave[k].append(responseFile[k])
                    subsystem.tm2File(tosave, tmPacket, parameters_list, fileNames)
                    self.cnt = 0


    def command_executer(self) -> None:
        """
        Method to read commands from schedule, insert them in the scheduler object and then execute the commands.
        :return: None.
        """
        # READ THE.txt file and populate the schedule
        with open(self.schedule_file, 'r') as sf:
            commands = sf.readlines()
            for comm in commands:
                subsystem, command, delay, priority = comm.strip().split(' ')
                self.s.enter(delay=int(delay), priority=int(priority), action=self.command_sender, argument=(subsystem, command))
        self.s.run()


    def run_scheduler(self) -> None:
        while self.cnt < self.runs:
            self.cnt += 1
            self.command_executer()
            time.sleep(sleep_time)

# Example usage
if __name__ == "__main__":
    scheduler = CommandScheduler(configuration_file=config_file, schedule_file=command_file, telemetry_file=tm_file)
    # scheduler.run_scheduler()
    scheduler.telemetry_receiver(subsystem_name='GENERIC_IMU_DEBUG')

'''
########TM READ FROM .JSON###########################
configData = io.readJson(config_file)
for subsystem, v in configData['subsystems'].items():
    tmPackets = configData['subsystems'][subsystem]['tm']
    for tmPacket, dic in tmPackets.items():
        parameters = list(configData['subsystems'][subsystem]['tm'][tmPacket].keys())
        print(tmPacket, parameters)
        print('DONE FOR TM PACKET', tmPacket)
    print('DONE FOR SUBSYSTEM', subsystem)
'''