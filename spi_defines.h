// spi_defines.h
#ifndef __SPI_DEFINES_H__
#define __SPI_DEFINES_H__

// SPI0 Pin Select functionality in PINSEL0
#define SCK0   0x00000100  // P0.4 configured as SCK0
#define MISO0  0x00000400  // P0.5 configured as MISO0
#define MOSI0  0x00001000  // P0.6 configured as MOSI0
#define CS     7           // P0.7 configured as GPIO Chip Select

// S0SPCR (Control Register) Bit positions
#define CPHA_BIT  3
#define CPOL_BIT  4
#define MSTR_BIT  5

// S0SPSR (Status Register) Bit positions
#define SPIF_BIT  7

void Init_SPI0(void);
u8 SPI0(u8 data);

#endif
