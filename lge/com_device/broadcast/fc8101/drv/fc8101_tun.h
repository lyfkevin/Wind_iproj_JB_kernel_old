/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8101_tun.c
 
 Description : API of dmb baseband module
 
 History : 
 ----------------------------------------------------------------------
 2009/09/11 	jason		initial
*******************************************************************************/

#ifndef __FC8101_TUNER__
#define __FC8101_TUNER__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8101_tuner_init(HANDLE hDevice, u8 band_type);
extern int fc8101_tuner_deinit(HANDLE hDevice);
extern int fc8101_set_freq(HANDLE hDevice, u8 ch_num);
extern int fc8101_get_rssi(HANDLE hDevice, s32 *rssi);

#ifdef __cplusplus
}
#endif

#endif // __FC8101_TUNER__

