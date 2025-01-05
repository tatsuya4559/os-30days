#pragma once

#include "types.h"

uint32_t memtest(uint32_t start, uint32_t end);

#define MEMMAN_FREES 4090

typedef struct {
  uint32_t addr, size;
} FreeInfo;

typedef struct {
  int32_t frees, maxfrees, lostsize, losts;
  FreeInfo free[MEMMAN_FREES];
} MemoryManager;

void memman_init(MemoryManager *man);
uint32_t memman_total(MemoryManager *man);
uint32_t memman_alloc(MemoryManager *man, uint32_t size);
int32_t memman_free(MemoryManager *man, uint32_t addr, uint32_t size);
uint32_t memman_alloc_4k(MemoryManager *memman, uint32_t size);
int32_t memman_free_4k(MemoryManager *memman, uint32_t addr, uint32_t size);
