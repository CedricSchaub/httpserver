#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <malloc.h>
#include <assert.h>

typedef struct {
    uint32_t size;
    uint32_t offset; // next free memory location
    char *base;
} memory_arena;

void initialize_arena(memory_arena *arena, uint32_t arena_size) {
   char *memory_ptr = (char *)malloc(sizeof(char) * arena_size);
   assert(memory_ptr != NULL);

   arena->size = arena_size;
   arena->base = memory_ptr;
   arena->offset = 0;

   printf("Allocated %i bytes\nMemory base address at: %p\n", sizeof(char) * arena_size, arena->base);
}

char *arena_allocate(memory_arena *arena, uint32_t mem_size) {
    // prevent promotion to other type. will probably never happen but who knows
    uint32_t new_offset = (uint32_t)(mem_size + arena->offset);
    // check if there is enough space for the base. also check for unsigned integer overflow
    assert(new_offset > arena->offset && new_offset < arena->size);

    char *memory_ptr = arena->base + arena->offset;
    printf("Allocated %i bytes beginning from address: %p\n", sizeof(char) * mem_size, memory_ptr);
    arena->offset = new_offset;

    return memory_ptr;
}

void reset_arena(memory_arena *arena) {
    arena->offset = 0;
}

void free_arena(memory_arena *arena) {
    if(arena->base == NULL) return;

    free(arena->base);
    arena->offset = 0;
    arena->size = 0;
}
#endif
