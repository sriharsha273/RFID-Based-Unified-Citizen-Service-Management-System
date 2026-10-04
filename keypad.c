/******************************************************************************
 * @file    keypad.c
 * @brief   Matrix Keypad (KPM) driver implementation for LPC2148.
 *          Provides non-blocking scans, debounce filters, and timeout handlers.
 * @author  RFID Pair Programming Team
 * @date    July 2026
 ******************************************************************************/

#include "types.h"
#include "defines.h"
#include "kpm_defines.h"
#include <LPC21xx.h>
#include "lcd_defines.h"
#include "delay.h"
extern volatile u8 auto_logout_flag;
/**
 * @brief  Keypad lookup table for 4x4 matrix configuration.
 */
u8 kpmLUT[4][4]=
{
	{'1','2','3','A'},
	{'4','5','6','B'},
	{'7','8','9','C'},
	{'*','0','#','D'}
};

/**
 * @brief  Initialize rows of KPM as outputs and columns as inputs.
 */
void InitKPM(void)
{
  //ground all row lines
  WRITENIBBLE(IODIR1,ROW0,15);	
}	

/**
 * @brief  Checks if any column is pulled low.
 * @return 0 if a key is pressed, 1 otherwise.
 */
u8  ColScan(void)
{
	u8 t;
	t=(READNIBBLE(IOPIN1,COL0)<15)?0:1;
	return t; 
}

/**
 * @brief  Scan rows iteratively to find which row has a low column signal.
 * @return Row index (0-3).
 */
u8   RowCheck(void)
{
	u8 r;
	for(r=0;r<4;r++)
	{
		//ground iteratively one row
		WRITENIBBLE(IOPIN1,ROW0,~(1<<r));
		//check if key was pressed in that row
		if(!ColScan())
			break;
	}
  //re-initialize all rows as ground
  WRITENIBBLE(IOPIN1,ROW0,0);
  return r; 	
}

/**
 * @brief  Scan columns to find which specific column pin is pulled low.
 * @return Column index (0-3).
 */
u8   ColCheck(void)
{
	u8 c;
	for(c=0;c<4;c++)
	{
		if(READBIT(IOPIN1,COL0+c)==0)
			break;
	}
	return c;
}

extern volatile u8 rtc_interrupted_flag;

/**
 * @brief  Scans the keypad with a maximum timeout. Returns key character.
 *         Handles debounce, active release, and checks for RTC interrupt flags.
 * @param  timeout_ms Maximum polling time in milliseconds (0xFFFFFFFF for infinite).
 * @return ASCII character of key, 0 on timeout, or 0xFE if RTC interrupt occurs.
 */
u8 KeyScanWithTimeout(u32 timeout_ms)
{
    u32 elapsed = 0;
    u8 r, c, keyV;
    u32 last_timer_sec = 0xFFFFFFFF;

    /* Wait for any key press */
    while (ColScan())
    {
        /* Check RTC interrupt */
        if (rtc_interrupted_flag)
        {
            rtc_interrupted_flag = 0;
            return 0xFE;
        }

        /* Display countdown */
        if (timeout_ms != 0xFFFFFFFF && timeout_ms >= 1000)
        {
            u32 remaining_seconds;

            remaining_seconds = (timeout_ms - elapsed) / 1000;

            if (remaining_seconds != last_timer_sec)
            {
                u8 saved_lcd_pos = lcd_current_pos;
                
                last_timer_sec = remaining_seconds;

                /*
                 * Display timer on LCD line 4 and restore active cursor position.
                 */
                CmdLCD(GOTO_LINE4_POS0 + 16);
                StrLCD("T-");

                CharLCD('0' + (remaining_seconds / 10));
                CharLCD('0' + (remaining_seconds % 10));

                CmdLCD(saved_lcd_pos); // Restore cursor position immediately

                /* Compensate for LCD write delay */
                elapsed += 12;
            }
        }

        delay_ms(1);

        /* Update elapsed time */
        if (timeout_ms != 0xFFFFFFFF)
        {
            elapsed++;

            /* Timeout */
            if (elapsed >= timeout_ms)
            {
                auto_logout_flag = 1;
                return 0;
            }
        }
    }

    /* Debounce key press */
    delay_ms(20);

    if (ColScan())
        return 0;

    /* Identify row */
    r = RowCheck();

    /* Identify column */
    c = ColCheck();

    /* Get key value from lookup table */
    keyV = kpmLUT[r][c];

    /* Wait until key is released */
    while (ColScan() == 0)
    {
        delay_ms(1);
    }

    /* Debounce key release */
    delay_ms(20);

    return keyV;
}
/**
 * @brief  Synchronously scans the keypad (blocking call).
 * @return ASCII character of pressed key.
 */
u8 KeyScan(void)
{
	return KeyScanWithTimeout(0xFFFFFFFF);
}

/**
 * @brief  Reads a numeric value from the keypad and echoes it on the LCD.
 * @return Entered numeric value.
 */
u32 ReadNum(void)
{
	u32 keyV,sum=0;
	while(1)
	{
		keyV=KeyScan();
		if((keyV>='0') && (keyV<='9'))
		{
			sum=(sum*10)+(keyV-'0');
			CmdLCD(GOTO_LINE2_POS0);
			U32LCD(sum);
			while(ColScan()==0);
		}
		else
		{
			if(keyV=='*')
			{
				CmdLCD(CLEAR_LCD);
			}
			while(ColScan()==0);
			break;
		}
	}
	return sum;
}

/**
 * @brief  Reads a numeric value from the keypad and outputs the final exit key.
 * @param  num Reference pointer to store entered number.
 * @param  lastKey Reference pointer to store final pressed control key.
 */
void ReadNum2(u32 *num,u8 *lastKey)
{
	while(1)
	{
		*lastKey=KeyScan();
		if((*lastKey>='0') && (*lastKey<='9'))
		{
			*num=(*num * 10)+(*lastKey-'0');
			CmdLCD(GOTO_LINE2_POS0);
			U32LCD(*num);
			while(ColScan()==0);
		}
		else
		{
			if(*lastKey=='*')
			{
				CmdLCD(CLEAR_LCD);
			}
			while(ColScan()==0);
			break;
		}
	}
}
