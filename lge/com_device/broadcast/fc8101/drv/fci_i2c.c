/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fci_i2c.c
 
 Description : fci i2c driver
 
 History : 
 ----------------------------------------------------------------------
 2009/09/11 	jason		initial
*******************************************************************************/

#include <linux/delay.h>

#include "fci_types.h"
#include "fci_oal.h"
#include "fc8101_regs.h"
#include "fci_hal.h"
//#define FEATURE_FCI_I2C_CHECK_STATUS
#define FEATURE_INTERFACE_TEST

#define I2CSTAT_TIP 		0x02	/* Tip bit */
#define I2CSTAT_NACK		0x80	/* Nack bit */

#define I2C_TIMEOUT 		1	/* 1 second */

#define I2C_CR_STA			0x80
#define I2C_CR_STO			0x40
#define I2C_CR_RD			0x20
#define I2C_CR_WR			0x10
#define I2C_CR_NACK 		0x08
#define I2C_CR_IACK 		0x01

#define I2C_WRITE			0
#define I2C_READ			1

#define I2C_BB_WRITE		0
#define I2C_BB_READ			1
#define I2C_RF_WRITE		2
#define I2C_RF_READ			3

#define I2C_OK				0
#define I2C_NOK 			1
#define I2C_NACK			2
#define I2C_NOK_LA			3		/* Lost arbitration */
#define I2C_NOK_TOUT		4		/* time out */

static int WaitForXfer (HANDLE hDevice)
{
	int res = I2C_OK;

	udelay(30);
	
#ifdef FEATURE_FCI_I2C_CHECK_STATUS
	int i;
	u8 status;

	i = I2C_TIMEOUT * 10000;

	do {
		 bbm_ext_read(hDevice, BBM_I2C_SR, &status);
		 i--;
	} while ((i > 0) && (status & I2CSTAT_TIP));

	if(status & I2CSTAT_TIP) {
		res = I2C_NOK_TOUT;
	} 
	else
	{										
		bbm_ext_read(hDevice, BBM_I2C_SR, &status);			
		if(status & I2CSTAT_NACK) res = I2C_NACK;
		else res = I2C_OK;
	}
#endif
	return res;
	
}

static int fci_i2c_transfer (HANDLE hDevice, u8 cmd_type, u8 chip, u8 addr[], u8 addr_len, u8 data[], u8 data_len)
{
	int i;	
	int result = I2C_OK;
	u16 cmd;
	switch (cmd_type) {

		case I2C_BB_WRITE:
#ifdef FEATURE_INTERFACE_TEST
			cmd = (I2C_CR_STA | I2C_CR_WR);
			cmd = (cmd<<8) | (chip | I2C_WRITE);
			bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
			bbm_ext_write(hDevice, BBM_I2C_TXR, chip | I2C_WRITE);
			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR /*0x90*/);
#endif
			result = WaitForXfer(hDevice);
			if(result != I2C_OK) return result;		

			if (addr && addr_len) {
				i = 0;
				while ((i < addr_len) && (result == I2C_OK)) {
#ifdef FEATURE_INTERFACE_TEST
					cmd = (I2C_CR_WR);
					cmd = (cmd<<8) | (addr[i]);
					bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
					bbm_ext_write(hDevice, BBM_I2C_TXR, addr[i]);
					bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
					result = WaitForXfer(hDevice);
					if(result != I2C_OK) return result;	

					i++;
				}
			}
			
			i = 0;
			while ((i < data_len) && (result == I2C_OK)) {
#ifdef FEATURE_INTERFACE_TEST
				cmd = (I2C_CR_WR);
				cmd = (cmd<<8) | (data[i]);
				bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
				bbm_ext_write(hDevice, BBM_I2C_TXR, data[i]);
				bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
				result = WaitForXfer(hDevice);
				if(result != I2C_OK) return result;		

				i++;
			}

			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STO /*0x40*/);

			result = WaitForXfer(hDevice);
			if(result != I2C_OK) return result;		

			break;

		case I2C_BB_READ:
			if (addr && addr_len) {
#ifdef FEATURE_INTERFACE_TEST
				cmd = (I2C_CR_STA | I2C_CR_WR);
				cmd = (cmd<<8) | (chip | I2C_WRITE);
				bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
				bbm_ext_write(hDevice, BBM_I2C_TXR, chip | I2C_WRITE);
				bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR /*0x90*/); // send start
#endif
				result = WaitForXfer(hDevice);
				if(result != I2C_OK) {			
					return result;
				}

				i = 0;
				while ((i < addr_len) && (result == I2C_OK)) {
#ifdef FEATURE_INTERFACE_TEST
					cmd = (I2C_CR_WR);
					cmd = (cmd<<8) | (addr[i]);
					bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
					bbm_ext_write(hDevice, BBM_I2C_TXR, addr[i]);
					bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
					result = WaitForXfer(hDevice);
					if(result != I2C_OK) {		
						return result;
					}

					i++;
				}
			}
			
