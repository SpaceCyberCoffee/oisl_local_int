/*******************************************************************************
** File: oisl_device.c
**
** Purpose:
**   This file contains the source code for the OISL device.
**
*******************************************************************************/

/*
** Include Files
*/
#include "oisl_device.h"
#include <stdio.h>
#include <math.h>
#include "CFDP_Luca.h"

/*
** Constants representing the paths of different files
*/
const char* file_beam_forward = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/F.txt"; 
const char* file_beam_backward = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/B.txt"; 
const char* file_beam_receiver = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/fileInput/ireceive.txt";    // if this file is present, then this sat is the receving end of a transmission.

const char* alignment_info_forward = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/F_sat_back_alignment.txt";
const char* alignment_info_backward = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/B_sat_for_alignment.txt";
const char* my_alignments = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/my_alignments.txt";

const char* tle_file_path_forward = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/forward_sat.txt";
const char* tle_file_path_backward = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/backward_sat.txt";

const char* ECI_position_file_path = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/ECI_position.txt";

extern struct __cmdline cmdline;

/* 
** Generic read data from device
*/
int32_t OISL_ReadData(uart_info_t* device, uint8_t* read_data, uint8_t data_length)
{
    int32_t status = OS_SUCCESS;
    int32_t bytes = 0;
    int32_t bytes_available = 0;
    uint8_t ms_timeout_counter = 0;

    /* Wait until all data received or timeout occurs */
    bytes_available = uart_bytes_available(device);
    while((bytes_available < data_length) && (ms_timeout_counter < OISL_CFG_MS_TIMEOUT))
    {   
        ms_timeout_counter++;
        OS_TaskDelay(1);
        bytes_available = uart_bytes_available(device);
    }

    if (ms_timeout_counter < OISL_CFG_MS_TIMEOUT)
    {   
        /* Limit bytes available */
        if (bytes_available > data_length)
        {
            bytes_available = data_length;
        }
        
        /* Read data */
        bytes = uart_read_port(device, read_data, bytes_available);
        if (bytes != bytes_available)
        {   
            OS_printf("  OISL_ReadData: Bytes read != to requested! \n");
            #ifdef OISL_CFG_DEBUG
                OS_printf("  OISL_ReadData: Bytes read != to requested! \n");
            #endif
            status = OS_ERROR;
        } /* uart_read */
    }
    else
    {   OS_printf("Errore qui in OISL_ReadData.");
        status = OS_ERROR;
    } /* ms_timeout_counter */

    return status;
}


/* 
** Generic command to device
** Note that confirming the echoed response is specific to this implementation
*/
int32_t OISL_CommandDevice(uart_info_t* device, uint8_t cmd_code, uint32_t payload)
{
    int32_t status = OS_SUCCESS;
    int32_t bytes = 0;
    uint8_t write_data[OISL_DEVICE_CMD_SIZE];
    uint8_t read_data[OISL_DEVICE_DATA_SIZE];

    /* Prepare command */
    write_data[0] = OISL_DEVICE_HDR_0;
    write_data[1] = OISL_DEVICE_HDR_1;
    write_data[2] = cmd_code;
    write_data[3] = payload >> 24;
    write_data[4] = payload >> 16;
    write_data[5] = payload >> 8;
    write_data[6] = payload;
    write_data[7] = OISL_DEVICE_TRAILER_0;
    write_data[8] = OISL_DEVICE_TRAILER_1;

    /* Flush any prior data */
    status = uart_flush(device);
    if (status == UART_SUCCESS)
    {
        /* Write data */
        bytes = uart_write_port(device, write_data, OISL_DEVICE_CMD_SIZE);
        #ifdef OISL_CFG_DEBUG
            OS_printf("  OISL_CommandDevice[%d] = ", bytes);
            for (uint32_t i = 0; i < OISL_DEVICE_CMD_SIZE; i++)
            {
                OS_printf("%02x", write_data[i]);
            }
            OS_printf("\n");
        #endif
        if (bytes == OISL_DEVICE_CMD_SIZE)
        {
            status = OISL_ReadData(device, read_data, OISL_DEVICE_CMD_SIZE);
            if (status == OS_SUCCESS)
            {
                /* Confirm echoed response */
                bytes = 0;
                while ((bytes < (int32_t) OISL_DEVICE_CMD_SIZE) && (status == OS_SUCCESS))
                {
                    if (read_data[bytes] != write_data[bytes])
                    {
                        status = OS_ERROR;
                    }
                    bytes++;
                }
            } /* OISL_ReadData */
            else
            {
                #ifdef OISL_CFG_DEBUG
                    OS_printf("OISL_CommandDevice - OISL_ReadData returned %d \n", status);
                #endif
            }
        } 
        else
        {
            #ifdef OISL_CFG_DEBUG
                OS_printf("OISL_CommandDevice - uart_write_port returned %d, expected %d \n", bytes, OISL_DEVICE_CMD_SIZE);
            #endif
        } /* uart_write */
    } /* uart_flush*/
    return status;
}


