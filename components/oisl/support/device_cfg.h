#ifndef _OISL_CHECKOUT_DEVICE_CFG_H_
#define _OISL_CHECKOUT_DEVICE_CFG_H_

/*
** OISL Checkout Configuration
*/
#define OISL_CFG
/* Note: NOS3 uart requires matching handle and bus number */
#define OISL_CFG_STRING           "/dev/usart_8"
#define OISL_CFG_HANDLE           8
#define OISL_CFG_BAUDRATE_HZ      115200
#define OISL_CFG_MS_TIMEOUT       250
#define OISL_CFG_DEBUG            

#endif /* _OISL_CHECKOUT_DEVICE_CFG_H_ */
