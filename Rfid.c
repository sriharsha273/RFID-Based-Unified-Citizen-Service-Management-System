#include <lpc21xx.h>
#include "types.h"
#include "rfid.h"
#include "uart_defines.h"
#include "uart0.h"
void RFID_Init(void)
{
	// cfg p0.0 & p0.1 pins as TxD0 and RxD0 respectively
	PINSEL0 &= ~0x0000000F;
	PINSEL0 |= 0x00000005;
	// cfg U0LCR for 8N1 & DLAB activate
	U0LCR = ((1<<DLAB_BIT)|(_8BIT<<WORD_LEN_SEL_BITS));
	// CFG BAUDRATE
	U0DLL = DIVISOR;
	U0DLM = DIVISOR>>8;
	// RESET DLAB BIT
	U0LCR &= ~(1<<DLAB_BIT);
}
unsigned char RFID_Readchar(unsigned char *cardid)
{
	unsigned char data;
	unsigned char i;
	//wait for the start of the test (0x02)
	do{
	data=UART0_Rx();
	}while(data!=0x02);
	//rerad 8 byte rfid number
	for(i=0;i<RFID_ID_LEN;i++)
	{
	cardid[i]=UART0_Rx();
	}
	//read end of the text(0x03)
	data=UART0_Rx();
	if(data!=0x03)
		return 0;
	cardid[RFID_ID_LEN]='\0';
	return 1;
}			


