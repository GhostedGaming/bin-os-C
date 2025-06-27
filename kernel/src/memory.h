#ifndef MEMORY_H
#define MEMORY_H
#include <stddef.h>
#include <stdint.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *allocate_memory(size_t size);
int memcmp(const void *s1, const void *s2, size_t n);
uint64_t detect_total_memory(void);

void init_memory_allocator(void);
void free_memory(void *ptr);

#endif // MEMORY_H