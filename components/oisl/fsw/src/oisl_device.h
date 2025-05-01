/*******************************************************************************
** File: oisl_device.h
**
** Purpose:
**   This is the header file for the OISL device.
**
*******************************************************************************/
#ifndef _OISL_DEVICE_H_
#define _OISL_DEVICE_H_

/*
** Required header files.
*/
#include "device_cfg.h"
#include "hwlib.h"
#include "oisl_platform_cfg.h" 



/*
** Type definitions
** TODO: Make specific to your application
*/
#define OISL_DEVICE_HDR              0xDEAD
#define OISL_DEVICE_HDR_0            0xDE
#define OISL_DEVICE_HDR_1            0xAD

#define OISL_DEVICE_NOOP_CMD         0x00
#define OISL_DEVICE_REQ_HK_CMD       0x01
#define OISL_DEVICE_REQ_DATA_CMD     0x02
#define OISL_DEVICE_CFG_CMD          0x03

#define OISL_DEVICE_TRAILER          0xBEEF
#define OISL_DEVICE_TRAILER_0        0xBE
#define OISL_DEVICE_TRAILER_1        0xEF

#define OISL_DEVICE_HDR_TRL_LEN      4
#define OISL_DEVICE_CMD_SIZE         9

/*
** OISL device housekeeping telemetry definition
*/
typedef struct
{
    uint32_t  DeviceCounter;
    uint32_t  DeviceConfig;
    uint32_t  DeviceStatus;

} __attribute__((packed)) OISL_Device_HK_tlm_t;
#define OISL_DEVICE_HK_LNGTH sizeof ( OISL_Device_HK_tlm_t )
#define OISL_DEVICE_HK_SIZE OISL_DEVICE_HK_LNGTH + OISL_DEVICE_HDR_TRL_LEN


/*
** OISL device data telemetry definition
*/
typedef struct
{
    uint32_t  DeviceCounter;
    double    BACKWARD_ISL_X;
    double    BACKWARD_ISL_Y;
    double    BACKWARD_ISL_Z;
    double    FORWARD_ISL_X;
    double    FORWARD_ISL_Y;
    double    FORWARD_ISL_Z;
    uint8_t   ForwardAlignment;
    uint8_t   BackwardAlignment;
    uint8_t   ForwardConnection;
    uint8_t   BackwardConnection;
    uint8_t   OGSAlignment;
    double    MemoryUsed;
    uint8_t   MemoryAvailable;
    uint8_t   TransferActive;
} __attribute__((packed)) OISL_Device_Data_tlm_t;
#define OISL_DEVICE_DATA_LNGTH sizeof ( OISL_Device_Data_tlm_t )
#define OISL_DEVICE_DATA_SIZE OISL_DEVICE_DATA_LNGTH + OISL_DEVICE_HDR_TRL_LEN

// Declare the global variable for the memory information
// extern OISL_Device_Data_tlm_t memoryInfo;


/*
** Prototypes
*/
int32_t OISL_ReadData(uart_info_t* device, uint8_t* read_data, uint8_t data_length);
int32_t OISL_CommandDevice(uart_info_t* device, uint8_t cmd, uint32_t payload);
int32_t OISL_RequestHK(uart_info_t* device, OISL_Device_HK_tlm_t* data);
int32_t OISL_RequestData(uart_info_t* device, OISL_Device_Data_tlm_t* data);
void get_isl_vectors(double* forward_ISL_vector, double* backward_ISL_vector);
void UNITV2(double V[3]);
int read_alignment_info(const char* filename);


#endif /* _OISL_DEVICE_H_ */
