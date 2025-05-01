"""
----------------------------------------------------------------------------
Satellite Alignment Data Exchange Script
Author: Luca Pizzuto

This script manages the alignment data exchange between three satellites:
- Central Satellite
- Forward Satellite
- Backward Satellite

The general structure of the communication is as follows:
1. **Central Satellite**:
   - Communicates with both the Forward and Backward Satellites.
   - Sends its first alignment value to the Forward Satellite and second alignment value to the Backward Satellite.
2. **Forward Satellite**:
   - Only communicates with the Central Satellite.
   - Sends its alignment data to the Central Satellite.
3. **Backward Satellite**:
   - Only communicates with the Central Satellite.
   - Sends its alignment data to the Central Satellite.

Every satellite propagates the orbit of its forward and backward intra-planar satellites.
The alignment data for each satellite is stored in a file called "my_alignments.txt", WHICH CONTAINS ALSO THE MEMORY INFORMATION (08/01/2025).
Ex:
If the central sat is in OISL_F mode, the file will contain "1 0"
If the central sat is in OISL_B mode, the file will contain "0 1"
If the central sat is in OISL_OGS mode, the file will contain "0 0"

Each satellite writes its respective alignment data to designated output files based on the direction:
- Central Satellite writes its backward alignment to "F_sat_back_alignment.txt" to be read by the backward sat,
  and its forward alignment to "B_sat_for_alignment.txt" to be read by the forward sat.
- Forward Satellite writes its backward alignment to "F_sat_back_alignment.txt" to be read by the central sat.
- Backward Satellite writes its forward alignment to "B_sat_for_alignment.txt" to be read by the central sat.

The script processes the alignment data in a loop, ensuring continuous communication
between the satellites and proper handling of file transfers between them.
It also checks for a "file_sent.txt" indicating that a file is to be transferred
between the satellites and copies it accordingly.

The script is designed to be scalable, so if additional satellites are added,
the `satellite_connections_dict` can easily be updated to accommodate the new connections.
----------------------------------------------------------------------------
"""
import os
from time import sleep
import shutil
import json

# Define fsw directories for each satellite
dir_central = "/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src"
dir_forward = "/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/forward_sat/nos3_rbt/components/oisl/fsw/src"
dir_backward = "/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/Backward_Sat/components/oisl/fsw/src"
dir_Sat_23 = "/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/Sat_1_23/components/oisl/fsw/src"
# dir_1_3 = "/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/1_3/components/oisl/fsw/src"

# Alignment file names
file_alignment_back = "/F_sat_back_alignment.txt"    # The sat gives his backward alignment to the backward
file_alignment_for = "/B_sat_for_alignment.txt"      # The sat gives his forward alignment to the forward

# Paths for the output files
forward_output_back = dir_central + file_alignment_back                  # used by the forward satellite
# forward_output_for = dir_1_3 + file_alignment_for                      # used by the forward satellite
backward_output_for = dir_central + file_alignment_for                   # used by the backward satellite
backward_output_back = dir_Sat_23 + file_alignment_back                  # used by the forward satellite
central_output_for  = dir_forward + file_alignment_for                   # used by the central satellite
central_output_back = dir_backward + file_alignment_back                 # used by the central satellite
Sat_23_output_for = dir_backward + file_alignment_for                    # used by the 1_23 satellite


# Function to read my_alignments file and return two integers
def read_my_alignments(file_path):
    try:
        with open(file_path, "r") as file:
            data = file.read().strip()
            alignment_values = [float(x) for x in data.split()]
            alignment_values = [int(x) for x in alignment_values]
            if len(alignment_values) != 3:
                print(f"ERROR: Unexpected format in my_alignments file. What I got in file {file_path} is: {alignment_values}\n")
            return alignment_values
    except FileNotFoundError:
        return [0, 0, 0]


# Function to write alignment info
def write_alignment_info(file_path, alignment_value, memory_value):
    with open(file_path, "w") as file:
        file.write(f"{alignment_value} {memory_value}")


