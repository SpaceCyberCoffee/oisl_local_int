/*******************************************************************************
** File: oisl_app.c
**
** Purpose:
**   This file contains the source code for the OISL application.
**
*******************************************************************************/

/*
** Include Files
*/
#include <arpa/inet.h>
#include "oisl_app.h"
#include "CFDP_Luca.h" 


/*
** Global Data
*/
OISL_AppData_t OISL_AppData;

/*
** Application entry point and main process loop
*/
void OISL_AppMain(void)
{
    int32 status = OS_SUCCESS;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(OISL_PERF_ID);

    /* 
    ** Perform application initialization
    */
    status = OISL_AppInit();
    if (status != CFE_SUCCESS)
    {
        OISL_AppData.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** Main loop
    */
    while (CFE_ES_RunLoop(&OISL_AppData.RunStatus) == true)
    {
        /*
        ** Performance log exit stamp
        */
        CFE_ES_PerfLogExit(OISL_PERF_ID);

        /* 
        ** Pend on the arrival of the next Software Bus message
        ** Note that this is the standard, but timeouts are available
        */
        status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&OISL_AppData.MsgPtr,  OISL_AppData.CmdPipe,  CFE_SB_PEND_FOREVER);
        
        /* 
        ** Begin performance metrics on anything after this line. This will help to determine
        ** where we are spending most of the time during this app execution.
        */
        CFE_ES_PerfLogEntry(OISL_PERF_ID);

        /*
        ** If the CFE_SB_ReceiveBuffer was successful, then continue to process the command packet
        ** If not, then exit the application in error.
        ** Note that a SB read error should not always result in an app quitting.
        */
        if (status == CFE_SUCCESS)
        {
            OISL_ProcessCommandPacket();
        }
        else
        {
            CFE_EVS_SendEvent(OISL_PIPE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: SB Pipe Read Error = %d", (int) status);
            OISL_AppData.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Disable component, which cleans up the interface, upon exit
    */
    OISL_Disable();

    /*
    ** Performance log exit stamp
    */
    CFE_ES_PerfLogExit(OISL_PERF_ID);

    /*
    ** Exit the application
    */
    CFE_ES_ExitApp(OISL_AppData.RunStatus);
} 


/* 
** Initialize application
*/
int32 OISL_AppInit(void)
{
    int32 status = OS_SUCCESS;
    
    OISL_AppData.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Register the events
    */ 
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);    /* as default, no filters are used */
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("OISL: Error registering for event services: 0x%08X\n", (unsigned int) status);
       return status;
    }

    /*
    ** Create the Software Bus command pipe 
    */
    status = CFE_SB_CreatePipe(&OISL_AppData.CmdPipe, OISL_PIPE_DEPTH, "OISL_CMD_PIPE");
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(OISL_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
            "Error Creating SB Pipe,RC=0x%08X",(unsigned int) status);
       return status;
    }
    
    /*
    ** Subscribe to ground commands
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(OISL_CMD_MID), OISL_AppData.CmdPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(OISL_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
            "Error Subscribing to HK Gnd Cmds, MID=0x%04X, RC=0x%08X",
            OISL_CMD_MID, (unsigned int) status);
        return status;
    }

    /*
    ** Subscribe to housekeeping (hk) message requests
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(OISL_REQ_HK_MID), OISL_AppData.CmdPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(OISL_SUB_REQ_HK_ERR_EID, CFE_EVS_EventType_ERROR,
            "Error Subscribing to HK Request, MID=0x%04X, RC=0x%08X",
            OISL_REQ_HK_MID, (unsigned int) status);
        return status;
    }

    /*
    ** TODO: Subscribe to any other messages here
    */


    /* 
    ** Initialize the published HK message - this HK message will contain the 
    ** telemetry that has been defined in the OISL_HkTelemetryPkt for this app.
    */
    CFE_MSG_Init(CFE_MSG_PTR(OISL_AppData.HkTelemetryPkt.TlmHeader),
                   CFE_SB_ValueToMsgId(OISL_HK_TLM_MID),
                   OISL_HK_TLM_LNGTH);

    /*
    ** Initialize the device packet message
    ** This packet is specific to your application
    */
    CFE_MSG_Init(CFE_MSG_PTR(OISL_AppData.DevicePkt.TlmHeader),
                   CFE_SB_ValueToMsgId(OISL_DEVICE_TLM_MID),
                   OISL_DEVICE_TLM_LNGTH);

    /*
    ** TODO: Initialize any other messages that this app will publish
    */


    /* 
    ** Always reset all counters during application initialization 
    */
    OISL_ResetCounters();

    /*
    ** Initialize application data
    ** Note that counters are excluded as they were reset in the previous code block
    */
    OISL_AppData.HkTelemetryPkt.DeviceEnabled = OISL_DEVICE_DISABLED;
    OISL_AppData.HkTelemetryPkt.DeviceHK.DeviceCounter = 0;
    OISL_AppData.HkTelemetryPkt.DeviceHK.DeviceConfig = 0;
    OISL_AppData.HkTelemetryPkt.DeviceHK.DeviceStatus = 0;

    /* Remove the my alignment file at startup if it exists */
    if (remove("/home/jstar/Desktop/github-nos3/components/oisl/fsw/src/my_alignments.txt") == 0) {
        printf("File my alignment deleted successfully.\n");
    } else {
        printf("Failed to delete the alignment file. Does not exist\n");
    }

    /* 
     ** Send an information event that the app has initialized. 
     ** This is useful for debugging the loading of individual applications.
     */
    status = CFE_EVS_SendEvent(OISL_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION,
               "OISL App Initialized. Version %d.%d.%d.%d",
                OISL_MAJOR_VERSION,
                OISL_MINOR_VERSION, 
                OISL_REVISION, 
                OISL_MISSION_REV);	
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("OISL: Error sending initialization event: 0x%08X\n", (unsigned int) status);
    }
    return status;
} 


