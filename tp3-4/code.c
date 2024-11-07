#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define STACK_SIZE 4096

u_int32_t stack[4][STACK_SIZE];

typedef void *coroutine_t;

void enter_coroutine(coroutine_t cr);

coroutine_t init_coroutine(void *stack_begin, u_int32_t stack_size,
                           void (*initial_pc)(void)) {
    u_int8_t *stack_end = ((u_int8_t *)stack_begin) + stack_size;
    printf("Ininting coroutine\nStack start: %p\nStack end: %p\nFunction addr: %p\n", stack_begin, stack_end, initial_pc);
    void **ptr = (void*)stack_end;
    ptr--;
    *ptr = initial_pc;
    printf("(initial_pc) ptr: %p, val: %p\n", ptr, *ptr);
    ptr--;
    for(int i = 1;i <= 5;i++){
        *ptr = 0;
        printf("(internal) ptr: %p, val: %p\n", ptr, *ptr);
        ptr--;
    }

    printf("(final) ptr: %p, val: %p\n", ptr, *ptr);
    return ptr;
}

void inf_run(){
    // uint32_t x = 1;
    char buf[4] = "1234";
    fflush(stdout); // Will now print everything in the stdout buffer
    puts("A");
    write(1, buf, 4);
    puts(",");
    printf("%s", buf);
    // write(1, (int *)&buf[0], 4);
    puts("B");
    fflush(stdout); // Will now print everything in the stdout buffer
    // printf("%p\n", &x);
    while(1){
        puts("$ajkgsdahsj");
        // break;
        printf("%p\n", buf);
        // printf("\33[2K");
        // x++;
    }
}

int main() {
    printf("%p\n", inf_run);
    coroutine_t cr = init_coroutine(&stack[0], STACK_SIZE, inf_run);
    
    enter_coroutine(cr);
    // inf_run();
}