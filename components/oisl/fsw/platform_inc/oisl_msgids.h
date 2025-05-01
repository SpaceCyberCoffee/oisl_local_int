/************************************************************************
** File:
**   $Id: oisl_msgids.h  $
**
** Purpose:
**  Define OISL Message IDs
**
*************************************************************************/
#ifndef _OISL_MSGIDS_H_
#define _OISL_MSGIDS_H_

/* 
** CCSDS V1 Command Message IDs (MID) must be 0x18xx
*/
#define OISL_CMD_MID              0x1898 /* TODO: Change this for your app */ 

/* 
** This MID is for commands telling the app to publish its telemetry message
*/
#define OISL_REQ_HK_MID           0x1899 /* TODO: Change this for your app */

/* 
** CCSDS V1 Telemetry Message IDs must be 0x08xx
*/
#define OISL_HK_TLM_MID           0x0898 /* TODO: Change this for your app */
#define OISL_DEVICE_TLM_MID       0x0899 /* TODO: Change this for your app */

#endif /* _OISL_MSGIDS_H_ */