#ifdef FEATURE_INTERFACE_TEST
			cmd = (I2C_CR_STA | I2C_CR_WR);
			cmd = (cmd<<8) | (chip | I2C_READ);
			bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else

			bbm_ext_write(hDevice, BBM_I2C_TXR, chip | I2C_READ);
			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR /*0x90*/); // resend start
#endif
			result = WaitForXfer(hDevice);
			if(result != I2C_OK) {
				return result;
			}	

			i = 0;
			while ((i < data_len) && (result == I2C_OK)) {
				if (i == data_len - 1) {
					bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_RD|I2C_CR_NACK/*0x28*/);	// No Ack Read
					result = WaitForXfer(hDevice);
					if((result != I2C_NACK) && (result != I2C_OK)){
						PRINTF(hDevice, "NACK4-0[%02x]\n", result);
						return result;
					}
				} else {
					bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_RD /*0x20*/);	// Ack Read
					result = WaitForXfer(hDevice);
					if(result != I2C_OK){
						PRINTF(hDevice, "NACK4-1[%02x]\n", result);
						return result;
					}
				}
				bbm_ext_read(hDevice, BBM_I2C_RXR, &data[i]);
				i++;
			}	

			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STO /*0x40*/);		// send stop

			result = WaitForXfer(hDevice);
			if((result != I2C_NACK) && (result != I2C_OK)) {
				PRINTF(hDevice, "NACK5[%02X]\n", result);
				return result;
			}

			break;

		case I2C_RF_WRITE:
#ifdef FEATURE_INTERFACE_TEST
			cmd = (I2C_CR_STA | I2C_CR_WR);
			cmd = (cmd<<8) | (chip | I2C_WRITE);
			bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
			bbm_ext_write(hDevice, BBM_I2C_TXR, chip | I2C_WRITE);
			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR /*0x90*/);
#endif
			result = WaitForXfer(hDevice);
			if(result != I2C_OK) return result;
#ifdef FEATURE_INTERFACE_TEST
			cmd = (I2C_CR_WR);
			cmd = (cmd<<8) | (0x02);
			bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else

			bbm_ext_write(hDevice, BBM_I2C_TXR, 0x02);
			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
			result = WaitForXfer(hDevice);
			if(result != I2C_OK) return result;

#ifdef FEATURE_INTERFACE_TEST
			cmd = (I2C_CR_WR);
			cmd = (cmd<<8) | (0x01);
			bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else

			bbm_ext_write(hDevice, BBM_I2C_TXR, 0x01);
			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
			result = WaitForXfer(hDevice);
			if(result != I2C_OK) return result;

#ifdef FEATURE_INTERFACE_TEST
			cmd = (I2C_CR_STA | I2C_CR_WR);
			cmd = (cmd<<8) | (chip | I2C_WRITE);
			bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else

			bbm_ext_write(hDevice, BBM_I2C_TXR, chip | I2C_WRITE);
			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR /*0x90*/);
#endif
			result = WaitForXfer(hDevice);
			if(result != I2C_OK) return result;

			
			if (addr && addr_len) {
				i = 0;
				while ((i < addr_len) && (result == I2C_OK)) {
#ifdef FEATURE_INTERFACE_TEST
					cmd = (I2C_CR_WR);
					cmd = (cmd<<8) | (addr[i]);
					bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
					bbm_ext_write(hDevice, BBM_I2C_TXR, addr[i]);
					bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
					result = WaitForXfer(hDevice);
					if(result != I2C_OK) return result;		

					i++;
				}
			}
			
			i = 0;
			while ((i < data_len) && (result == I2C_OK)) {
#ifdef FEATURE_INTERFACE_TEST
				cmd = (I2C_CR_WR);
				cmd = (cmd<<8) | (data[i]);
				bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
				bbm_ext_write(hDevice, BBM_I2C_TXR, data[i]);
				bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
				result = WaitForXfer(hDevice);
				if(result != I2C_OK) return result;			

				i++;
			}

			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STO /*0x40*/);

			result = WaitForXfer(hDevice);
			if(result != I2C_OK) return result;		

			break;

		case I2C_RF_READ:
			if (addr && addr_len) {
#ifdef FEATURE_INTERFACE_TEST
				cmd = (I2C_CR_STA | I2C_CR_WR);
				cmd = (cmd<<8) | (chip | I2C_WRITE);
				bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
				bbm_ext_write(hDevice, BBM_I2C_TXR, chip | I2C_WRITE);
				bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR /*0x90*/); // send start
#endif
				result = WaitForXfer(hDevice);
				if(result != I2C_OK) {			
					return result;
				}

#ifdef FEATURE_INTERFACE_TEST
				cmd = (I2C_CR_WR);
				cmd = (cmd<<8) | ( 0x02);
				bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else

				bbm_ext_write(hDevice, BBM_I2C_TXR, 0x02);
				bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
				result = WaitForXfer(hDevice);
				if(result != I2C_OK) return result;