/* 
** Process packets received on the OISL command pipe
*/
void OISL_ProcessCommandPacket(void)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_GetMsgId(OISL_AppData.MsgPtr, &MsgId);
    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        /*
        ** Ground Commands with command codes fall under the OISL_CMD_MID (Message ID)
        */
        case OISL_CMD_MID:
            OISL_ProcessGroundCommand();
            break;

        /*
        ** All other messages, other than ground commands, add to this case statement.
        */
        case OISL_REQ_HK_MID:
            OISL_ProcessTelemetryRequest();
            break;

        /*
        ** All other invalid messages that this app doesn't recognize, 
        ** increment the command error counter and log as an error event.  
        */
        default:
            OISL_AppData.HkTelemetryPkt.CommandErrorCount++;
            CFE_EVS_SendEvent(OISL_PROCESS_CMD_ERR_EID,CFE_EVS_EventType_ERROR, "OISL: Invalid command packet, MID = 0x%x", CFE_SB_MsgIdToValue(MsgId));
            break;
    }
    return;
} 


/*
** Process ground commands
** TODO: Add additional commands required by the specific component
*/
void OISL_ProcessGroundCommand(void)
{
    int32 status = OS_SUCCESS;
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t CommandCode = 0;

    /*
    ** MsgId is only needed if the command code is not recognized. See default case
    */
    CFE_MSG_GetMsgId(OISL_AppData.MsgPtr, &MsgId);

    /*
    ** Ground Commands, by definition, have a command code (_CC) associated with them
    ** Pull this command code from the message and then process
    */
    CFE_MSG_GetFcnCode(OISL_AppData.MsgPtr, &CommandCode);
    switch (CommandCode)
    {
        /*
        ** NOOP Command
        */
        case OISL_NOOP_CC:
            /*
            ** First, verify the command length immediately after CC identification 
            ** Note that VerifyCmdLength handles the command and command error counters
            */
            if (OISL_VerifyCmdLength(OISL_AppData.MsgPtr, sizeof(OISL_NoArgs_cmd_t)) == OS_SUCCESS)
            {
                /* Second, send EVS event on successful receipt ground commands*/
                CFE_EVS_SendEvent(OISL_CMD_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "OISL: NOOP command received");
                /* Third, do the desired command action if applicable, in the case of NOOP it is no operation */
            }
            break;

        /*
        ** Reset Counters Command
        */
        case OISL_RESET_COUNTERS_CC:
            if (OISL_VerifyCmdLength(OISL_AppData.MsgPtr, sizeof(OISL_NoArgs_cmd_t)) == OS_SUCCESS)
            {
                CFE_EVS_SendEvent(OISL_CMD_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "OISL: RESET counters command received");
                OISL_ResetCounters();
            }
            break;

        /*
        ** Enable Command
        */
        case OISL_ENABLE_CC:
            if (OISL_VerifyCmdLength(OISL_AppData.MsgPtr, sizeof(OISL_NoArgs_cmd_t)) == OS_SUCCESS)
            {
                CFE_EVS_SendEvent(OISL_CMD_ENABLE_INF_EID, CFE_EVS_EventType_INFORMATION, "OISL: Enable command received");
                OISL_Enable();
            }
            break;

        /*
        ** Disable Command
        */
        case OISL_DISABLE_CC:
            if (OISL_VerifyCmdLength(OISL_AppData.MsgPtr, sizeof(OISL_NoArgs_cmd_t)) == OS_SUCCESS)
            {
                CFE_EVS_SendEvent(OISL_CMD_DISABLE_INF_EID, CFE_EVS_EventType_INFORMATION, "OISL: Disable command received");
                OISL_Disable();
            }
            break;

        /*
        ** TODO: Edit and add more command codes as appropriate for the application
        ** Set Configuration Command
        ** Note that this is an example of a command that has additional arguments
        */
        case OISL_CONFIG_CC:
            if (OISL_VerifyCmdLength(OISL_AppData.MsgPtr, sizeof(OISL_Config_cmd_t)) == OS_SUCCESS)
            {
                uint32_t config = ntohl(((OISL_Config_cmd_t*) OISL_AppData.MsgPtr)->DeviceCfg); // command is defined as big-endian... need to convert to host representation
                CFE_EVS_SendEvent(OISL_CMD_CONFIG_INF_EID, CFE_EVS_EventType_INFORMATION, "OISL: Configuration command received: %u", config);
                /* Command device to send HK */
                status = OISL_CommandDevice(&OISL_AppData.OislUart, OISL_DEVICE_CFG_CMD, config);
                if (status == OS_SUCCESS)
                {
                    OISL_AppData.HkTelemetryPkt.DeviceCount++;
                }
                else
                {
                    OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
                }
            }
            break;

        case OISL_SEND_FILE:
            if (OISL_VerifyCmdLength(OISL_AppData.MsgPtr, sizeof(OISL_CFDP_cmd_t)) == OS_SUCCESS)
            {   
                OISL_CFDP_cmd_t *cmd;
                cmd = (OISL_CFDP_cmd_t *)OISL_AppData.MsgPtr; 
                if (cmd->FileName == NULL) {
                    CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_INFORMATION, "Error: cmd->FileName is NULL\n");
                } 
                else {
                    strcpy(OISL_AppData.CFDP.FileName, cmd->FileName);  // Copy the file path
                    CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_INFORMATION, "Filename is %s", OISL_AppData.CFDP.FileName);
                }
                OISL_AppData.CFDP.Target = cmd->Target; 
                CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_EID, CFE_EVS_EventType_INFORMATION, "OISL: Transfer File command received. Trying to reach Sat %u", cmd->Target);
                OISL_SendFile_CFDP();
            }
            else {
                CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_INFORMATION, "OISL: Transfer File command received but error encountered");
            }
            break;

        /*
        ** Invalid Command Codes
        */
        default:
            /* Increment the error counter upon receipt of an invalid command */
            OISL_AppData.HkTelemetryPkt.CommandErrorCount++;
            CFE_EVS_SendEvent(OISL_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                "OISL: Invalid command code for packet, MID = 0x%x, cmdCode = 0x%x", CFE_SB_MsgIdToValue(MsgId), CommandCode);
            break;
            
    }
    return;
} 


