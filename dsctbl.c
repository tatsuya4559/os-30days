#include "nasmfunc.h"
#include "common.h"
#include "dsctbl.h"

#define  LIMIT_BOTPAK  0x0007ffff
#define  ADR_BOTPAK    0x00280000
#define  AR_CODE32_ER  0x409a
#define  AR_INTGATE32  0x008e

typedef struct {
  short limit_low, base_low;
  Byte base_mid, access_right;
  Byte limit_high, base_high;
} SegmentDescriptor;

typedef struct {
  short offset_low, selector;
  Byte dw_count, access_right;
  short offset_high;
} GateDescriptor;

static
void
set_segmdesc(SegmentDescriptor *sd, unsigned int limit, int base, int ar)
{
  if (limit > 0xfffff) {
    ar |= 0x8000;
    limit /= 0x1000;
  }
  sd->limit_low = limit & 0xffff;
  sd->base_low = base & 0xffff;
  sd->base_mid = (base >> 16) & 0xff;
  sd->access_right = ar & 0xff;
  sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
  sd->base_high = (base >> 24) & 0xff;
}

static
void
set_gatedesc(GateDescriptor *gd, int offset, int selector, int ar)
{
  gd->offset_low = offset & 0xffff;
  gd->selector = selector;
  gd->dw_count = (ar >> 8) & 0xff;
  gd->access_right = ar & 0xff;
  gd->offset_high = (offset >> 16) & 0xffff;
}

void
init_gdtidt(void)
{
  SegmentDescriptor *gdt = (SegmentDescriptor *) 0x00270000;
  GateDescriptor *idt = (GateDescriptor *) 0x0026f800;

  // init GDT
  for (int i=0; i<8192; i++) {
    set_segmdesc(gdt + i, 0, 0, 0);
  }
  set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
  set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
  _load_gdtr(0xffff, 0x00270000);

  // init IDT
  for (int i=0; i< 256; i++) {
    set_gatedesc(idt + i, 0, 0, 0);
  }
  _load_idtr(0x7ff, 0x0026f800);
  set_gatedesc(idt + 0x20, (int) _asm_inthandler20, 2 << 3, AR_INTGATE32);
  set_gatedesc(idt + 0x21, (int) _asm_inthandler21, 2 << 3, AR_INTGATE32);
  set_gatedesc(idt + 0x2c, (int) _asm_inthandler2c, 2 << 3, AR_INTGATE32);
}