#ifdef FEATURE_INTERFACE_TEST
				cmd = (I2C_CR_WR);
				cmd = (cmd<<8) | (0x01);
				bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else

				bbm_ext_write(hDevice, BBM_I2C_TXR, 0x01);
				bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
				result = WaitForXfer(hDevice);
				if(result != I2C_OK) return result;

#ifdef FEATURE_INTERFACE_TEST
				cmd = (I2C_CR_STA | I2C_CR_WR);
				cmd = (cmd<<8) | (chip | I2C_WRITE);
				bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else

				bbm_ext_write(hDevice, BBM_I2C_TXR, chip | I2C_WRITE);
				bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR /*0x90*/);
#endif
				result = WaitForXfer(hDevice);
				if(result != I2C_OK) return result;

			
				i = 0;
				while ((i < addr_len) && (result == I2C_OK)) {
#ifdef FEATURE_INTERFACE_TEST
					cmd = (I2C_CR_WR);
					cmd = (cmd<<8) | (addr[i]);
					bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else
					bbm_ext_write(hDevice, BBM_I2C_TXR, addr[i]);
					bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
#endif
					result = WaitForXfer(hDevice);
					if(result != I2C_OK) {		
						return result;
					}

					i++;
				}
			}
#ifdef FEATURE_INTERFACE_TEST
			cmd = (I2C_CR_STA | I2C_CR_WR);
			cmd = (cmd<<8) | (chip | I2C_READ);
			bbm_ext_word_write(hDevice, BBM_I2C_TXR, cmd);
#else

			bbm_ext_write(hDevice, BBM_I2C_TXR, chip | I2C_READ);
			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR /*0x90*/); // resend start
#endif
			result = WaitForXfer(hDevice);
			if(result != I2C_OK) {
				return result;
			}	

			i = 0;
			while ((i < data_len) && (result == I2C_OK)) {
				if (i == data_len - 1) {
					bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_RD|I2C_CR_NACK/*0x28*/);	// No Ack Read
					result = WaitForXfer(hDevice);
					if((result != I2C_NACK) && (result != I2C_OK)){
						PRINTF(hDevice, "NACK4-0[%02x]\n", result);
						return result;
					}
				} else {
					bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_RD /*0x20*/);	// Ack Read
					result = WaitForXfer(hDevice);
					if(result != I2C_OK){
						PRINTF(hDevice, "NACK4-1[%02x]\n", result);
						return result;
					}
				}
				bbm_ext_read(hDevice, BBM_I2C_RXR, &data[i]);
				i++;
			}	

			bbm_ext_write(hDevice, BBM_I2C_CR, I2C_CR_STO /*0x40*/);		// send stop

			result = WaitForXfer(hDevice);
			if((result != I2C_NACK) && (result != I2C_OK)) {
				PRINTF(hDevice, "NACK5[%02X]\n", result);
				return result;
			}

			break;

		default:
			return I2C_NOK;			
	}

	return I2C_OK;
	
}

int fci_i2c_init (HANDLE hDevice, int speed, int slaveaddr)
{
	u16 pr, rpr =0;
	
	pr = (u16)((6400/speed) -1);
	
	bbm_ext_word_write(hDevice, BBM_I2C_PR, pr);

	bbm_ext_word_read(hDevice, BBM_I2C_PR, &rpr);
	if(pr != rpr) {
		return BBM_NOK;
	}

	bbm_ext_write(hDevice, BBM_I2C_CTR, 0xC0);

	return BBM_OK;
}

int fci_i2c_deinit (HANDLE hDevice)
{
	return BBM_OK;
}

int fci_i2c_read(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	int ret;

	ret = fci_i2c_transfer(hDevice, I2C_BB_READ, chip << 1, &addr, alen, data, len);
	if(ret != I2C_OK) {
		PRINTF(hDevice, "fci_i2c_read() result=%d, addr = %x, data=%x\n", ret, addr, *data);
		return ret;
	}
	
	return ret;
}

int fci_i2c_write(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	int ret;

	ret = fci_i2c_transfer(hDevice, I2C_BB_WRITE, chip << 1, &addr, alen, data, len);
	if(ret != I2C_OK) {
		PRINTF(hDevice, "fci_i2c_write() result=%d, addr= %x, data=%x\n", ret, addr, *data);
	}

	return ret;
}

int fci_i2c_rf_read(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	int ret;

	ret = fci_i2c_transfer(hDevice, I2C_RF_READ, chip << 1, &addr, alen, data, len);
	if(ret != I2C_OK) {
		PRINTF(hDevice, "fci_i2c_rf_read() result=%d, addr = %x, data=%x\n", ret, addr, *data);
		return ret;
	}
	
	return ret;
}

int fci_i2c_rf_write(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	int ret;

	ret = fci_i2c_transfer(hDevice, I2C_RF_WRITE, chip << 1, &addr, alen, data, len);
	if(ret != I2C_OK) {
		PRINTF(hDevice, "fci_i2c_rf_write() result=%d, addr= %x, data=%x\n", ret, addr, *data);
	}

	return ret;
}


