/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fci_hal.c

 Description : fc8110 host interface

 History :
 ----------------------------------------------------------------------
 2009/08/29 	jason		initial
*******************************************************************************/

#include "fci_types.h"
#include "bbm.h"
#include "fci_hal.h"
#include "fci_tun.h"
#include "fc8101_hpi.h"
#include "fc8101_spi.h"
#include "fc8101_ppi.h"

typedef struct {
	int (*init)(HANDLE hDevice, u16 param1, u16 param2);

	int (*byteread)(HANDLE hDevice, u16 addr, u8  *data);
	int (*wordread)(HANDLE hDevice, u16 addr, u16 *data);
	int (*longread)(HANDLE hDevice, u16 addr, u32 *data);
	int (*bulkread)(HANDLE hDevice, u16 addr, u8  *data, u16 size);

	int (*bytewrite)(HANDLE hDevice, u16 addr, u8  data);
	int (*wordwrite)(HANDLE hDevice, u16 addr, u16 data);
	int (*longwrite)(HANDLE hDevice, u16 addr, u32 data);
	int (*bulkwrite)(HANDLE hDevice, u16 addr, u8* data, u16 size);

	int (*dataread)(HANDLE hDevice, u16 addr, u8* data, u16 size);

	int (*deinit)(HANDLE hDevice);
} IF_PORT;

static IF_PORT hpiif = {
	&fc8101_hpi_init,

	&fc8101_hpi_byteread,
	&fc8101_hpi_wordread,
	&fc8101_hpi_longread,
	&fc8101_hpi_bulkread,

	&fc8101_hpi_bytewrite,
	&fc8101_hpi_wordwrite,
	&fc8101_hpi_longwrite,
	&fc8101_hpi_bulkwrite,

	&fc8101_hpi_dataread,

	&fc8101_hpi_deinit
};

static IF_PORT spiif = {
	&fc8101_spi_init,

	&fc8101_spi_byteread,
	&fc8101_spi_wordread,
	&fc8101_spi_longread,
	&fc8101_spi_bulkread,

	&fc8101_spi_bytewrite,
	&fc8101_spi_wordwrite,
	&fc8101_spi_longwrite,
	&fc8101_spi_bulkwrite,

	&fc8101_spi_dataread,

	&fc8101_spi_deinit
};

static IF_PORT ppiif = {
	&fc8101_ppi_init,

	&fc8101_ppi_byteread,
	&fc8101_ppi_wordread,
	&fc8101_ppi_longread,
	&fc8101_ppi_bulkread,

	&fc8101_ppi_bytewrite,
	&fc8101_ppi_wordwrite,
	&fc8101_ppi_longwrite,
	&fc8101_ppi_bulkwrite,

	&fc8101_ppi_dataread,

	&fc8101_ppi_deinit
};

static IF_PORT *ifport = &hpiif;
static u8 hostif_type = BBM_HPI;

int bbm_hostif_get(HANDLE hDevice, u8 *hostif)
{
	*hostif = hostif_type;

	return BBM_OK;
}

int bbm_hostif_select(HANDLE hDevice, u8 hostif)
{
	hostif_type = hostif;

	switch(hostif) {
		case BBM_HPI:
			ifport = &hpiif;
			break;
		case BBM_SPI:
			ifport = &spiif;
			break;
		case BBM_PPI:
			ifport = &ppiif;
			break;
		default:
			return BBM_NOK;
	}

	if(ifport->init(hDevice, 0, 0))
		return BBM_NOK;

	return BBM_OK;
}

int bbm_hostif_deselect(HANDLE hDevice)
{
	if(ifport->deinit(hDevice))
		return BBM_NOK;

	ifport = NULL;
	hostif_type = BBM_HPI;

	return BBM_OK;
}

int bbm_ext_read(HANDLE hDevice, u16 addr, u8 *data)
{
	if(ifport->byteread(hDevice, addr, data))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_byte_read(HANDLE hDevice, u16 addr, u8 *data)
{
	if(ifport->byteread(hDevice, addr, data))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_word_read(HANDLE hDevice, u16 addr, u16 *data)
{
	if(ifport->wordread(hDevice, addr, data))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_long_read(HANDLE hDevice, u16 addr, u32 *data)
{
	if(ifport->longread(hDevice, addr, data))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_bulk_read(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	if(ifport->bulkread(hDevice, addr, data, length))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_write(HANDLE hDevice, u16 addr, u8 data)
{
	if(ifport->bytewrite(hDevice, addr, data))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_byte_write(HANDLE hDevice, u16 addr, u8 data)
{
	if(ifport->bytewrite(hDevice, addr, data))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_word_write(HANDLE hDevice, u16 addr, u16 data)
{
	if(ifport->wordwrite(hDevice, addr, data))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_long_write(HANDLE hDevice, u16 addr, u32 data)
{
	if(ifport->longwrite(hDevice, addr, data))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_bulk_write(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	if(ifport->bulkwrite(hDevice, addr, data, length))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_ext_data(HANDLE hDevice, u16 addr, u8* data, u16 length)
{
	if(ifport->dataread(hDevice, addr, data, length))
		return BBM_NOK;
	return BBM_OK;
}

int bbm_read(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;
	
	res = tuner_i2c_read(hDevice, addr, 1, data, 1);

	return res;
}

int bbm_byte_read(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;
	
	res = tuner_i2c_read(hDevice, addr, 1, data, 1);

	return res;
}

int bbm_word_read(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;
	
	res = tuner_i2c_read(hDevice, addr, 1, (u8*)data, 2);

	return res;
}

int bbm_long_read(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;
	
	res = tuner_i2c_read(hDevice, addr, 1, (u8*)data, 4);

	return res;
}

int bbm_bulk_read(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	
	res = tuner_i2c_read(hDevice, addr, 1, data, length);

	return res;
}

int bbm_write(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res = tuner_i2c_write(hDevice, addr, 1, &data, 1);

	return res;
}

int bbm_byte_write(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res = tuner_i2c_write(hDevice, addr, 1, &data, 1);

	return res;
}

int bbm_word_write(HANDLE hDevice, u16 addr, u16 data)
{
	int res;

	res = tuner_i2c_write(hDevice, addr, 1, (u8*)&data, 2);

	return res;
}

int bbm_long_write(HANDLE hDevice, u16 addr, u32 data)
{
	int res;

	res = tuner_i2c_write(hDevice, addr, 1, (u8*)&data, 4);

	return res;
}

int bbm_bulk_write(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res;

	res = tuner_i2c_write(hDevice, addr, 1, data, size);

	return res;
}