#ifndef UART0_H
#define UART0_H

#include "types.h"

extern volatile unsigned char rfid_buffer[16];
extern volatile u8 rfid_ready;

void UART0_Init(void);
void UART0_Tx(u8 sDAT);
u8 UART0_Rx(void);
void UART0_SendString(const char *str);
void UART0_SendInteger(u32 val);
void UART0_ISR(void) __irq;
void flush_rfid_reader(void);

#endif
