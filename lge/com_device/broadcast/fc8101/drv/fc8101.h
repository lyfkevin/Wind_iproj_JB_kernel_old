/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : bbm.c
 
 Description : API of dmb baseband module
 
 History : 
 ----------------------------------------------------------------------
 2009/08/29 	jason		initial
*******************************************************************************/

#ifndef __ISDBT_H__
#define __ISDBT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/list.h>

#include "fci_types.h"
#include "fci_ringbuffer.h"

#define CTL_TYPE 0
#define TS_TYPE 1

#define ISDBT_IOC_MAGIC	'I'
#define IOCTL_MAXNR		18

typedef struct
{
	unsigned short 	lock;	/*baseband lock state 					(1: Lock, 0:Unlock)*/
	unsigned short	cn;		/*Signal Level (C/N) 					(0 ~ 28)*/
	unsigned int		ber; 	/*Bit Error rate 						(0 ~ 100000)*/
	unsigned int		per;  	/*Packet Error rate  					(0 ~ 100000)*/
	unsigned short	agc;  	/*Auto Gain control  					(0 ~ 255)*/
	int 				rssi;  	/*Received Signal Strength Indication  	(0 ~ -99)*/
	unsigned short	ErrTSP;
	unsigned short	TotalTSP;
	unsigned int		Num;
	unsigned int		Exp;
	unsigned int		mode;
} IOCTL_ISDBT_SIGNAL_INFO;

#define IOCTL_ISDBT_POWER_ON			_IO(ISDBT_IOC_MAGIC,10)
#define IOCTL_ISDBT_POWER_OFF			_IO(ISDBT_IOC_MAGIC, 11)
#define IOCTL_ISDBT_SCAN_FREQ			_IOW(ISDBT_IOC_MAGIC,12, unsigned int)
#define IOCTL_ISDBT_SET_FREQ			_IOW(ISDBT_IOC_MAGIC,13, unsigned int)
#define IOCTL_ISDBT_GET_LOCK_STATUS    	_IOR(ISDBT_IOC_MAGIC,14, unsigned int)
#define IOCTL_ISDBT_GET_SIGNAL_INFO		_IOR(ISDBT_IOC_MAGIC,15, IOCTL_ISDBT_SIGNAL_INFO)
#define IOCTL_ISDBT_START_TS			_IO(ISDBT_IOC_MAGIC,16)
#define IOCTL_ISDBT_STOP_TS				_IO(ISDBT_IOC_MAGIC,17)

typedef struct {
	HANDLE				*hInit;
	struct list_head		hList;
	struct fci_ringbuffer		RingBuffer;
       	u8				*buf;
	u8				isdbttype;
} ISDBT_OPEN_INFO_T;

typedef struct {
	struct list_head		hHead;		
} ISDBT_INIT_INFO_T;

#ifdef __cplusplus
}
#endif

#endif // __ISDBT_H__

