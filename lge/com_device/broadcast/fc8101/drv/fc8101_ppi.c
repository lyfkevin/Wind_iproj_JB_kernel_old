/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8101_ppi.c
 
 Description : fc8101 host interface
 
 History : 
 ----------------------------------------------------------------------
 2009/09/14 	jason		initial
*******************************************************************************/

#include "fci_types.h"
#include "fc8101_regs.h"


#define BBM_BASE_ADDR				0
#define BBM_BASE_OFFSET 			0x00

#define PPI_BMODE                       0x00
#define PPI_WMODE                       0x10
#define PPI_LMODE                       0x20
#define PPI_READ                        0x40
#define PPI_WRITE                       0x00
#define PPI_AINC                        0x80

#define FC8101_PPI_REG			(*(volatile u8 *)(BBM_BASE_ADDR + (BBM_COMMAND_REG << BBM_BASE_OFFSET)))

int fc8101_ppi_init(HANDLE hDevice, u16 param1, u16 param2)
{
	
	return BBM_OK;
}

int fc8101_ppi_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	u32 length = 1;

	FC8101_PPI_REG = addr & 0xff;

	FC8101_PPI_REG = PPI_READ | ((length & 0x0f0000) >> 16);
	FC8101_PPI_REG = (length & 0x00ff00) >> 8;
	FC8101_PPI_REG = (length & 0x0000ff);

	*data = FC8101_PPI_REG;
	return BBM_OK;
}

int fc8101_ppi_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	u32 length = 2;
	u8 command = PPI_AINC | PPI_READ | PPI_BMODE;

	FC8101_PPI_REG = addr & 0xff;

	FC8101_PPI_REG = command | ((length & 0x0f0000) >> 16);
	FC8101_PPI_REG = (length & 0x00ff00) >> 8;
	FC8101_PPI_REG = (length & 0x0000ff);

	*data = FC8101_PPI_REG;
	*data |= FC8101_PPI_REG << 8;
	return BBM_OK;
}

int fc8101_ppi_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	u32 length = 4;

	FC8101_PPI_REG = addr & 0xff;

	FC8101_PPI_REG = PPI_AINC | PPI_READ | ((length & 0x0f0000) >> 16);
	FC8101_PPI_REG = (length & 0x00ff00) >> 8;
	FC8101_PPI_REG = (length & 0x0000ff);

	*data = FC8101_PPI_REG;
	*data |= FC8101_PPI_REG << 8;
	*data |= FC8101_PPI_REG << 16;
	*data |= FC8101_PPI_REG << 24;
	return BBM_OK;
}

int fc8101_ppi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	u16 i;
	
	FC8101_PPI_REG = addr & 0xff;
	
	FC8101_PPI_REG = PPI_AINC | PPI_READ | ((length & 0x0f0000) >> 16);
	FC8101_PPI_REG = (length & 0x00ff00) >> 8;
	FC8101_PPI_REG = (length & 0x0000ff);

	for(i=0; i<length; i++) {
		data[i] = FC8101_PPI_REG;
	}
	return BBM_OK;
}

int fc8101_ppi_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	u32 length = 1;

	FC8101_PPI_REG = addr & 0xff;

	FC8101_PPI_REG = PPI_AINC | PPI_WRITE | ((length & 0x0f0000) >> 16);
	FC8101_PPI_REG = (length & 0x00ff00) >> 8;
	FC8101_PPI_REG = (length & 0x0000ff);

	FC8101_PPI_REG = data;
	return BBM_OK;
}

int fc8101_ppi_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	u32 length = 2;
	u8 command = PPI_AINC | PPI_WRITE | PPI_BMODE;

	FC8101_PPI_REG = addr & 0xff;
	
	FC8101_PPI_REG = command | ((length & 0x0f0000) >> 16);
	FC8101_PPI_REG = (length & 0x00ff00) >> 8;
	FC8101_PPI_REG = (length & 0x0000ff);
	
	FC8101_PPI_REG = data & 0xff;
	FC8101_PPI_REG = (data & 0xff00) >> 8;
	return BBM_OK;
}

int fc8101_ppi_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	u32 length = 4;

	FC8101_PPI_REG = addr & 0xff;

	FC8101_PPI_REG = PPI_AINC | PPI_WRITE | ((length & 0x0f0000) >> 16);
	FC8101_PPI_REG = (length & 0x00ff00) >> 8;
	FC8101_PPI_REG = (length & 0x0000ff);

	FC8101_PPI_REG = data &  0x000000ff;
	FC8101_PPI_REG = (data & 0x0000ff00) >> 8;
	FC8101_PPI_REG = (data & 0x00ff0000) >> 16;
	FC8101_PPI_REG = (data & 0xff000000) >> 24;
	return BBM_OK;
}

int fc8101_ppi_bulkwrite(HANDLE hDevice, u16 addr, u8* data, u16 length)
{
	u16 i;

	FC8101_PPI_REG = addr & 0xff;
	
	FC8101_PPI_REG = PPI_AINC | PPI_WRITE | ((length & 0x0f0000) >> 16);
	FC8101_PPI_REG = (length & 0x00ff00) >> 8;
	FC8101_PPI_REG = (length & 0x0000ff);

	for(i=0; i<length; i++) {
		FC8101_PPI_REG = data[i];
	}
	return BBM_OK;
}

int fc8101_ppi_dataread(HANDLE hDevice, u16 addr, u8* data, u16 length)
{
	u16 i;

	FC8101_PPI_REG = addr & 0xff;
	
	FC8101_PPI_REG = PPI_READ | ((length & 0x0f0000) >> 16);
	FC8101_PPI_REG = (length & 0x00ff00) >> 8;
	FC8101_PPI_REG = (length & 0x0000ff);

	for(i=0; i<length; i++) {
		data[i] = FC8101_PPI_REG;
	}

	return BBM_OK;
}

int fc8101_ppi_deinit(HANDLE hDevice)
{
	
	return BBM_OK;
}
