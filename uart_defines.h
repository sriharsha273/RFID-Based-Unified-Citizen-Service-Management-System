//Uart_defines.h
#define TxD0_PIN_FUNC 0x00000001
#define RxD0_PIN_FUNC 0x00000005
#define BAUD 9600
#define FOSC 1200000
#define CCLK (FOSC*5)
#define PCLK (CCLK/4)
#define DIVISOR (PCLK /(16*BAUD))
//defines for UXLCR
#define _8BIT 3
#define WORD_LEN_SEL_BITS 0
#define DLAB_BIT 7
//defines for uxLSR
#define TEMT_BIT 6
#define DR_BIT   0