/*
** Process Telemetry Request - Triggered in response to a telemetery request
** TODO: Add additional telemetry required by the specific component
*/
void OISL_ProcessTelemetryRequest(void)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t CommandCode = 0;

    /* MsgId is only needed if the command code is not recognized. See default case */
    CFE_MSG_GetMsgId(OISL_AppData.MsgPtr, &MsgId);

    /* Pull this command code from the message and then process */
    CFE_MSG_GetFcnCode(OISL_AppData.MsgPtr, &CommandCode);
    switch (CommandCode)
    {
        case OISL_REQ_HK_TLM:
            OISL_ReportHousekeeping();
            break;

        case OISL_REQ_DATA_TLM:
            OISL_ReportDeviceTelemetry();
            break;

        /*
        ** Invalid Command Codes
        */
        default:
            /* Increment the error counter upon receipt of an invalid command */
            OISL_AppData.HkTelemetryPkt.CommandErrorCount++;
            CFE_EVS_SendEvent(OISL_DEVICE_TLM_ERR_EID, CFE_EVS_EventType_ERROR, 
                "OISL: Invalid command code for packet, MID = 0x%x, cmdCode = 0x%x", CFE_SB_MsgIdToValue(MsgId), CommandCode);
            break;
    }
    return;
}


/* 
** Report Application Housekeeping
*/
void OISL_ReportHousekeeping(void)
{
    int32 status = OS_SUCCESS;

    /* Check that device is enabled */
    if (OISL_AppData.HkTelemetryPkt.DeviceEnabled == OISL_DEVICE_ENABLED)
    {
        status = OISL_RequestHK(&OISL_AppData.OislUart, (OISL_Device_HK_tlm_t*) &OISL_AppData.HkTelemetryPkt.DeviceHK);
        if (status == OS_SUCCESS)
        {
            OISL_AppData.HkTelemetryPkt.DeviceCount++;
        }
        else
        {
            OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
            CFE_EVS_SendEvent(OISL_REQ_HK_ERR_EID, CFE_EVS_EventType_ERROR, 
                    "OISL: Request device HK reported error %d", status);
        }
    }
    /* Intentionally do not report errors if disabled */

    /* Time stamp and publish housekeeping telemetry */
    CFE_SB_TimeStampMsg((CFE_MSG_Message_t *) &OISL_AppData.HkTelemetryPkt);
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *) &OISL_AppData.HkTelemetryPkt, true);
    return;
}