/*
** Request housekeeping command
*/
int32_t OISL_RequestHK(uart_info_t* device, OISL_Device_HK_tlm_t* data)
{
    int32_t status = OS_SUCCESS;
    uint8_t read_data[OISL_DEVICE_HK_SIZE];

    /* Command device to send HK */
    status = OISL_CommandDevice(device, OISL_DEVICE_REQ_HK_CMD, 0);
    if (status == OS_SUCCESS)
    {
        /* Read HK data */
        status = OISL_ReadData(device, read_data, sizeof(read_data));
        if (status == OS_SUCCESS)
        {
            #ifdef OISL_CFG_DEBUG
                OS_printf("  OISL_RequestHK = ");
                for (uint32_t i = 0; i < sizeof(read_data); i++)
                {
                    OS_printf("%02x", read_data[i]);
                }
                OS_printf("\n");
            #endif

            /* Verify data header and trailer */
            if ((read_data[0]  == OISL_DEVICE_HDR_0)     && 
                (read_data[1]  == OISL_DEVICE_HDR_1)     && 
                (read_data[14] == OISL_DEVICE_TRAILER_0) && 
                (read_data[15] == OISL_DEVICE_TRAILER_1) )
            {
                data->DeviceCounter  = read_data[2] << 24;
                data->DeviceCounter |= read_data[3] << 16;
                data->DeviceCounter |= read_data[4] << 8;
                data->DeviceCounter |= read_data[5];

                data->DeviceConfig  = read_data[6] << 24;
                data->DeviceConfig |= read_data[7] << 16;
                data->DeviceConfig |= read_data[8] << 8;
                data->DeviceConfig |= read_data[9];

                data->DeviceStatus  = read_data[10] << 24;
                data->DeviceStatus |= read_data[11] << 16;
                data->DeviceStatus |= read_data[12] << 8;
                data->DeviceStatus |= read_data[13];

                #ifdef OISL_CFG_DEBUG
                    OS_printf("  Header  = 0x%02x%02x  \n", read_data[0], read_data[1]);
                    OS_printf("  Counter = 0x%08x      \n", data->DeviceCounter);
                    OS_printf("  Config  = 0x%08x      \n", data->DeviceConfig);
                    OS_printf("  Status  = 0x%08x      \n", data->DeviceStatus);
                    OS_printf("  Trailer = 0x%02x%02x  \n", read_data[14], read_data[15]);
                #endif
            }
            else
            {
                #ifdef OISL_CFG_DEBUG
                    OS_printf("  OISL_RequestHK: OISL_ReadData reported error %d \n", status);
                #endif 
                status = OS_ERROR;
            }
        } /* OISL_ReadData */
    }
    else
    {
        #ifdef OISL_CFG_DEBUG
            OS_printf("  OISL_RequestHK: OISL_CommandDevice reported error %d \n", status);
        #endif 
    }
    return status;
}

