"""
Script to check for anomaly detected and move the cmd file into a folder within the VM
USELESS DAVID WILL DO
"""
import datetime
import os
import shutil
import time



def check_folder(folder_path, archive_folder, nos3_folder):
    # Check if the folder exists
    if not os.path.exists(folder_path):
        print(f"Folder '{folder_path}' does not exist.")
        return

    # Check if the folder is empty
    if not os.listdir(folder_path):
        print(f"Folder '{folder_path}' is empty. Sleeping for 1 minute...")
    else:
        print('Anomaly detected, about to move the cmd file .')
        for file in os.listdir(folder_path):
            cmd_file = os.path.join(folder_path, file)
            shutil.copy2(cmd_file, nos3_folder)
            shutil.move(cmd_file, archive_folder)
            print(f"Archived '{cmd_file}' to '{archive_folder}'")





if __name__ == "__main__":

    while True:
        # Specify the folder to check
        folder_to_check = '/mnt/extras/SSD/NOS3_RBT/Clock_data/clock_cmd'
        archive_folder = '/mnt/extras/SSD/NOS3_RBT/Clock_data/cmd_archive'
        nos3_folder = '/mnt/extras/SSD/NOS3_RBT/nos3_lp_clock/nos3/COSMOS_Control/Execution/cmd_athmos'

        # Check the folder
        check_folder(folder_to_check, archive_folder, nos3_folder)
        time.sleep(60)


