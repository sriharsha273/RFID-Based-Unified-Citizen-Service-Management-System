/******************************************************************************
 * @file    spi_eeprom.c
 * @brief   Microchip 25LC512 SPI EEPROM driver implementation.
 *          Implements hardware status polling (RDSR WIP bit) and multi-byte
 *          page write / sequential read for maximum efficiency.
 *          Manages dynamic pin switching to prevent bus conflicts with LCD.
 * @author  RFID Pair Programming Team
 * @date    July 2026
 ******************************************************************************/

#include <LPC21xx.h>
#include "types.h"
#include "spi_defines.h"
#include "spi_eeprom_defines.h"
#include "delay.h"

/**
 * @brief  Sends a command instruction to the SPI EEPROM (e.g. WREN, WRDI).
 * @param  cmd EEPROM command code byte.
 */
void Cmd_25LC512(u8 cmd)
{
  IOCLR0 = 1 << CS;
  SPI0(cmd); // Issue instruction
  IOSET0 = 1 << CS;
}

/**
 * @brief  Polls the 25LC512 Status Register (RDSR) until the internal write cycle completes.
 *         Replaces fixed software delays with hardware status polling (WIP bit) for maximum speed.
 */
void EEPROM_WaitReady(void)
{
  u8 status;
  
  // Ensure P0.4 - P0.6 pins are configured for SPI0
  PINSEL0 &= ~0x00003F00;
  PINSEL0 |= 0x00001500;
  
  do {
    IOCLR0 = 1 << CS;
    SPI0(RDSR);
    status = SPI0(0x00);
    IOSET0 = 1 << CS;
  } while (status & 0x01); // Bit 0 is WIP (Write In Progress)
}

/**
 * @brief  Writes a single byte of data to a specific 16-bit EEPROM address.
 *         Uses hardware status polling for fast write completion.
 * @param  wBufAddr 16-bit EEPROM destination memory address.
 * @param  dat Data byte to write.
 */
void ByteWrite_25LC512(u16 wBufAddr, u8 dat)
{
  // Switch P0.4 - P0.6 pins dynamically to SPI0 function
  PINSEL0 &= ~0x00003F00;
  PINSEL0 |= 0x00001500;

  Cmd_25LC512(WREN);

  IOCLR0 = 1 << CS;
  SPI0(WRITE); 
  SPI0(wBufAddr >> 8);
  SPI0(wBufAddr & 0xFF);
  SPI0(dat);
  IOSET0 = 1 << CS;

  // Hardware status polling (replaces 10ms fixed delay for 3x speedup)
  EEPROM_WaitReady();

  Cmd_25LC512(WRDI);

  // Restore P0.4 - P0.6 pins to standard GPIO output mode for LCD data
  PINSEL0 &= ~0x00003F00;
  IODIR0 |= 0x00000FF0;
}

/**
 * @brief  Reads a byte of data from a specific 16-bit EEPROM address.
 * @param  rBufAddr 16-bit EEPROM source memory address.
 * @return Retrieved data byte from EEPROM.
 */
u8 ByteRead_25LC512(u16 rBufAddr)
{
  u8 dat;

  // Switch P0.4 - P0.6 pins dynamically to SPI0 function
  PINSEL0 &= ~0x00003F00;
  PINSEL0 |= 0x00001500;

  IOCLR0 = 1 << CS;
  SPI0(READ);   
  SPI0(rBufAddr >> 8);
  SPI0(rBufAddr & 0xFF);   
  dat = SPI0(0x00);
  IOSET0 = 1 << CS;

  // Restore P0.4 - P0.6 pins to standard GPIO output mode for LCD data
  PINSEL0 &= ~0x00003F00;
  IODIR0 |= 0x00000FF0;

  return dat;   
}

/**
 * @brief  Writes a buffer of data to 25LC512 EEPROM using 128-byte Page Write capability.
 * @param  wBufAddr 16-bit destination EEPROM address.
 * @param  pBuf Pointer to input data buffer.
 * @param  len Number of bytes to write.
 */
void BufferWrite_25LC512(u16 wBufAddr, const u8 *pBuf, u16 len)
{
  u16 i, bytes_to_write, page_offset;

  if (!pBuf || len == 0) return;

  // Switch P0.4 - P0.6 pins dynamically to SPI0 function
  PINSEL0 &= ~0x00003F00;
  PINSEL0 |= 0x00001500;

  while (len > 0)
  {
    // AT25LC512 page size is 128 bytes (A6-A0 bits)
    page_offset = wBufAddr & (128 - 1);
    bytes_to_write = 128 - page_offset;
    if (bytes_to_write > len) bytes_to_write = len;

    Cmd_25LC512(WREN);

    IOCLR0 = 1 << CS;
    SPI0(WRITE);
    SPI0(wBufAddr >> 8);
    SPI0(wBufAddr & 0xFF);

    for (i = 0; i < bytes_to_write; i++)
    {
      SPI0(pBuf[i]);
    }
    IOSET0 = 1 << CS;

    // Poll status register until internal page write completes
    EEPROM_WaitReady();

    wBufAddr += bytes_to_write;
    pBuf += bytes_to_write;
    len -= bytes_to_write;
  }

  Cmd_25LC512(WRDI);

  // Restore P0.4 - P0.6 pins to standard GPIO output mode for LCD data
  PINSEL0 &= ~0x00003F00;
  IODIR0 |= 0x00000FF0;
}

/**
 * @brief  Reads a buffer of data from 25LC512 EEPROM using sequential-read mode.
 * @param  rBufAddr 16-bit source EEPROM address.
 * @param  pBuf Pointer to destination buffer.
 * @param  len Number of bytes to read.
 */
void BufferRead_25LC512(u16 rBufAddr, u8 *pBuf, u16 len)
{
  u16 i;

  if (!pBuf || len == 0) return;

  // Switch P0.4 - P0.6 pins dynamically to SPI0 function
  PINSEL0 &= ~0x00003F00;
  PINSEL0 |= 0x00001500;

  IOCLR0 = 1 << CS;
  SPI0(READ);
  SPI0(rBufAddr >> 8);
  SPI0(rBufAddr & 0xFF);

  for (i = 0; i < len; i++)
  {
    pBuf[i] = SPI0(0x00);
  }

  IOSET0 = 1 << CS;

  // Restore P0.4 - P0.6 pins to standard GPIO output mode for LCD data
  PINSEL0 &= ~0x00003F00;
  IODIR0 |= 0x00000FF0;
}