/* Function to read alignment info from file */
int read_alignment_info(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        // If file doesn't exist, return 0
        return 0;
    }

    int alignment_value;
    // Read the first value in the file (expected to be 1 or 0)
    if (fscanf(file, "%d", &alignment_value) != 1) {
        fclose(file);
        return 0; // Return 0 if reading fails
    }

    fclose(file);
    return alignment_value;
}

/*
** Request data command
*/
/**
 * @brief Requests and processes OISL device telemetry and alignment data.
 *
 * This function is responsible for requesting housekeeping (HK) data from the Optical Inter-Satellite Link (OISL) hardware,
 * parsing the returned information, and populating the telemetry structure `OISL_Device_Data_tlm_t` with device counters,
 * alignment states, ISL vector data, and connection status with other satellites or a ground station.
 * 
 * Key operations include:
 * 1. Sending a command to the OISL device to request telemetry data.
 * 2. Reading a compact telemetry packet containing device counters and alignment flags.
 * 3. Validating the packet using header and trailer bytes.
 * 4. Parsing forward, backward, and ground station alignment flags from the data.
 * 5. Obtaining current ISL direction vectors using `get_isl_vectors()`.
 * 6. Evaluating connection availability with neighbors based on alignment status and external files.
 * 7. Setting the `TransferActive` status, including checking if this satellite is currently receiving data.
 * 8. Creating or removing symbolic files that visualize active laser beams for telemetry downlink or relay.
 * 9. Storing the current memory usage and availability from the platform.
 * 10. Writing out a local file with alignment and memory status, enabling other satellites to perform a handshake.
 *
 * @param device Pointer to the UART device interface used to communicate with the OISL hardware.
 * @param data Pointer to the telemetry structure to be filled with parsed values.
 * @return int32_t Returns OS_SUCCESS (0) on success or OS_ERROR on failure.
 */