# Function to process the exchange of alignment data between satellites. 08/01/2025: alignment_values[2] contains the memory information.
def process_satellite_alignment(alignment_values, direction):
    """
    Process and write alignment info for the given satellite.
    The direction helps determine which alignment value (first or second) to write.
    """
    # Case 1: write to the backward satellite's directory
    if "F_sat_back_alignment" in direction:
        write_alignment_info(direction, alignment_values[1], alignment_values[2])
    # Case 2: write to the forward satellite's directory
    elif "B_sat_for_alignment" in direction:
        write_alignment_info(direction, alignment_values[0], alignment_values[2])
    else:
        print(f"Error: the direction {direction} cannot be used since it does not contain the right keyword \n")

# General processing function for any satellite to handle its exchanges
def process_satellite(directory, satellite_connections):
    """
    Each satellite exchanges information with other satellites based on the satellite_connections dictionary.
    """
    alignment_file = os.path.join(directory, "my_alignments.txt")
    alignment_values = read_my_alignments(alignment_file)

    if alignment_values:
        for connection in satellite_connections:
            process_satellite_alignment(alignment_values, connection)


# Define connections between satellites (could also be dynamically set if needed)
satellite_connections_dict = {
    'Sat_1_1': {
        'directory': dir_central,
        'connections': [central_output_for, central_output_back]
    },
    'Sat_1_2': {
        'directory': dir_forward,
        'connections': [forward_output_back]
    },
    'Sat_1_24': {
        'directory': dir_backward,
        'connections': [backward_output_for, backward_output_back]
    },
    'Sat_1_23': {
        'directory': dir_Sat_23,
        'connections': [Sat_23_output_for]
    }
}
i = 1
# Main loop for processing
while True:

    for sat_id, info in satellite_connections_dict.items():
        process_satellite(info["directory"], info["connections"])

    directories_to_check = ['./nos3_rbt/file_sent.txt', './forward_sat/nos3_rbt/file_sent.txt',
                            './Backward_Sat/file_sent.txt', './Sat_1_23/file_sent.txt']  # TODO: ADD DIRECTORIES WHEN NEW VMs ARE ADDED
    
    # Check if there is a file that is being transferred between this sat and other satellites
    for dir_to_check in directories_to_check:
        if os.path.isfile(dir_to_check):
            try:
                with open(dir_to_check) as f:  # Open the file
                    file_data = json.load(f)  # Load JSON data

                src = file_data.get("SOURCE")  # Get the source path
                dest = file_data.get("DESTINATION")  # Get the destination path

                if src and dest:
                    # Copy the file from source to destination
                    shutil.copy(src, dest)
                    print(f"File moved from {src} to {dest}")
                else:
                    print("Error: SOURCE or DESTINATION not found in the JSON data.")
            except: # 13.1.2025: assume large file transfer
                print("Error: Invalid JSON format in 'file_sent.txt' NOW I WILL ASSUME IT IS LARGE FILE TRANSFER")
                # ASSUME IT IS A LARGE FILE TRANSFER, SO READ SRC AND DEST FROM THE FIRST LINE.
                with open(dir_to_check, encoding="ISO-8859-1") as f:
                    first_line = f.readline().strip()
                    remaining_lines = f.readlines()
                    # Read all remaining lines
                    if first_line:
                        try:
                            src, dest = first_line.split()
                            src = dir_to_check

                            # Check if dest is a directory
                            if not os.path.isdir(dest[0:12]):
                                raise ValueError(f"Destination {dest} is not a directory")

                            # Write back the remaining lines to the file
                            with open(dir_to_check, 'w') as fw:
                                fw.writelines(remaining_lines)

                            shutil.copy(src, dest)
                            print(f"File moved from {src} to {dest}")
                        except Exception as e:
                            print(f"Exception: {e}")
                            print(
                                "First line is here but no SRC and DEST, I assume it is a OGS DL. Move it to OGS folder")
                            OGS_dir_filename = f"./OGS/portion_{i}"
                            src = dir_to_check
                            dest = OGS_dir_filename
                            shutil.move(src, dest)
                            i += 1
                    else:
                        print("Error: SOURCE or DESTINATION not found in the first line.")
            # Delete file sent
            try:
                os.remove(dir_to_check)
                print("File moved")
            except:
                print("FIle does not need to be moved")

    sleep(2)
