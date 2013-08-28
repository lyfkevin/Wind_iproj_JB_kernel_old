/*****************************************************************************
 Copyright(c) 2010 FCI Inc. All Rights Reserved
 
 File name : fci_tun.h
 
 Description : tuner control driver header
 
 History : 
 ----------------------------------------------------------------------
 2009/08/29 	jason		initial
*******************************************************************************/

#ifndef __FCI_TUN_H__
#define __FCI_TUN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

typedef enum {
	FCI_I2C_TYPE = 0
} i2c_type;

typedef enum {
	BAND3_TYPE = 0,
	LBAND_TYPE,
	ISDBT_1_SEG_TYPE
} band_type;

typedef enum {
	FC8100_TUNER = 0,
	FC8101_TUNER = 1
} product_type;

extern int tuner_i2c_init(HANDLE hDevice, int speed, int slaveaddr);
extern int tuner_i2c_read(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len);
extern int tuner_i2c_write(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len);
extern int tuner_i2c_rf_read(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len);		
extern int tuner_i2c_rf_write(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len);

extern int tuner_select(HANDLE hDevice, u32 product, u32 band);
extern int tuner_deselect(HANDLE hDevice);
extern int tuner_ctrl_select(HANDLE hDevice, i2c_type type);
extern int tuner_ctrl_deselect(HANDLE hDevice);
extern int tuner_set_freq(HANDLE hDevice, u8 ch_num);
extern int tuner_get_rssi(HANDLE hDevice, s32 *rssi);

#ifdef __cplusplus
}
#endif

#endif		// __FCI_TUN_H__
