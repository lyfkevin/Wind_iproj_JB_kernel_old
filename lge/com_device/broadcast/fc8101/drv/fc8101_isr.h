/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8101_isr.c
 
 Description : API of dmb baseband module
 
 History : 
 ----------------------------------------------------------------------
 2009/09/11 	jason		initial
*******************************************************************************/

#ifndef __FC8101_ISR__
#define __FC8101_ISR__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

extern u32 gTSUserData;

extern int (*pTSCallback)(u32 userdata, u8 *data, int length);

extern void fc8101_isr(HANDLE hDevice);

extern void fc8101_buffer_init(u8 alloc);

extern void fc8101_check_overflow(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif // __FC8101_ISR__

