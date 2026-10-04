#ifndef __KPM_H__
#define __KPM_H__

#include "types.h"

void InitKPM(void);
u8   ColScan(void);
u8   RowCheck(void);
u8   ColCheck(void);
u8   KeyScan(void);
u8   KeyScanWithTimeout(u32 timeout_ms);
u32  ReadNum(void);
void ReadNum2(u32 *num, u8 *lastKey);

#endif
