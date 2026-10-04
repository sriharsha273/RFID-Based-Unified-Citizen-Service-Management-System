/******************************************************************************
 * @file    uart.c
 * @brief   UART0 peripheral transceiver driver for LPC2148.
 *          Implements standard serial transmission, integer formatting,
 *          and an ISR handler for reading RFID card buffers asynchronously.
 * @author  RFID Pair Programming Team
 * @date    July 2026
 ******************************************************************************/

#include <LPC21xx.h>
#include "types.h"
#include "uart_defines.h"
#include "delay.h"

volatile unsigned char rfid_buffer[16]; ///< Storage buffer for incoming RFID tag characters.
volatile u8 rfid_index = 0;              ///< Incoming character index for RFID buffer.
volatile u8 rfid_ready = 0;              ///< Flag set to 1 when a full RFID frame is received.

/**
 * @brief  Initialize UART0 at 9600 baud (assuming 15MHz Pclk, DLL=97, DLM=0).
 *         Configures RxD0 and TxD0 pins and enables RX Interrupts.
 */
void UART0_Init(void)
{
    // cfg p0.0, p0.1 pin as TxD0 and RxD0 pins respectively
    PINSEL0 &= ~0x0000000F;
    PINSEL0 |= 0x00000005;
    // cfg UxCLR for 8N1 & DLAB Activate
    U0LCR = ((1<<DLAB_BIT)|(_8BIT<<WORD_LEN_SEL_BITS));
    // CFG BAUDRATE
    U0DLL = 97;
    U0DLM = 0;
    // RESET DLAB BIT
    U0LCR &= ~(1<<DLAB_BIT);
    
    // Enable FIFO & reset RX/TX FIFOs
    U0FCR = 0x07;
    
    // Clear stale status and data registers
    (void)U0LSR;
    (void)U0RBR;
    
    rfid_index = 0;
    rfid_ready = 0;
    rfid_buffer[0] = '\0';
    
    U0IER = 0x01; // Enable UART0 RX Interrupt
}

/**
 * @brief  Flush RFID buffer state and drain UART RX FIFO to clear hardware noise.
 */
void flush_rfid_reader(void)
{
    rfid_ready = 0;
    rfid_index = 0;
    rfid_buffer[0] = '\0';
    
    while ((U0LSR & 0x01))
    {
        u8 dummy = U0RBR;
        (void)dummy;
    }
}

/**
 * @brief  Transmit a single byte of data over UART0.
 *         Blocks execution until the Transmitter Empty (TEMT) status goes high.
 * @param  sDAT Data byte to transmit.
 */
void UART0_Tx(u8 sDAT)
{
    //write to THR (UxTHR),serialiaztion BE GINS
    U0THR=sDAT;
    //wait until serialaizatin
    while(((U0LSR>>TEMT_BIT)&1)==0);
}

/**
 * @brief  Receive a single byte of data from UART0.
 *         Blocks execution until the Data Ready (DR) status bit is set.
 * @return Received data byte.
 */
u8 UART0_Rx(void)
{
    //wait until reception status observed
    while(((U0LSR>>DR_BIT)&1)==0);
    //read &return received byte
    return U0RBR;
}

/**
 * @brief  Transmit a null-terminated string over UART0.
 * @param  str Reference pointer to character array.
 */
void UART0_SendString(const char *str)
{
    while (*str)
    {
        UART0_Tx(*str++);
    }
}

/**
 * @brief  Format and transmit a decimal integer value over UART0.
 * @param  val Numeric value to format.
 */
void UART0_SendInteger(u32 val)
{
    char buf[12];
    int i = 0;
    if (val == 0)
    {
        UART0_Tx('0');
        return;
    }
    while (val > 0)
    {
        buf[i++] = (val % 10) + '0';
        val /= 10;
    }
    while (i > 0)
    {
        UART0_Tx(buf[--i]);
    }
}

/**
 * @brief  UART0 Interrupt Service Routine (ISR) triggered by hardware.
 *         Decodes incoming serial frames enclosed by STX (0x02) and ETX (0x03) delimiters.
 */
void UART0_ISR(void) __irq
{
    u8 iir = U0IIR & 0x0E;
    
    // Check for Receiver Line Status Interrupt (Overrun, Parity, Framing Error)
    if (iir == 0x06)
    {
        u8 dummy = U0LSR; // Reading U0LSR clears Line Status Interrupt
        (void)dummy;
    }
    else if (iir == 0x04 || iir == 0x0C) // RX data available or character timeout
    {
        u8 data = U0RBR;
        
        if (data == 0x02) // Start byte STX
        {
            if (!rfid_ready)
            {
                rfid_index = 0;
                rfid_buffer[0] = '\0';
            }
        }
        else if (data == 0x03 || data == '\r' || data == '\n') // End byte ETX / CR / LF
        {
            if (rfid_index >= 8 && !rfid_ready)
            {
                rfid_buffer[rfid_index] = '\0';
                rfid_ready = 1;
            }
        }
        else if (data >= 32 && data <= 126) // Printable ASCII character payload
        {
            if (!rfid_ready && rfid_index < 15)
            {
                rfid_buffer[rfid_index++] = data;
                rfid_buffer[rfid_index] = '\0';
                
                // Fallback capping for fixed-length modules without ETX delimiter
                if (rfid_index >= 12)
                {
                    rfid_ready = 1;
                }
            }
        }
    }
    
    VICVectAddr = 0x00; // End of interrupt
}
