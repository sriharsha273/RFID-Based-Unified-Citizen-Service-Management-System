/******************************************************************************
 * @file    lcd.c
 * @brief   HD44780 compliant 4-line LCD character display driver for LPC2148.
 *          Supports commands, characters, numbers, and custom CGRAM characters.
 * @author  RFID Pair Programming Team
 * @date    July 2026
 ******************************************************************************/

#include <LPC21xx.h>
#include "lcd_defines.h"
#include "defines.h"
#include "types.h"
#include "delay.h"

/**
 * @brief  Sends a data byte to the LCD parallel port and generates an Enable latch pulse.
 * @param  byte 8-bit instruction or data byte to transmit.
 */
void WriteLCD(u8 byte)
{
	//select write operation
	IOCLR0=1<<LCD_RW;
	//place any byte on data pins d0 to d7
	WRITEBYTE(IOPIN0,LCD_DATA,byte);
	//provide high to low pulse for latching
	IOSET0=1<<LCD_EN;
	delay_us(1);
	IOCLR0=1<<LCD_EN;
	delay_ms(2);
}

volatile u8 lcd_current_pos = 0x80;

/**
 * @brief  Sends an instruction command to the LCD display (RS=0).
 * @param  cmd Command instruction byte.
 */
void CmdLCD(u8 cmd)
{
	//select cmd register
	IOCLR0=1<<LCD_RS;
	//write any cmd to lcd
	WriteLCD(cmd);
	if (cmd >= 0x80)
	{
		lcd_current_pos = cmd;
	}
	else if (cmd == CLEAR_LCD || cmd == RET_CUR_HOME)
	{
		lcd_current_pos = 0x80;
		delay_ms(5); // Extra delay for clearing / homing display
	}
}

/**
 * @brief  Initialize LCD pins as outputs and execute power-on commands.
 */
void InitLCD(void)
{
	//cfg lcd connection gpio output pins
	WRITEBYTE(IODIR0,LCD_DATA,0xFF);
	SETBIT(IODIR0,LCD_RS);
	SETBIT(IODIR0,LCD_RW);
	SETBIT(IODIR0,LCD_EN);
	//power on delay
	delay_ms(15);
	CmdLCD(0x30);
	delay_ms(4);
	delay_us(100);
	CmdLCD(0x30);
	delay_us(100);
	CmdLCD(0x30);
	CmdLCD(MODE_8BIT_2LINE);
	CmdLCD(DSP_ON_CUR_OFF);
	CmdLCD(CLEAR_LCD);
	CmdLCD(SHIFT_CUR_RIGHT);
}

/**
 * @brief  Writes a single character byte onto the display screen (RS=1).
 * @param  asciiVal Character byte to write.
 */
void CharLCD(u8 asciiVal)
{
	//select data register	 
	IOSET0=1<<LCD_RS;
	//write to ddram via data register
	WriteLCD(asciiVal);
	lcd_current_pos++;
}

/**
 * @brief  Writes a string of characters onto the display.
 * @param  s Reference pointer to null-terminated string array.
 */
void StrLCD(s8 *s)
{
    	while(*s)
		{
		CharLCD(*s++);
		}
}

/**
 * @brief  Formulates and writes an unsigned 32-bit integer value on the LCD.
 * @param  n Unsigned integer value.
 */
void U32LCD(u32 n)
{
	s32 i=0;
	u8 a[10];
	if(n==0)
	{
		CharLCD('0');
	}
  else
	{
		while(n>0)
		{
			a[i++]=(n%10)+48;
			n/=10;
		}
		for(--i;i>=0;i--)
		{
			CharLCD(a[i]);
		}
	}		
}

/**
 * @brief  Formulates and writes a signed 32-bit integer value on the LCD.
 * @param  n Signed integer value.
 */
void S32LCD(s32 n)
{
	if(n<0)
	{
		CharLCD('-');
		n=-n;
	}
	U32LCD(n);
}

/**
 * @brief  Formulates and writes a single-precision floating point number on the LCD.
 * @param  fn Floating point number.
 * @param  nDP Number of decimal places to output.
 */
void F32LCD(f32 fn,u8 nDP)
{
	 u32 n,i;
	 if(fn<0.0)
	 {	 
		 CharLCD('-');
		 fn=-fn;
	 }	 
		  n=fn;
		  U32LCD(n);
		  CharLCD('.');
	 for(i=0;i<nDP;i++)
	 {
		fn=(fn-n)*10;
		n=fn;
    	CharLCD(n+48);		 
	 }		 
}

/**
 * @brief  Builds and registers custom graphics patterns in the LCD CGRAM.
 * @param  p Reference pointer to custom pattern array bytes.
 * @param  nBytes Total pattern byte capacity (number of custom icons * 8).
 */
void BuildCGRAM(u8 *p,u8 nBytes)
{
	u32 i;
	CmdLCD(GOTO_CGRAM_START);
	IOCLR0=1<<LCD_RW;
	IOSET0=1<<LCD_RS;
	for(i=0;i<nBytes;i++)
	{
		//write to cgram via  data reg
		WriteLCD(p[i]);
	}
	//return back ddram
	CmdLCD(GOTO_LINE1_POS0);
	delay_ms(5); // Settle time after CGRAM write
}