int32_t OISL_RequestData(uart_info_t* device, OISL_Device_Data_tlm_t* data)
{
    int32_t status = OS_SUCCESS;
    uint8_t read_data[11]; // the 6 doubles (48 bytes) are not considered.


    /* Command device to send HK */
    status = OISL_CommandDevice(device, OISL_DEVICE_REQ_DATA_CMD, 0);
    if (status == OS_SUCCESS)
    {
        /* Read HK data */
        status = OISL_ReadData(device, read_data, sizeof(read_data));
        if (status == OS_SUCCESS)
        {   

            #ifdef OISL_CFG_DEBUG
                OS_printf("  OISL_RequestData = ");
                for (uint32_t i = 0; i < sizeof(read_data); i++)
                {
                    OS_printf("%02x", read_data[i]);
                }
                OS_printf("\n");
            #endif

            /* Verify data header and trailer */
            if ((read_data[0]  == OISL_DEVICE_HDR_0)     && 
                (read_data[1]  == OISL_DEVICE_HDR_1)     && 
                (read_data[8] == OISL_DEVICE_TRAILER_0) && 
                (read_data[9] == OISL_DEVICE_TRAILER_1) )
            {
                data->DeviceCounter  = read_data[2] << 24;
                data->DeviceCounter |= read_data[3] << 16;
                data->DeviceCounter |= read_data[4] << 8;
                data->DeviceCounter |= read_data[5];
            }
        } 
        else
        {   
            OS_printf("  OISL_RequestData: Invalid data read! \n");
            #ifdef OISL_CFG_DEBUG
                OS_printf("  OISL_RequestData: Invalid data read! \n");
            #endif 
            status = OS_ERROR;
        } /* OISL_ReadData */
    }
    else
    {   OS_printf("  OISL_RequestData: OISL_CommandDevice reported error %d \n", status);
        #ifdef OISL_CFG_DEBUG
            OS_printf("  OISL_RequestData: OISL_CommandDevice reported error %d \n", status);
        #endif 
    }

    // Call the new submethod to get the ISL vectors
    double backward_isl_vector[3];
    double forward_isl_vector[3];
    get_isl_vectors(forward_isl_vector, backward_isl_vector);

    data->BACKWARD_ISL_X = backward_isl_vector[0];
    data->BACKWARD_ISL_Y = backward_isl_vector[1];
    data->BACKWARD_ISL_Z = backward_isl_vector[2];
    data->FORWARD_ISL_X = forward_isl_vector[0];
    data->FORWARD_ISL_Y = forward_isl_vector[1];
    data->FORWARD_ISL_Z = forward_isl_vector[2];

    /* Read Forward Alignment */
    data->ForwardAlignment = read_data[6];

    /* Read Backward Alignment */
    data->BackwardAlignment = read_data[7];

    /* Read OGS Alignment */
    data->OGSAlignment = read_data[8];   

    // TODO: Is it possible to get the alignment and memory information of the neighbour sats in a better way?

    // /* Connection with Forward Satellite */
    if (data->ForwardAlignment == 0) { 
        data->ForwardConnection = 0; 
    } 
    else { 
        data->ForwardConnection = read_alignment_info(alignment_info_forward); 
    } 

    /* Connection with Backward Satellite */
    if (data->BackwardAlignment == 0) {
        data->BackwardConnection = 0;
    }
    else {
        data->BackwardConnection = read_alignment_info(alignment_info_backward);
    }

    // Add TransferActive info
    data->TransferActive = *transferActive;
    // Check if this satellite is acting as the receiving end of a transfer and if so adjust TransferActive value
    FILE *file_receiver = fopen(file_beam_receiver, "r"); 
    if (file_receiver) { 
        data->TransferActive = 1; 
        fclose(file_receiver); 
    }

    // Create or delete the file based on Connection values. FOR OGS MODE DRAW BEAM FORWARD, ALIGNED WITH b2, because the mode does so.
    FILE *fpdef = NULL;
    if ((data->ForwardConnection == 1 && data->TransferActive == 1) || (data->OGSAlignment == 1 && data->TransferActive == 1)) { 
        fpdef = fopen(file_beam_forward, "w"); 
        // Create the file 
        if (fpdef == NULL) { 
            OS_printf("Error creating the file "); 
        } 
        else { 
            fclose(fpdef); 
        } 
    } 
    else { 
        remove(file_beam_forward); 
    }
    if (data->BackwardConnection == 1 && data->TransferActive == 1) { 
        fpdef = fopen(file_beam_backward, "w"); 
        // Create the file 
        if (fpdef == NULL) { 
            OS_printf("Error creating the file "); 
        } 
        else { 
            fclose(fpdef); 
        } 
    } 
    else { 
        remove(file_beam_backward); 
    }

    // Add memory info
    data->MemoryUsed = (double)memoryInfo->currentUsed; //Bytes 
    data->MemoryAvailable = memoryInfo->isAvailable; 

    /* Write to file the alignment conditions. Used by other sats to establish connection. Send also the memoryUsed information. This simulates a kind of handshake before starting the iteraction */
    FILE *fp = fopen(my_alignments, "w");
    if (fp == NULL)
    {
        OS_printf("Error opening the file %s", my_alignments);
    }
    fprintf(fp, "%u %u %f", data->ForwardAlignment,  data->BackwardAlignment,  data->MemoryUsed);
    fclose(fp);

    return status;
}

// Submethod to get the forward and backward OISL vectors
/**
 * @brief Computes the normalized Optical Inter-Satellite Link (OISL) direction vectors
 *        between the current (central) satellite and both its forward and backward neighbors.
 *
 * This function calls a Python script to propagate the TLEs of the forward and backward satellites,
 * obtaining their current ECI position vectors at the current simulation time. It also reads the
 * current satellite's ECI position from a file. The function then computes the directional vectors
 * from the central satellite to the forward and backward satellites and normalizes them to obtain
 * unit vectors pointing along the forward and backward OISLs.
 *
 * @param[out] forward_ISL_vector  Pointer to an array of 3 doubles to store the forward OISL unit vector.
 * @param[out] backward_ISL_vector Pointer to an array of 3 doubles to store the backward OISL unit vector.
 *
 * The forward and backward satellite TLE paths are defined in:
 *     - tle_file_path_forward
 *     - tle_file_path_backward
 * The current satellite's ECI position is read from:
 *     - ECI_position_file_path
 *
 * Note: Make sure the external Python script `orbital_propagation.py` prints the ECI position in the format:
 *       ECI propagated Position: [x, y, z]
 */

