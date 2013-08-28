/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : bbm.c
 
 Description : API of dmb baseband module
 
 History : 
 ----------------------------------------------------------------------
 2009/08/29 	jason		initial
*******************************************************************************/

#include "fci_types.h"
#include "fci_tun.h"
#include "fc8101_regs.h"
#include "fc8101_bb.h"
#include "fci_hal.h"
#include "fc8101_isr.h"

int BBM_RESET(HANDLE hDevice)
{
	int res;

	res = fc8101_reset(hDevice);

	return res;
}

int BBM_PROBE(HANDLE hDevice)
{
	int res;

	res = fc8101_probe(hDevice);

	return res;
}

int BBM_INIT(HANDLE hDevice)
{
	int res;

	res = fc8101_init(hDevice);

	return res;
}

int BBM_DEINIT(HANDLE hDevice)
{
	int res;

	res = fc8101_deinit(hDevice);

	return res;
}

int BBM_READ(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	res = bbm_read(hDevice, addr, data);

	return res;
}

int BBM_BYTE_READ(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	res = bbm_byte_read(hDevice, addr, data);

	return res;
}

int BBM_WORD_READ(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;

	res = bbm_word_read(hDevice, addr, data);

	return res;
}

int BBM_LONG_READ(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;

	res = bbm_long_read(hDevice, addr, data);

	return BBM_OK;
}

int BBM_BULK_READ(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res;

	res = bbm_bulk_read(hDevice, addr, data, size);

	return res;
}

int BBM_WRITE(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res = bbm_write(hDevice, addr, data);

	return res;
}

int BBM_BYTE_WRITE(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res = bbm_byte_write(hDevice, addr, data);

	return res;
}

int BBM_WORD_WRITE(HANDLE hDevice, u16 addr, u16 data)
{
	int res;

	res = bbm_word_write(hDevice, addr, data);

	return res;
}

int BBM_LONG_WRITE(HANDLE hDevice, u16 addr, u32 data)
{
	int res;

	res = bbm_long_write(hDevice, addr, data);

	return res;
}

int BBM_BULK_WRITE(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res;

	res = bbm_bulk_write(hDevice, addr, data, size);

	return res;
}

int BBM_I2C_INIT(HANDLE hDevice, u32 type)
{
	int res;
	
	res = tuner_ctrl_select(hDevice, (i2c_type) type);
	
	return res;
}

int BBM_I2C_DEINIT(HANDLE hDevice)
{
	int res;
	
	res = tuner_ctrl_deselect(hDevice);
	
	return res;
}

int BBM_TUNER_READ(HANDLE hDevice, u8 addr, u8 alen, u8 *buffer, u8 len)
{
	int res;

	res = tuner_i2c_rf_read(hDevice, addr, alen, buffer, len);

	return res;
}

int BBM_TUNER_WRITE(HANDLE hDevice, u8 addr, u8 alen, u8 *buffer, u8 len)
{
	int res;

	res = tuner_i2c_rf_write(hDevice, addr, alen, buffer, len);

	return res;
}

int BBM_TUNER_SET_FREQ(HANDLE hDevice, u32 ch_num)
{
	int res = BBM_OK;

	res = tuner_set_freq(hDevice, ch_num);

	return res;
}

int BBM_TUNER_SELECT(HANDLE hDevice, u32 product, u32 band)
{
	int res = BBM_OK;

	res = tuner_select(hDevice, product, band);

	return res;
}

int BBM_TUNER_DESELECT(HANDLE hDevice)
{
	int res = BBM_OK;

	res = tuner_deselect(hDevice);

	return res;
}

int BBM_TUNER_GET_RSSI(HANDLE hDevice, s32 *rssi)
{
	int res = BBM_OK;

	res = tuner_get_rssi(hDevice, rssi);

	return res;
}

int BBM_SCAN_STATUS(HANDLE hDevice)
{
	int res = BBM_OK;

	res = fc8101_scan_status(hDevice);

	return res;
}

void BBM_ISR(HANDLE hDevice)
{
	fc8101_isr(hDevice);
}

int BBM_HOSTIF_SELECT(HANDLE hDevice, u8 hostif)
{
	int res = BBM_NOK;

	res = bbm_hostif_select(hDevice, hostif);

	return res;
}		

int BBM_HOSTIF_DESELECT(HANDLE hDevice)
{
	int res = BBM_NOK;

	res = bbm_hostif_deselect(hDevice);

	return res;
}		

int BBM_CALLBACK_REGISTER(u32 userdata, int (*callback)(u32 userdata, u8 *data, int length))
{
	gTSUserData = userdata;
	pTSCallback = callback;

	return BBM_OK;
}

int BBM_CALLBACK_DEREGISTER(HANDLE hDevice)
{
	gTSUserData = 0;
	pTSCallback = NULL;

	return BBM_OK;
}

