/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fci_tun.c

 Description : tuner driver

 History :
 ----------------------------------------------------------------------
 2009/08/29 	jason		initial
*******************************************************************************/

#include "fci_types.h"
#include "fci_oal.h"
#include "fci_hal.h"
#include "fci_tun.h"
#include "fci_i2c.h"
#include "fc8101_regs.h"
#include "fc8101_bb.h"
#include "fc8101_tun.h"

#define FC8101_TUNER_ADDR	0x77 // Slave address

u8 tuner_addr = FC8101_TUNER_ADDR;
static band_type tuner_band = ISDBT_1_SEG_TYPE;

typedef struct {
	int		(*init)(HANDLE hDevice, int speed, int slaveaddr);
	int		(*read)(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len);
	int		(*write)(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len);
	int		(*rfread)(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len);		
	int		(*rfwrite)(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len);
	int		(*deinit)(HANDLE hDevice);
} I2C_DRV;

static I2C_DRV fcii2c = {
	&fci_i2c_init,
	&fci_i2c_read,
	&fci_i2c_write,
	&fci_i2c_rf_read,		
	&fci_i2c_rf_write,
	&fci_i2c_deinit
};

typedef struct {
	int		(*init)(HANDLE hDevice, u8 band);
	int		(*set_freq)(HANDLE hDevice, u8 ch_num);
	int		(*get_rssi)(HANDLE hDevice, int *rssi);
	int		(*deinit)(HANDLE hDevice);
} TUNER_DRV;

static TUNER_DRV fc8101_tuner = {
	&fc8101_tuner_init,
	&fc8101_set_freq,
	&fc8101_get_rssi,
	&fc8101_tuner_deinit
};

static I2C_DRV* tuner_ctrl = &fcii2c;
static TUNER_DRV* tuner = &fc8101_tuner;

int tuner_ctrl_select(HANDLE hDevice, i2c_type type)
{
	int res = BBM_OK;

	switch (type) {
		case FCI_I2C_TYPE:
			tuner_ctrl = &fcii2c;
			tuner_addr = FC8101_TUNER_ADDR;
			break;
 		default:
			res = BBM_NOK;
			break;
	}

	if(res) 
		return BBM_NOK;
	
	res = tuner_ctrl->init(hDevice, 400, 0);

	return res;
}

int tuner_ctrl_deselect(HANDLE hDevice)
{
	return BBM_OK;
}
	
int tuner_i2c_read(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len)
{
	if(tuner_ctrl->read(hDevice, tuner_addr, addr, alen, data, len))
		return BBM_NOK;
	return BBM_OK;
}

int tuner_i2c_write(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len)
{
	if(tuner_ctrl->write(hDevice, tuner_addr, addr, alen, data, len))
		return BBM_NOK;
	return BBM_OK;
}

int tuner_i2c_rf_read(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len)
{
	if(tuner_ctrl->rfread(hDevice, tuner_addr, addr, alen, data, len))
		return BBM_NOK;
	return BBM_OK;
}

int tuner_i2c_rf_write(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len)
{
	if(tuner_ctrl->rfwrite(hDevice, tuner_addr, addr, alen, data, len))
		return BBM_NOK;
	return BBM_OK;
}

int tuner_type(HANDLE hDevice, u32 *type)
{
	*type = tuner_band;

	return BBM_OK;
}

int tuner_set_freq(HANDLE hDevice, u8 ch_num)
{
	int res = BBM_NOK;

	if(tuner == NULL) {
		PRINTF(NULL, "TUNER NULL ERROR \r\n");
		return BBM_NOK;
	}

	res = tuner->set_freq(hDevice, ch_num);
	if(res != BBM_OK) {
		PRINTF(NULL, "TUNER res ERROR \r\n");
		return BBM_NOK;
	}

	fc8101_reset(hDevice);

	return res;
}

int tuner_select(HANDLE hDevice, u32 product, u32 band)
{
	switch(product) {
		case FC8101_TUNER:
			tuner = &fc8101_tuner;
			tuner_band = (band_type) band;
			tuner_addr = FC8101_TUNER_ADDR;
			break;
		default:
			return BBM_NOK;
	}

	if(tuner == NULL) {
		PRINTF(hDevice, "[ERROR] Can not supported Tuner(%d,%d)\n", product, band);
		return BBM_NOK;
	}

	if(tuner->init(hDevice, tuner_band))
		return BBM_NOK;
	return BBM_OK;
}

int tuner_deselect(HANDLE hDevice)
{
	int res = BBM_OK;

	return res;
}

int tuner_get_rssi(HANDLE hDevice, s32 *rssi)
{
	if(tuner->get_rssi(hDevice, rssi))
		return BBM_NOK;
	return BBM_OK;
}

