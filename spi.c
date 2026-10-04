/******************************************************************************
 * @file    spi.c
 * @brief   LPC2148 SPI0 peripheral driver implementation.
 *          Manages master mode settings, bit rate generation, and byte exchange.
 * @author  RFID Pair Programming Team
 * @date    July 2026
 ******************************************************************************/

#include "types.h"
#include <LPC21xx.h>
#include "spi_defines.h"

/**
 * @brief  Initialize SPI0 peripheral in Master mode.
 *         Configures SCK0, MISO0, MOSI0 pin functions, sets clock speed divider,
 *         and defines active-low Chip Select P0.7 as GPIO output.
 */
void Init_SPI0(void)
{
  PINSEL0 &= ~0x00003F00; // Clear bits 13:8 for SPI0 SCK0/MISO0/MOSI0
  PINSEL0 |= SCK0|MISO0|MOSI0; // select SPI pin functionality  

  S0SPCCR = 8;     // Optimized bit clock rate (max clock frequency for fast EEPROM access)

  // SPI Master mode, CPOL=1, CPHA=1, MSB first
  S0SPCR  = (1<<MSTR_BIT)|(1<<CPHA_BIT)|(1<<CPOL_BIT); 	

  // Disable chip select (active low, set P0.7 high)
  IOSET0 = 1<<CS;

  // Set Chip Select pin direction as GPIO output
  IODIR0 |= 1<<CS;
}

/**
 * @brief  Transmit a byte of data and receive a byte from the SPI slave.
 *         Blocks execution until the SPIF serialization flag is set.
 * @param  data Byte of data to transmit.
 * @return Received byte of data from SPI slave.
 */
u8 SPI0(u8 data)
{
	// Load SPI data register, automatically starts serialization
	S0SPDR = data;
	// Wait until serialization (tx & rx) completes (SPIF bit 7 in status register goes high)
	while(((S0SPSR >> SPIF_BIT) & 1) == 0);
	// Read and return received byte
	return S0SPDR;
}

