/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8101_spi.c
 
 Description : API of dmb baseband module
 
 History : 
 ----------------------------------------------------------------------
 2009/09/14 	jason		initial
*******************************************************************************/

#ifndef __FC8101_SPI__
#define __FC8101_SPI__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8101_spi_init(HANDLE hDevice, u16 param1, u16 param2);
extern int fc8101_spi_byteread(HANDLE hDevice, u16 addr, u8 *data);
extern int fc8101_spi_wordread(HANDLE hDevice, u16 addr, u16 *data);
extern int fc8101_spi_longread(HANDLE hDevice, u16 addr, u32 *data);
extern int fc8101_spi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8101_spi_bytewrite(HANDLE hDevice, u16 addr, u8 data);
extern int fc8101_spi_wordwrite(HANDLE hDevice, u16 addr, u16 data);
extern int fc8101_spi_longwrite(HANDLE hDevice, u16 addr, u32 data);
extern int fc8101_spi_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8101_spi_dataread(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8101_spi_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif // __FC8101_SPI__

