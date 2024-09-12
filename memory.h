#ifndef _MEMORY_H_
#define _MEMORY_H_

unsigned int memtest(unsigned int start, unsigned int end);

#define MEMMAN_FREES 4090

typedef struct {
    unsigned int addr, size;
} FreeInfo;

typedef struct {
    int frees, maxfrees, lostsize, losts;
    FreeInfo free[MEMMAN_FREES];
} MemoryManager;

void memman_init(MemoryManager *man);
unsigned int memman_total(MemoryManager *man);
unsigned int memman_alloc(MemoryManager *man, unsigned int size);
int memman_free(MemoryManager *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(MemoryManager *memman, unsigned int size);
int memman_free_4k(MemoryManager *memman, unsigned int addr, unsigned int size);

#endif /* _MEMORY_H_ */