void get_isl_vectors(double* forward_ISL_vector, double* backward_ISL_vector)
{
    FILE* fp;
    char buffer[128];
    char command[256];
    const char* tle_files [] = {tle_file_path_backward, tle_file_path_forward};
    // Variable to store the position vectors
    double eci_position[2][3]; // 2 satellites, each with a 3D position vector, back and for

    // Retrieve current SIM time
    CFE_TIME_SysTime_t nowT = CFE_TIME_GetTime();
    uint32 seconds = nowT.Seconds;
    uint32 subseconds = nowT.Subseconds;

    // Convert the time to double
    double pd = (double)seconds + ((double)subseconds / 4294967296.0); // 4294967296.0 = 2^32

    for (int i=0; i<2; i++) {
        // Construct the command with arguments
        snprintf(command, sizeof(command),
             "python3 /mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/orbital_propagation.py %s %f",
             tle_files[i], pd);

        // Run the Python script and capture its output
        fp = popen(command, "r");
        if (fp == NULL) {
            fprintf(stderr, "Failed to run Python script\n");
        }
    
        // Read the output and parse the position vector
        while (fgets(buffer, sizeof(buffer) - 1, fp) != NULL) {
            if (sscanf(buffer, "ECI propagated Position: [%lf, %lf, %lf]", &eci_position[i][0], &eci_position[i][1], &eci_position[i][2]) == 3) { 
                // Successfully parsed the position vector
                // OS_printf("Got the propagation");
            } else {
                // Print other output lines if any
                printf("%s", buffer);
            }
        }
        pclose(fp);
    }
    double backward_satellite [] = {eci_position[0][0], eci_position[0][1], eci_position[0][2]};
    double forward_satellite [] = {eci_position[1][0], eci_position[1][1], eci_position[1][2]};

    // Retrieve the ECI GPS position of this satellite
    FILE *file_in = fopen(ECI_position_file_path, "r");
    double x, y, z;
    fscanf(file_in, "%lf %lf %lf", &x, &y, &z);
    fclose(file_in);
    double central_sat [] = {x/1000, y/1000, z/1000};

    // Calculate the ISL vector with the forward satellite
    double ISL_vector_forward[] = {forward_satellite[0] - central_sat[0], forward_satellite[1] - central_sat[1], forward_satellite[2] - central_sat[2]};
    double ISL_vector_backward[] = {backward_satellite[0] - central_sat[0] , backward_satellite[1] - central_sat[1], backward_satellite[2] - central_sat[2]};

    // Normalize the difference vector to get the unit vector
    UNITV2(ISL_vector_backward);
    UNITV2(ISL_vector_forward);

    // Assign the normalized vectors to the output parameters
    backward_ISL_vector[0] = ISL_vector_backward[0];
    backward_ISL_vector[1] = ISL_vector_backward[1];
    backward_ISL_vector[2] = ISL_vector_backward[2];

    forward_ISL_vector[0] = ISL_vector_forward[0];
    forward_ISL_vector[1] = ISL_vector_forward[1];
    forward_ISL_vector[2] = ISL_vector_forward[2];

}


/*  Normalize a 3-vector if it is non-zero.                           */
void UNITV2(double V[3])
{
      double A;

      A=sqrt(V[0]*V[0]+V[1]*V[1]+V[2]*V[2]);
      if (A > 0.0) {
         V[0]/=A;
         V[1]/=A;
         V[2]/=A;
      }
}
