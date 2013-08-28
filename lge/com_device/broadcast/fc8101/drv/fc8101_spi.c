/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8101_spi.c
 
 Description : fc8110 host interface
 
 History : 
 ----------------------------------------------------------------------
 2009/08/29 	jason		initial
*******************************************************************************/
#include <linux/spi/spi.h>
#include <linux/slab.h>

#include "fci_types.h"
#include "fc8101_regs.h"
#include "fci_oal.h"

#define HPIC_READ			0x01	// read command
#define HPIC_WRITE			0x02	// write command
#define HPIC_AINC			0x04	// address increment
#define HPIC_BMODE			0x00	// byte mode
#define HPIC_WMODE          0x10	// word mode
#define HPIC_LMODE          0x20	// long mode
#define HPIC_ENDIAN			0x00	// little endian
#define HPIC_CLEAR			0x80	// currently not used

#define SPI_CMD_WRITE                           0x0
#define SPI_CMD_READ                            0x1
#define SPI_CMD_BURST_WRITE                     0x2
#define SPI_CMD_BURST_READ                      0x3

#define DRIVER_NAME "fc8101_spi"

#define QUP_GSBI_SPI_USE_DMOV // qsd spi dma mode

struct spi_device *fc8101_spi = NULL;

static u8 tx_data[10];

#ifdef QUP_GSBI_SPI_USE_DMOV
#define TX_RX_DATA_BUF_SIZE (8192)

static u8 *wdata_buf;
static u8 *rdata_buf;
static u8 wx_data_buf[TX_RX_DATA_BUF_SIZE+32] ={0,};
static u8 rx_data_buf[TX_RX_DATA_BUF_SIZE+32] = {0,};
#else
static u8 rdata_buf[8192]={0};
static u8 wdata_buf[8192]={0};
#endif

static DEFINE_MUTEX(lock);


static int __devinit fc8101_spi_probe(struct spi_device *spi)
{
	s32 ret;
	
	PRINTF(0, "fc8101_spi_probe\n");

	spi->max_speed_hz =  24000000; // 2011-09-01 taew00k.kang change to 24MHz from 4MHz (4000000)
	spi->bits_per_word = 8;
	spi->mode =  SPI_MODE_0;

	ret = spi_setup(spi);
	
	if (ret < 0)
		return ret;

	fc8101_spi = spi;

	return ret;
}

static int fc8101_spi_remove(struct spi_device *spi)
{
#if 0
	if(data_buf!=NULL)
		kfree(data_buf);
#endif	
	return 0;
}

static struct spi_driver fc8101_spi_driver = {
	.driver = {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
	},
	.probe		= fc8101_spi_probe,
	.remove		= __devexit_p(fc8101_spi_remove),
};

int fc8101_spi_init(HANDLE hDevice, u16 param1, u16 param2)
{
	int res = 0;
	PRINTF(0, "fc8101_spi_init : %d\n", res);
	res = spi_register_driver(&fc8101_spi_driver);
	
	if(res)
	{
		PRINTF(0, "fc8101_spi register fail : %d\n", res);
		return BBM_NOK;
	}
	
#ifdef QUP_GSBI_SPI_USE_DMOV
	wdata_buf = (u8*)(((u32)wx_data_buf +31)&~31);
	rdata_buf = (u8*)(((u32)rx_data_buf +31)&~31);
#endif

	
	return res;
}

static int fc8101_spi_write_then_read(struct spi_device *spi, u8 *txbuf, u16 tx_length, u8 *rxbuf, u16 rx_length)
{
	int res = 0;
	
	struct spi_message	message;
	struct spi_transfer	x;

	spi_message_init(&message);
	memset(&x, 0, sizeof x);

	spi_message_add_tail(&x, &message);
	
	memcpy(&wdata_buf[0], txbuf, tx_length);
	
	x.tx_buf=&wdata_buf[0];
	x.rx_buf=&rdata_buf[0];
	x.len = tx_length + rx_length;
	x.cs_change = 0; // 0 : CS is alternate per transfer, 1 : CS is alternate per 8/16/24/32 bit which is setted in spi_setup
	x.bits_per_word = 8;
	res = spi_sync(spi, &message);

	memcpy(rxbuf, x.rx_buf + tx_length, rx_length);
	
	return res;
}


