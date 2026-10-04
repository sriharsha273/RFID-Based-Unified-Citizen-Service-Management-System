/******************************************************************************
 * @file    delay.c
 * @brief   System delay routines for LPC2148 microcontrollers.
 *          Implements software delay loops calibrated to the system clock.
 * @author  RFID Pair Programming Team
 * @date    July 2026
 ******************************************************************************/

#include "types.h"

/**
 * @brief  Halts execution for a specified duration in microseconds.
 * @param  dlyUS Delay duration in microseconds.
 */
void delay_us(u32 dlyUS)
{
	volatile u32 count = dlyUS * 12;
	while (count--);
}

/**
 * @brief  Halts execution for a specified duration in milliseconds.
 * @param  dlyMS Delay duration in milliseconds.
 */
void delay_ms(u32 dlyMS)
{
	volatile u32 count = dlyMS * 12000;
	while (count--);
}

/**
 * @brief  Halts execution for a specified duration in seconds.
 * @param  dlyS Delay duration in seconds.
 */
void delay_s(u32 dlyS)
{
	volatile u32 count = dlyS * 12000000;
	while (count--);
}