/*
** Collect and Report Device Telemetry
*/
void OISL_ReportDeviceTelemetry(void)
{
    int32 status = OS_SUCCESS;

    /* Check that device is enabled */
    if (OISL_AppData.HkTelemetryPkt.DeviceEnabled == OISL_DEVICE_ENABLED)
    {
        status = OISL_RequestData(&OISL_AppData.OislUart, (OISL_Device_Data_tlm_t*) &OISL_AppData.DevicePkt.Oisl);
        if (status == OS_SUCCESS)
        {   
            OISL_AppData.HkTelemetryPkt.DeviceCount++;
            /* Time stamp and publish data telemetry */
            CFE_SB_TimeStampMsg((CFE_MSG_Message_t *) &OISL_AppData.DevicePkt);
            CFE_SB_TransmitMsg((CFE_MSG_Message_t *) &OISL_AppData.DevicePkt, true);
        }
        else
        {
            OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
            CFE_EVS_SendEvent(OISL_REQ_DATA_ERR_EID, CFE_EVS_EventType_ERROR, 
                    "OISL: Request device data reported error %d", status);
        }
    }
    /* Intentionally do not report errors if disabled */
    return;
}


/*
** Reset all global counter variables
*/
void OISL_ResetCounters(void)
{
    OISL_AppData.HkTelemetryPkt.CommandErrorCount = 0;
    OISL_AppData.HkTelemetryPkt.CommandCount = 0;
    OISL_AppData.HkTelemetryPkt.DeviceErrorCount = 0;
    OISL_AppData.HkTelemetryPkt.DeviceCount = 0;
    return;
} 


