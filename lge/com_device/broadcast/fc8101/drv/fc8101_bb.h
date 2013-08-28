/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8101_bb.h
 
 Description : baseband header file
 
 History : 
 ----------------------------------------------------------------------
 2009/09/14 	jason		initial
*******************************************************************************/

#ifndef __FC8101_BB_H__
#define __FC8101_BB_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int  fc8101_reset(HANDLE hDevice);
extern int  fc8101_probe(HANDLE hDevice);
extern int  fc8101_init(HANDLE hDevice);
extern int  fc8101_deinit(HANDLE hDevice);

extern int  fc8101_scan_status(HANDLE hDevice);

extern int  fc8101_channel_select(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int  fc8101_video_select(HANDLE hDevice, u8 subChId, u8 svcChId, u8 cdiId);
extern int  fc8101_audio_select(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int  fc8101_data_select(HANDLE hDevice, u8 subChId, u8 svcChId);

extern int  fc8101_channel_deselect(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int  fc8101_video_deselect(HANDLE hDevice, u8 subChId, u8 svcChId, u8 cdiId);
extern int  fc8101_audio_deselect(HANDLE hDevice, u8 subChId, u8 svcChId);
extern int  fc8101_data_deselect(HANDLE hDevice, u8 subChId, u8 svcChId);

#ifdef __cplusplus
}
#endif

#endif 		// __FC8101_BB_H__
