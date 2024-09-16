#include <stdbool.h>
#include "nasmfunc.h"
#include "memory.h"

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

static
unsigned int
memtest_sub(unsigned int start, unsigned int end)
{
  // optimizaion may break this function
  unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
  for (i = start; i <= end; i += 0x1000) {
    p = (unsigned int *) (i + 0xffc); // check last 4 bytes
    old = *p;
    *p = pat0; // try writing
    *p ^= 0xffffffff; // invert
    if (*p != pat1) {
not_memory:
      *p = old;
      break;
    }
    *p ^= 0xffffffff; // re-invert
    if (*p != pat0) {
      goto not_memory;
    }
    *p = old;
  }
  return i;
}

/* test how much memory can be used */
unsigned int
memtest(unsigned int start, unsigned int end)
{
  bool is486 = false;
  unsigned int eflg, cr0, i;

  // check if CPU is 386 or 486
  eflg = _io_load_eflags();
  eflg |= EFLAGS_AC_BIT; // AC-bit = 1
  _io_store_eflags(eflg);
  eflg = _io_load_eflags();
  if ((eflg & EFLAGS_AC_BIT) != 0) { // 386ではAC=1にしても自動で0に戻ってしまう
    is486 = true;
  }
  eflg &= ~EFLAGS_AC_BIT; // AC-bit = 0
  _io_store_eflags(eflg);

  if (is486) {
    cr0 = _load_cr0();
    cr0 |= CR0_CACHE_DISABLE; // disable cache
    _store_cr0(cr0);
  }

  i = memtest_sub(start, end);

  if (is486) {
    cr0 = _load_cr0();
    cr0 &= ~CR0_CACHE_DISABLE; // enable cache
    _store_cr0(cr0);
  }

  return i;
}

void
memman_init(MemoryManager *man)
{
  man->frees = 0;
  man->maxfrees = 0;
  man->lostsize = 0;
  man->losts = 0;
}

unsigned int
memman_total(MemoryManager *man)
{
  unsigned int i, t = 0;
  for (i = 0; i < man->frees; i++) {
    t += man->free[i].size;
  }
  return t;
}

unsigned int
memman_alloc(MemoryManager *man, unsigned int size)
{
  unsigned int i, a;
  for (i = 0; i < man->frees; i++) {
    if (man->free[i].size >= size) {
      a = man->free[i].addr;
      man->free[i].addr += size;
      man->free[i].size -= size;
      if (man->free[i].size == 0) {
        // close the hole
        man->frees--;
        for (; i < man->frees; i++) {
          man->free[i] = man->free[i + 1];
        }
      }
      return a;
    }
  }
  return 0; // no enough memory
}

int
memman_free(MemoryManager *man, unsigned int addr, unsigned int size)
{
  int i, j;
  // find the hole to insert
  for (i = 0; i < man->frees; i++) {
    if (man->free[i].addr > addr) {
      break;
    }
  }
  // free[i - 1].addr < addr < free[i].addr
  if (i > 0) {
    if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
      // merge with the hole
      man->free[i - 1].size += size;
      if (i < man->frees) {
        if (addr + size == man->free[i].addr) {
          // merge with the next hole
          man->free[i - 1].size += man->free[i].size;
          // remove the hole
          man->frees--;
          for (; i < man->frees; i++) {
            man->free[i] = man->free[i + 1];
          }
        }
      }
      return 0; // success
    }
  }
  if (i < man->frees) {
    if (addr + size == man->free[i].addr) {
      // merge with the next hole
      man->free[i].addr = addr;
      man->free[i].size += size;
      return 0; // success
    }
  }
  if (man->frees < MEMMAN_FREES) {
    // shift right
    for (j = man->frees; j > i; j--) {
      man->free[j] = man->free[j - 1];
    }
    man->frees++;
    if (man->maxfrees < man->frees) {
      man->maxfrees = man->frees;
    }
    man->free[i].addr = addr;
    man->free[i].size = size;
    return 0; // success
  }
  man->losts++;
  man->lostsize += size;
  return -1; // fail
}

unsigned int
memman_alloc_4k(MemoryManager *memman, unsigned int size)
{
  size = (size + 0xfff) & 0xfffff000; // 0x1000 - 1 を足してから下位3オクテットを切り捨てると切り上げになる
  return memman_alloc(memman, size);
}

int
memman_free_4k(MemoryManager *memman, unsigned int addr, unsigned int size)
{
  size = (size + 0xfff) & 0xfffff000;
  return memman_free(memman, addr, size);
}