/*
** Enable Component
** TODO: Edit for your specific component implementation
*/
void OISL_Enable(void)
{
    int32 status = OS_SUCCESS;

    /* Check that device is disabled */
    if (OISL_AppData.HkTelemetryPkt.DeviceEnabled == OISL_DEVICE_DISABLED)
    {
        /*
        ** Initialize hardware interface data
        ** TODO: Make specific to your application depending on protocol in use
        ** Note that other components provide examples for the different protocols available
        */ 
        OISL_AppData.OislUart.deviceString = OISL_CFG_STRING;
        OISL_AppData.OislUart.handle = OISL_CFG_HANDLE;
        OISL_AppData.OislUart.isOpen = PORT_CLOSED;
        OISL_AppData.OislUart.baud = OISL_CFG_BAUDRATE_HZ;
        OISL_AppData.OislUart.access_option = uart_access_flag_RDWR;

        /* Open device specific protocols */
        status = uart_init_port(&OISL_AppData.OislUart);
        if (status == OS_SUCCESS)
        {
            OISL_AppData.HkTelemetryPkt.DeviceCount++;
            OISL_AppData.HkTelemetryPkt.DeviceEnabled = OISL_DEVICE_ENABLED;
            CFE_EVS_SendEvent(OISL_ENABLE_INF_EID, CFE_EVS_EventType_INFORMATION, "OISL: Device enabled");
        }
        else
        {
            OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
            CFE_EVS_SendEvent(OISL_UART_INIT_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: UART port initialization error %d", status);
        }
    }
    else
    {
        OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
        CFE_EVS_SendEvent(OISL_ENABLE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: Device enable failed, already enabled");
    }
    return;
}


/*
** Disable Component
** TODO: Edit for your specific component implementation
*/
void OISL_Disable(void)
{
    int32 status = OS_SUCCESS;

    /* Check that device is enabled */
    if (OISL_AppData.HkTelemetryPkt.DeviceEnabled == OISL_DEVICE_ENABLED)
    {
        /* Open device specific protocols */
        status = uart_close_port(&OISL_AppData.OislUart);
        if (status == OS_SUCCESS)
        {
            OISL_AppData.HkTelemetryPkt.DeviceCount++;
            OISL_AppData.HkTelemetryPkt.DeviceEnabled = OISL_DEVICE_DISABLED;
            CFE_EVS_SendEvent(OISL_DISABLE_INF_EID, CFE_EVS_EventType_INFORMATION, "OISL: Device disabled");
        }
        else
        {
            OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
            CFE_EVS_SendEvent(OISL_UART_CLOSE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: UART port close error %d", status);
        }
    }
    else
    {
        OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
        CFE_EVS_SendEvent(OISL_DISABLE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: Device disable failed, already disabled");
    }
    /* Remove the my alignment file at startup if it exists */
    if (remove("/home/jstar/Desktop/github-nos3/components/oisl/fsw/src/my_alignments.txt") == 0) {
        printf("File my alignment deleted successfully.\n");
    } else {
        printf("Failed to delete the alignment file. Does not exist\n");
    }
    return;
}

/**
 * @brief Thread function responsible for handling file transfer over OISL or to an OGS.
 *
 * This function checks whether the alignment condition is met before proceeding
 * with file transfer. Depending on the target (forward, backward satellite, or
 * ground station), it waits for the corresponding alignment flag to be set.
 *
 * Target mapping:
 *  - 0: Backward satellite (checks BackwardAlignment flag)
 *  - 1: Forward satellite (checks ForwardAlignment flag)
 *  - 2: Ground station (bypasses alignment check; proceeds to CFDP downlink planning)
 *
 * The function waits up to 10 minutes (60 attempts with 10s sleep) for alignment.
 * If alignment is achieved within the time window, the file is sent using `sendFile()`.
 * If not, it logs an error via CFE event services.
 *
 * After the operation (success or failure), the allocated memory for the file
 * content and transfer data structure is released.
 *
 * @param arg Pointer to a FileTransferData struct containing file content, size, and target.
 * @return NULL Always returns NULL when the thread exits.
 */

void* FileTransferThread(void *arg) {
    FileTransferData *data = (FileTransferData *)arg;

    // Check alignment 
    int waitCount = 0;

    uint8_t *target_to_align = NULL;
    if (data->target == 0) {
        target_to_align = &OISL_AppData.DevicePkt.Oisl.BackwardAlignment;
    }
    else if (data->target == 1)
    {
        target_to_align = &OISL_AppData.DevicePkt.Oisl.ForwardAlignment;
    }
    else if (data->target == 2)
    {   
        printf("Trying to transfer data to a OGS. The filecontent will be passed to CFDP automatically to plan the DL and eventual relay.\n");
        target_to_align = &OISL_AppData.HkTelemetryPkt.DeviceEnabled;
    }
    else {
        printf("Unknown taget to align with, or method not yet impemented for target %u", data->target);
        free(data->fileContent);
        free(data);
        return NULL;
    }
    
    while (*target_to_align == 0 && waitCount < 60) { 
        printf("OISL FILE CFDP: The alignment condition is not verified, waiting for alignment.\n");
        sleep(10);  // Wait for 10 seconds before checking again
        waitCount += 1;
    }

    // If the alignment condition is verified, proceed with the file transfer
    if (*target_to_align) { 
        sendFile(data->fileContent, data->fileSize);
    } else {
        CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "OISL FILE CFDP: Alignment condition not met after waiting.");
    }

    free(data->fileContent);
    free(data);
    return NULL;
}

/**
 * @brief Sends a file using the CFDP (CCSDS File Delivery Protocol) over the Optical Inter-Satellite Link (OISL).
 *
 * This function performs the following steps:
 *  - Checks if the OISL device is enabled.
 *  - Attempts to open the file specified in OISL_AppData.CFDP.FileName.
 *  - Reads the file contents into dynamically allocated memory.
 *  - Logs the file size using a CFE event.
 *  - Prepares a FileTransferData structure for the file transfer.
 *  - Spawns a new detached pthread to handle the file transfer asynchronously.
 * 
 * Error conditions, such as failure to open the file, memory allocation errors,
 * and thread creation failures, are logged and increment an error counter.
 * 
 * Preconditions:
 *  - OISL_AppData.CFDP.FileName and OISL_AppData.CFDP.Target must be set correctly.
 *  - OISL device must be enabled (OISL_DEVICE_ENABLED).
 * 
 * Postconditions:
 *  - A file transfer is initiated in a separate thread if all checks pass.
 * 
 * Threaded transfer is handled via the FileTransferThread function.
 */

void OISL_SendFile_CFDP(void)
{   
    const char *filePath = OISL_AppData.CFDP.FileName;  
    FILE *file;
    size_t fileSize;
    char *fileContent;

    /* Check that device is enabled */
    if (OISL_AppData.HkTelemetryPkt.DeviceEnabled == OISL_DEVICE_ENABLED)
    {
        // Open the file for reading
        file = fopen(filePath, "r");
        if (file == NULL)
        {
            OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
            CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: Unable to open to transfer file %s.", filePath);
            return;
        }

        // Determine the file size
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        rewind(file);

        // Allocate memory to hold the file content
        fileContent = (char *)malloc(fileSize + 1);
        if (fileContent == NULL)
        {
            fclose(file);
            OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
            CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: Memory allocation failed for file content.");
            return;
        }

        // Read the file content
        size_t bytesRead = fread(fileContent, 1, fileSize, file);
        if (bytesRead != fileSize)
        {
            free(fileContent);
            fclose(file);
            OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
            CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: Failed to read the entire file %s.", filePath);
            return;
        }
        fileContent[fileSize] = '\0';  // Null-terminate the file content

        // Close the file
        fclose(file);

        // Log the file size and estimated transfer time
        CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_EID, CFE_EVS_EventType_INFORMATION,
                          "OISL FILE CFDP: Just read the file with size: %zu bytes.", fileSize);

        // Allocate memory for file transfer data and create a new thread
        FileTransferData *transferData = (FileTransferData *)malloc(sizeof(FileTransferData));
        if (transferData == NULL) {
            OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
            CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: Memory allocation failed for transfer data.");
            free(fileContent);
            return;
        }
        transferData->fileContent = fileContent;
        transferData->fileSize = fileSize;
        transferData->target = OISL_AppData.CFDP.Target;

        pthread_t transferThread;
        if (pthread_create(&transferThread, NULL, FileTransferThread, (void *)transferData) != 0) {
            OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
            CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: Failed to create file transfer thread.");
            free(transferData->fileContent);
            free(transferData);
        } else {
            pthread_detach(transferThread);  // Detach thread to run independently
        }
    }
    else
    {
        OISL_AppData.HkTelemetryPkt.DeviceErrorCount++;
        CFE_EVS_SendEvent(OISL_CMD_SEND_FILE_ERR_EID, CFE_EVS_EventType_ERROR, "OISL: Device is disabled, cannot send file.");
    }
    return;
}

/*
** Verify command packet length matches expected
*/
int32 OISL_VerifyCmdLength(CFE_MSG_Message_t * msg, uint16 expected_length)
{     
    int32 status = OS_SUCCESS;
    CFE_SB_MsgId_t msg_id = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t cmd_code = 0;
    size_t actual_length = 0;

    CFE_MSG_GetSize(msg, &actual_length);
    if (expected_length == actual_length)
    {
        /* Increment the command counter upon receipt of an invalid command */
        OISL_AppData.HkTelemetryPkt.CommandCount++;
    }
    else
    {
        CFE_MSG_GetMsgId(msg, &msg_id);
        CFE_MSG_GetFcnCode(msg, &cmd_code);

        CFE_EVS_SendEvent(OISL_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
           "Invalid msg length: ID = 0x%X,  CC = %d, Len = %ld, Expected = %d",    // Invalid msg length: ID = 0x1898,  CC = 5, Len = 9, Expected = 24
              CFE_SB_MsgIdToValue(msg_id), cmd_code, actual_length, expected_length);

        status = OS_ERROR;

        /* Increment the command error counter upon receipt of an invalid command */
        OISL_AppData.HkTelemetryPkt.CommandErrorCount++;
    }
    return status;
} 
