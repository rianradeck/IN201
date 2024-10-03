#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef Q3
// Question 1
#define MEM_SIZE 16 * 1024
uint32_t heap[MEM_SIZE];  // Allocated on the heap 16KB

// Question 2
uint16_t hp = 0;  // The top of the heap

uint8_t* memalloc(uint16_t size) {
    if (hp + size > MEM_SIZE) {
        puts("Unnable to allocate this much memory");
        exit(1);
    }

    uint8_t* free_addr = (u_int8_t*)heap + hp;
    hp += size;
    return free_addr;
}
#endif

// Question 3
#define MEM_SIZE 16 * 1024
uint32_t MEM_ZONE[MEM_SIZE];

// I'll mark alocated zones by setting their nxt_addr to 2^16 - 1;
#define ALOCATED_NEXT (uint64_t)65535

struct zone {
    uint16_t* nxt_addr;  // 8 bytes bruh
    uint16_t size;       // 2 bytes
    uint8_t data[];
};

// Helper functions
void print_zone(struct zone* z) {
    if (z)
        printf("Zone: %p, Size: %u, Points to: %p\n", z, (uint32_t)(z->size),
               z->nxt_addr);
}

void print_mem(struct zone* z) {
    if (z == NULL) return;
    print_zone(z);
    print_mem((struct zone*)z->nxt_addr);
}

uint8_t is_alocated(struct zone* z){
    return (uint64_t)(z->nxt_addr) == ALOCATED_NEXT;
}

struct zone* heap;

// Question 4
void meminit() {
    heap = (struct zone*)MEM_ZONE;
    heap->nxt_addr = NULL;
    heap->size = MEM_SIZE;
}

// Made a general function might as well use it
struct zone* allocate_in_address(struct zone** z, struct zone* alocated_zone,
                                 uint16_t size) {
    alocated_zone = *z;
}

// Question 5
struct zone* memalloc(uint16_t size) {
    // cannot use sizeof(struct zone) because it will cast to the next mutiple of 8 (16 in this case)
    if (size < 10) {
        puts("Alocations with size less then 10 are not allowed");
        return NULL;
    }

    struct zone** z = &heap;
    while (*z && (*z)->size < size) z = (struct zone**)&(*z)->nxt_addr;
    if (!*z) return NULL;

    struct zone* alocated_zone = *z;

    uint16_t* temp_addr = (*z)->nxt_addr;
    uint16_t temp_size = (*z)->size;
    *z = (struct zone*)((uint8_t*)(*z) + size);
    (*z)->nxt_addr = temp_addr;
    (*z)->size = temp_size - size;

    alocated_zone->nxt_addr = (uint16_t *)ALOCATED_NEXT;
    alocated_zone->size = size;
    return alocated_zone;
}

void memfree(struct zone* z) {
    if(!is_alocated(z)){
        puts("Cannot free unused zone");
        return;
    }

    z->nxt_addr = NULL;
    for(int i = 0;i < z->size - 10;i++) z->data[i] = 0; // security purposes

    struct zone** _z = &heap;
    while (*_z && (*_z)->nxt_addr != NULL) _z = (struct zone**)&(*_z)->nxt_addr;
    (*_z)->nxt_addr = (uint16_t *)z;
}

int main() {
    meminit();
    print_mem(heap);
    puts("--------");

    struct zone* z1 = memalloc(32);
    struct zone* z2 = memalloc(16);
    struct zone* z3 = memalloc(32);
    
    print_mem(heap);
    puts("--------");
    memfree(z2);
    print_mem(heap);
    puts("--------");
    memfree(z3);
    print_mem(heap);
    puts("--------");
    struct zone* z4 = memalloc(16);
    struct zone* z5 = memalloc(128);
    print_mem(heap);
    puts("--------");
    memfree(z5);
    print_mem(heap);
    puts("--------");
}