#ifndef _ISDBT_COMMON_H_
#define _ISDBT_COMMON_H_

#define GPIO_MB86A35S_SPIC_XIRQ 		27	//don't use rev.B
#define GPIO_ISDBT_IRQ 					33	//don't use
//#define GPIO_MB86A35S_FRAME_LOCK		46	//rev.B - not need
// kichul.park for Rev.D
#define GPIO_MB86A35S_SPIS_XIRQ 		141	//rev.B - not need??? rev.C : 62 -> Rev.D :141

#define GPIO_ISDBT_PWR_EN 				102	//1.2V, 1.8V (but 2.8V->PMIC)
#define GPIO_ISDBT_RST 					101
#define GPIO_ISDBT_ANT_SELECT 			46	//rev.D

#define ISDBT_DEFAULT_NOTUSE_MODE -1
#define ISDBT_UHF_MODE 0
#define ISDBT_VHF_MODE 1

#endif //_ISDBT_COMMON_H_