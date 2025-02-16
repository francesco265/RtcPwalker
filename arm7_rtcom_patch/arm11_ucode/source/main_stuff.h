#pragma once
#include "a11ucode.h"

#define RTC_CNT			0x1EC47100
#define RTC_REG_TIMER2	0x1EC47134

extern u8 ir_buffer[136];

void *pat_memesearch(const void *patptr, const void *bitptr, const void *searchptr, u32 searchlen,
                     u32 patsize, u32 alignment);

void ir_beginComm();
void ir_endComm();
void ir_send(u8 size);
u8 ir_recv();
