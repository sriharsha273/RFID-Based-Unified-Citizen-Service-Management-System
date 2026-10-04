//rfid.h
#ifndef __RFID_H__
#define __RFID_H__
#include<lpc21xx.h>
#define  RFID_ID_LEN  8
void RFID_Init(void);
unsigned char RFID_Readchar(unsigned char *cardid);

#endif
