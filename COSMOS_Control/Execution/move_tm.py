"""
Script to move the TM generated via NOS3 to the clockdata folder for ATHMOS.
TOBE run from the server.
"""

import os
import pathlib
import shutil
import time


def copy_files(source_folder, destination_folder, archive_folder):
    # Check if the source folder exists
    if not os.path.exists(source_folder):
        print(f"Source folder '{source_folder}' does not exist.")
        return

    # Recursively create the destination folder structure if it doesn't exist
    if not os.path.exists(destination_folder):
        os.makedirs(destination_folder)
        print(f"Destination folder '{destination_folder}' created.")
    # Copy each file from the source folder to the destination folder
    for file in os.listdir(source_folder):
        source_file = os.path.join(source_folder, file)
        destination_file = os.path.join(destination_folder, file)
        archive_file = os.path.join(archive_folder, file)

        # Check if the item is a file
        if source_file.endswith('.txt'):
            # Move the file to the destination folder
            shutil.copy2(source_file, destination_file)
            print(f"Copied '{source_file}' to '{destination_file}'")

            # Then move the file to the archive folder
            shutil.move(source_file, archive_file)
            print(f"Archived '{source_file}' to '{archive_file}'")





if __name__ == "__main__":
    # Specify the source and destination folders
    while True:
        source_folder = './tm_data/LUCA/CLOCK'
        destination_folder = '/mnt/extras/SSD/NOS3_RBT/Clock_data/tm_received'
        archive_folder = './tm_data/LUCA/CLOCK/archive'
        # Copy files from source to destination
        copy_files(source_folder, destination_folder, archive_folder)
        print('Sleep')
        time.sleep(120)

