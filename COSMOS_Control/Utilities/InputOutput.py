"""-------------------------------------------------------------------------\n
.. centered:: GERMAN AEROSPACE CENTER (DLR) \n
.. centered:: Galileo Competence Center - Systems Analysis and Evolution \n
.. centered:: Copyright © 2020. All rights reserved. \n
----------------------------------------------------------------------------\n
.. rubric:: Library: Contains function for reading and writting files\n
Author(s):
    Ulrich Kling < ulrich.kling@dlr.de > \n
Description:
    This library contains functions to read and write files of different formats.\n
Module:
    JsonToGMAT.py \n
Version:
    0.8.0 \n
.. hlist::
   :columns: 1

    * .. versionadded:: 0.8.0
       Initial Upload

----------------------------------------------------------------------------\n
"""

import json as js
import os
import sys

def readJson(jsonFile):
    r"""
        Read a Json-File. The content of the file is returned as Python structure
        as defined in the Json-File.

        Args:
            filename (str): Path to Json-File

        Returns:
            dict: Content of Json-File as Python structure e.g. dictionary as defined in the
            Json-File

        Error:
            - Error [1]: Unknown Error in function readGMATJson
            - Error [25]: .json-file does not exist
            - Error [27]: .json-file is empty

        Examples:
            >>> In:    readJson('./DataInput/testJson.json')
            >>> Out:   {'example1': {'example2': ...}, 'example3':{...}}
        """
    print(f'Loading json file {jsonFile}')
    try:
        content = ''
        with open(jsonFile) as f:
            content = f.read()
        print('Content read')
        if content == '':
            raise TypeError('File is Empty')

        f = open(jsonFile)
        jsData = js.load(f)
        f.close()

    except Exception as e:
        if type(e).__name__ == 'FileNotFoundError':
            print('Error: Json-File does not exist.')
            sys.exit(25)
        elif type(e).__name__ == 'TypeError':
            print('Error: Json-File is empty.')
            sys.exit(27)
        else:
            print('Unknown Error in function JsonToGMAT/readGMATJson')
            sys.exit(1)
    else:
        return jsData


def saveDataToFile(dataList, fileName='tlm_output.txt', mode='w'):
    with open(fileName, mode) as file:
        # Iterate through the list and write each element to the file
        for element in dataList:
            file.write(str(element) + '\n')


def prnt(output, printOut='y'):
    if printOut == 'y':
        print(output)