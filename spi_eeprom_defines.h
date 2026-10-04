// spi_eeprom_defines.h
#ifndef __SPI_EEPROM_DEFINES_H__
#define __SPI_EEPROM_DEFINES_H__

#include "types.h"

// AT25LC512 Instruction Set
#define WREN   0x06  // Write Enable
#define WRDI   0x04  // Write Disable
#define WRITE  0x02  // Write Data to Memory Array
#define READ   0x03  // Read Data from Memory Array
#define RDSR   0x05  // Read Status Register
#define WRSR   0x01  // Write Status Register

void Cmd_25LC512(u8 cmd);
void EEPROM_WaitReady(void);
void ByteWrite_25LC512(u16 wBufAddr, u8 dat);
u8 ByteRead_25LC512(u16 rBufAddr);
void BufferWrite_25LC512(u16 wBufAddr, const u8 *pBuf, u16 len);
void BufferRead_25LC512(u16 rBufAddr, u8 *pBuf, u16 len);

#endif
