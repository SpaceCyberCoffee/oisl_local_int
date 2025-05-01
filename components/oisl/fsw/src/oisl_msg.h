/*******************************************************************************
** File:
**   oisl_msg.h
**
** Purpose:
**  Define OISL application commands and telemetry messages
**
*******************************************************************************/
#ifndef _OISL_MSG_H_
#define _OISL_MSG_H_

#include "cfe.h"
#include "oisl_device.h"


/*
** Ground Command Codes
** TODO: Add additional commands required by the specific component
*/
#define OISL_NOOP_CC                 0
#define OISL_RESET_COUNTERS_CC       1
#define OISL_ENABLE_CC               2
#define OISL_DISABLE_CC              3
#define OISL_CONFIG_CC               4
#define OISL_SEND_FILE               5


/* 
** Telemetry Request Command Codes
** TODO: Add additional commands required by the specific component
*/
#define OISL_REQ_HK_TLM              0
#define OISL_REQ_DATA_TLM            1


/*
** Generic "no arguments" command type definition
*/
typedef struct
{
    /* Every command requires a header used to identify it */
    CFE_MSG_CommandHeader_t CmdHeader;

} OISL_NoArgs_cmd_t;


/*
** OISL write configuration command
*/
typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
    uint32   DeviceCfg;

} OISL_Config_cmd_t;

/*
** OISL transfer file command
*/
typedef struct
{
    /* Every command requires a header used to identify it */
    CFE_MSG_CommandHeader_t CmdHeader;          //size is 8
    char                    FileName[100];
    uint8                   Target;
} OISL_CFDP_cmd_t;


/*
** OISL device telemetry definition
*/
typedef struct 
{
    CFE_MSG_TelemetryHeader_t TlmHeader;
    OISL_Device_Data_tlm_t Oisl;

} __attribute__((packed)) OISL_Device_tlm_t;
#define OISL_DEVICE_TLM_LNGTH sizeof ( OISL_Device_tlm_t )


/*
** OISL housekeeping type definition
*/
typedef struct 
{
    CFE_MSG_TelemetryHeader_t TlmHeader;
    uint8   CommandErrorCount;
    uint8   CommandCount;
    uint8   DeviceErrorCount;
    uint8   DeviceCount;
  
    /*
    ** TODO: Edit and add specific telemetry values to this struct
    */
    uint8   DeviceEnabled;
    OISL_Device_HK_tlm_t DeviceHK;

} __attribute__((packed)) OISL_Hk_tlm_t;
#define OISL_HK_TLM_LNGTH sizeof ( OISL_Hk_tlm_t )

#endif /* _OISL_MSG_H_ */
