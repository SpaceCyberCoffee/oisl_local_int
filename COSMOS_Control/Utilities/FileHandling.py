import shutil
import os


def copyFilesToFolder(sourceFolder, fileList, destinationFolder):
    # Ensure the source folder exists
    if not os.path.exists(sourceFolder):
        print(f"The source folder {sourceFolder} does not exist.")
        return

    # Create the destination folder if it doesn't exist
    if not os.path.exists(destinationFolder):
        os.makedirs(destinationFolder)

    # Copy each specified file from the source to the destination folder
    for file_name in fileList:
        file_path = os.path.join(sourceFolder, file_name)
        if os.path.isfile(file_path):
            shutil.copy(file_path, destinationFolder)
        else:
            print(f"The file {file_name} does not exist in the source folder.")
