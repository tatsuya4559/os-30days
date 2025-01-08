#pragma once

#include "types.h"

#define  ADR_GDT       0x00270000

typedef struct {
  short limit_low, base_low;
  uint8_t base_mid, access_right;
  uint8_t limit_high, base_high;
} SegmentDescriptor;


void init_gdtidt(void);
void set_segmdesc(SegmentDescriptor *sd, uint32_t limit, int32_t base, int32_t ar);