static int spi_bulkread(HANDLE hDevice, u8 addr, u8 *data, u16 length)
{
	int ret;

	tx_data[0] = SPI_CMD_READ;
	tx_data[1] = addr;

	ret = fc8101_spi_write_then_read(fc8101_spi, &tx_data[0], 2, &data[0], length);

	if(ret)
	{
		PRINTF(0, "fc8101_spi_bulkread fail : %d\n", ret);
		return BBM_NOK;
	}

	return BBM_OK;
}

static int spi_bulkwrite(HANDLE hDevice, u8 addr, u8 *data, u16 length)
{
	s32 ret;
	s32 i;
	
	tx_data[0] = SPI_CMD_WRITE;
	tx_data[1] = addr;

	for(i=0;i<length;i++)
	{
		tx_data[2+i] = data[i];
	}

	ret = fc8101_spi_write_then_read(fc8101_spi, &tx_data[0], length+2, NULL, 0);

	if(ret)
	{
		PRINTF(0, "fc8101_spi_bulkwrite fail : %d\n", ret);
		return BBM_NOK;
	}
	
	return BBM_OK;
}

static int spi_dataread(HANDLE hDevice, u8 addr, u8 *data, u16 length)
{
	s32 ret = 0;

	tx_data[0] = SPI_CMD_BURST_READ;
	tx_data[1] = addr;

	ret = fc8101_spi_write_then_read(fc8101_spi, &tx_data[0], 2, &data[0], length);
	//printk("spi_dataread  (0x%x,0x%x,0x%x,0x%x)\n", data[0], data[1], data[2], data[3]);
	
	if(ret)
	{
		PRINTF(0, "fc8101_spi_dataread fail : %d\n", ret);
		return BBM_NOK;
	}

	return BBM_OK;
}

int fc8101_spi_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	u8 reg_addr = 0xff & addr; 
	
	mutex_lock(&lock);
	res = spi_bulkread(hDevice, reg_addr, data, 1);
	mutex_unlock(&lock);

	return res;	
}


int fc8101_spi_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;

	u8 reg_addr = 0xff & addr; 

	mutex_lock(&lock);
	res = spi_bulkread(hDevice, reg_addr, (u8*)data, 2);
	mutex_unlock(&lock);

	return res;	
}

int fc8101_spi_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;

	u8 reg_addr = 0xff & addr; 

	mutex_lock(&lock);
	res = spi_bulkread(hDevice, reg_addr, (u8*)data, 4);
	mutex_unlock(&lock);

	return res;	
}

int fc8101_spi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;

	u8 reg_addr = 0xff & addr; 

	mutex_lock(&lock);
	res = spi_bulkread(hDevice, reg_addr, data, length);
	mutex_unlock(&lock);

	return res;	
}

int fc8101_spi_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	u8 reg_addr = 0xff & addr; 

	mutex_lock(&lock);
	res = spi_bulkwrite(hDevice, reg_addr, &data, 1);
	mutex_unlock(&lock);

	return res;	
}

int fc8101_spi_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	int res;

	u8 reg_addr = 0xff & addr; 

	mutex_lock(&lock);
	res = spi_bulkwrite(hDevice, reg_addr, (u8*)&data, 2);
	mutex_unlock(&lock);

	return res;	
}

int fc8101_spi_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	int res;

	u8 reg_addr = 0xff & addr; 

	mutex_lock(&lock);
	res = spi_bulkwrite(hDevice, reg_addr, (u8*)&data, 4);
	mutex_unlock(&lock);

	return res;	
}

int fc8101_spi_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;

	u8 reg_addr = 0xff & addr; 

	mutex_lock(&lock);
	res = spi_bulkwrite(hDevice, reg_addr, data, length);
	mutex_unlock(&lock);

	return res;	
}

int fc8101_spi_dataread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;

	u8 reg_addr = 0xff & addr; 

	mutex_lock(&lock);
	res = spi_dataread(hDevice, reg_addr, data, length);
	mutex_unlock(&lock);

	return res;	
}

int fc8101_spi_deinit(HANDLE hDevice)
{

	spi_unregister_driver(&fc8101_spi_driver);

	return BBM_OK;
}
