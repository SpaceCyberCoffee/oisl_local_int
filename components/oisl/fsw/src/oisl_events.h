/************************************************************************
** File:
**    oisl_events.h
**
** Purpose:
**  Define OISL application event IDs
**
*************************************************************************/

#ifndef _OISL_EVENTS_H_
#define _OISL_EVENTS_H_

/* Standard app event IDs */
#define OISL_RESERVED_EID              0
#define OISL_STARTUP_INF_EID           1
#define OISL_LEN_ERR_EID               2
#define OISL_PIPE_ERR_EID              3
#define OISL_SUB_CMD_ERR_EID           4
#define OISL_SUB_REQ_HK_ERR_EID        5
#define OISL_PROCESS_CMD_ERR_EID       6

/* Standard command event IDs */
#define OISL_CMD_ERR_EID               10
#define OISL_CMD_NOOP_INF_EID          11
#define OISL_CMD_RESET_INF_EID         12
#define OISL_CMD_ENABLE_INF_EID        13
#define OISL_ENABLE_INF_EID            14
#define OISL_ENABLE_ERR_EID            15
#define OISL_CMD_DISABLE_INF_EID       16
#define OISL_DISABLE_INF_EID           17
#define OISL_DISABLE_ERR_EID           18

/* Device specific command event IDs */
#define OISL_CMD_CONFIG_INF_EID        20

/* CFDP Transfer File */
#define OISL_CMD_SEND_FILE_EID         21
#define OISL_CMD_SEND_FILE_ERR_EID     22

/* Standard telemetry event IDs */
#define OISL_DEVICE_TLM_ERR_EID        30
#define OISL_REQ_HK_ERR_EID            31

/* Device specific telemetry event IDs */
#define OISL_REQ_DATA_ERR_EID          32

/* Hardware protocol event IDs */
#define OISL_UART_INIT_ERR_EID         40
#define OISL_UART_CLOSE_ERR_EID        41

#endif /* _OISL_EVENTS_H_ */
