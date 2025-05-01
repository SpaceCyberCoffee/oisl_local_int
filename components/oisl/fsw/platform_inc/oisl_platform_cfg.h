/************************************************************************
** File:
**   $Id: oisl_platform_cfg.h  $
**
** Purpose:
**  Define oisl Platform Configuration Parameters
**
** Notes:
**
*************************************************************************/
#ifndef _OISL_PLATFORM_CFG_H_
#define _OISL_PLATFORM_CFG_H_

/*
** Default OISL Configuration
*/
#ifndef OISL_CFG
    /* Notes: 
    **   NOS3 uart requires matching handle and bus number
    */
    #define OISL_CFG_STRING           "usart_8"
    #define OISL_CFG_HANDLE           8
    #define OISL_CFG_BAUDRATE_HZ      115200
    #define OISL_CFG_MS_TIMEOUT       50            /* Max 255 */
    /* Note: Debug flag disabled (commented out) by default */
    //#define OISL_CFG_DEBUG
#endif

#endif /* _OISL_PLATFORM_CFG_H_ */
