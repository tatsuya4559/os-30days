#pragma once

#include "types.h"

void init_gdtidt(void);

typedef struct {
  int32_t backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
  // 32bit registers
  int32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
  // 16bit registers
  int32_t es, cs, ss, ds, fs, gs;
  int32_t ldtr, iomap;
} TaskStatusSegment;

extern TaskStatusSegment tss_a, tss_b;
