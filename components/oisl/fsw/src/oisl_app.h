/*******************************************************************************
** File: oisl_app.h
**
** Purpose:
**   This is the main header file for the OISL application.
**
*******************************************************************************/
#ifndef _OISL_APP_H_
#define _OISL_APP_H_

/*
** Include Files
*/
#include "cfe.h"
#include "oisl_device.h"
#include "oisl_events.h"
#include "oisl_platform_cfg.h"
#include "oisl_perfids.h"
#include "oisl_msg.h"
#include "oisl_msgids.h"
#include "oisl_version.h"
#include "hwlib.h"

#include <pthread.h>

// Structure to hold file transfer data
// TODO: ADD THE FILENAME PARAMETER. The file will have: type (Text, CMD, TM), SOURCE DIRECTORY and then the destination in the receiving sat. THIS IS WRITTEN WITHIN THE FILE. 
typedef struct {
    char *fileContent;
    size_t fileSize;
    char target;
} FileTransferData;


/*
** Specified pipe depth - how many messages will be queued in the pipe
*/
#define OISL_PIPE_DEPTH            32


/*
** Enabled and Disabled Definitions
*/
#define OISL_DEVICE_DISABLED       0
#define OISL_DEVICE_ENABLED        1


/*
** OISL global data structure
** The cFE convention is to put all global app data in a single struct. 
** This struct is defined in the `oisl_app.h` file with one global instance 
** in the `.c` file.
*/
typedef struct
{
    /*
    ** Housekeeping telemetry packet
    ** Each app defines its own packet which contains its OWN telemetry
    */
    OISL_Hk_tlm_t   HkTelemetryPkt;   /* OISL Housekeeping Telemetry Packet */
    
    /*
    ** Operational data  - not reported in housekeeping
    */
    CFE_MSG_Message_t * MsgPtr;             /* Pointer to msg received on software bus */
    CFE_SB_PipeId_t CmdPipe;            /* Pipe Id for HK command pipe */
    uint32 RunStatus;                   /* App run status for controlling the application state */

    /*
	** Device data 
    ** TODO: Make specific to your application
	*/
    OISL_Device_tlm_t DevicePkt;      /* Device specific data packet */

    /* 
    ** Device protocol
    ** TODO: Make specific to your application
    */ 
    uart_info_t OislUart;             /* Hardware protocol definition */

    OISL_CFDP_cmd_t CFDP;

} OISL_AppData_t;


/*
** Exported Data
** Extern the global struct in the header for the Unit Test Framework (UTF).
*/
extern OISL_AppData_t OISL_AppData; /* OISL App Data */


/*
**
** Local function prototypes.
**
** Note: Except for the entry point (OISL_AppMain), these
**       functions are not called from any other source module.
*/
void  OISL_AppMain(void);
int32 OISL_AppInit(void);
void  OISL_ProcessCommandPacket(void);
void  OISL_ProcessGroundCommand(void);
void  OISL_ProcessTelemetryRequest(void);
void  OISL_ReportHousekeeping(void);
void  OISL_ReportDeviceTelemetry(void);
void  OISL_ResetCounters(void);
void  OISL_Enable(void);
void  OISL_Disable(void);
void  OISL_SendFile_CFDP(void);
void* FileTransferThread(void *arg);
int32 OISL_VerifyCmdLength(CFE_MSG_Message_t * msg, uint16 expected_length);

#endif /* _OISL_APP_H_ */
