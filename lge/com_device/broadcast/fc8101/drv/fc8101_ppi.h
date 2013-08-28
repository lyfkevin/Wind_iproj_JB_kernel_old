/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8101_ppi.c
 
 Description : API of dmb baseband module
 
 History : 
 ----------------------------------------------------------------------
 2009/09/14 	jason		initial
*******************************************************************************/

#ifndef __FC8101_PPI_H__
#define __FC8101_PPI_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8101_ppi_init(HANDLE hDevice, u16 param1, u16 param2);
extern int fc8101_ppi_byteread(HANDLE hDevice, u16 addr, u8 *data);
extern int fc8101_ppi_wordread(HANDLE hDevice, u16 addr, u16 *data);
extern int fc8101_ppi_longread(HANDLE hDevice, u16 addr, u32 *data);
extern int fc8101_ppi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8101_ppi_bytewrite(HANDLE hDevice, u16 addr, u8 data);
extern int fc8101_ppi_wordwrite(HANDLE hDevice, u16 addr, u16 data);
extern int fc8101_ppi_longwrite(HANDLE hDevice, u16 addr, u32 data);
extern int fc8101_ppi_bulkwrite(HANDLE hDevice, u16 addr, u8* data, u16 length);
extern int fc8101_ppi_dataread(HANDLE hDevice, u16 addr, u8* data, u16 length);
extern int fc8101_ppi_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif // __FC8101_PPI_H__

