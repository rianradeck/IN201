#include <signal.h>
#include <stddef.h>
#include <stdio.h>  // getchar
#include <stdlib.h> // exit
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>

// Note: printf in coroutines/threads segfaults on some machines, if that is
// the case on yours, replace the printf's with calls to the handcoded
// print_str and print_int

// print a string char by char using putchar
void print_str(char *str) {
  while (*str) {
    putchar((int)*str);
    ++str;
  }
}

// print a integer in base 10 using putchat
void print_int(int x) {
  if (x < 0) {
    putchar('-');
    x = -x;
  }
  if (x == 0) {
    putchar('0');
    return;
  }
  int pos = 10;
  while (x >= pos) {
    pos *= 10;
  }
  while (pos > 1) {
    pos /= 10;
    putchar('0' + (x / pos) % 10);
  }
}

// The size of our stacks
#define STACK_SIZE_FULL 4096

// The size of our stack, minus bottom padding
#define STACK_SIZE (STACK_SIZE_FULL - 10 * sizeof(void *))

// getchar returns -1 when no character is read on stdin
#define NO_READ_CHAR -1

typedef void *coroutine_t;

/* Quitte le contexte courant et charge les registres et la pile de CR. */
void enter_coroutine(coroutine_t cr);

/* Sauvegarde le contexte courant dans p_from, et entre dans TO. */
void switch_coroutine(coroutine_t *p_from, coroutine_t to);

/* Initialise la pile et renvoie une coroutine telle que, lorsquâ€™on entrera
dedans, elle commencera Ã  sâ€™exÃ©cuter Ã  lâ€™adresse initial_pc. */
coroutine_t init_coroutine(void *stack_begin, size_t stack_size,
                           void (*initial_pc)());

// ====================================
// ============ Question 1 ============
// ====================================

char stack_1[STACK_SIZE_FULL];
char stack_2[STACK_SIZE_FULL];
char stack_3[STACK_SIZE_FULL];
char stack_4[STACK_SIZE_FULL];

// Simple guard function
// We write the pointer of this function at the top of our stack (at the start
// of main), This way, if we accidentaly pop one too many value, we get a
// clearer error message then a segfault.
void ret_too_far() {
  print_str("Error: Out of stack!\n");
  exit(5);
}

// ====================================
// ============ Question 3 ============
// ====================================

coroutine_t init_coroutine(void *stack_begin, size_t stack_size,
                           void (*initial_pc)()) {
  char *stack_end = ((char *)stack_begin) + stack_size;
  void **ptr = (void **)stack_end;
  *(--ptr) = initial_pc;
  *(--ptr) = 0; // %rbp
  *(--ptr) = 0; // %rbx
  *(--ptr) = 0; // %r12
  *(--ptr) = 0; // %r13
  *(--ptr) = 0; // %r14
  *(--ptr) = 0; // %r15
  return ptr;
}

// ====================================
// ============ Question 4 ============
// ====================================

void infinite_loop_print() {
loop:
  print_str("Looping...\n");
  goto loop;
}

// ====================================
// ============ Question 6 ============
// ====================================

// Store both coroutines as global variables, so we can switch as appropriate
coroutine_t cr1, cr2;

void func1() {
  int x = 0;
  for (;;) {
    ++x;
    print_str("In func1, x = ");
    print_int(x);
    print_str("\n");
    switch_coroutine(&cr1, cr2);
  }
}

void func2() {
  int z = 0;
  for (;;) {
    ++z;
    print_str("In func2, x = ");
    print_int(z);
    print_str("\n");
    switch_coroutine(&cr2, cr1);
  }
}

// ====================================
// ============ Question 7 ============
// ====================================

enum status {
  READY,
  BLOCKED,
};

struct thread {
  coroutine_t couroutine;
  enum status status;
};

// ====================================
// ============ Question 8 ============
// ====================================

// Same as before, we use global variables to know which coroutines to use
// in switch_coroutine. We cill always switch between scheduler and a thread
// so we only need two variables.
coroutine_t scheduler, current_thread;

void yield() { switch_coroutine(&current_thread, scheduler); }

struct thread thread_create(void *stack, void (*f)()) {
  struct thread th = {.couroutine = init_coroutine(stack, STACK_SIZE, f),
                      .status = READY};
  return th;
}

// Ideally, these would be the arguments of schedule
// but since we haven't handled arguments in enter and switch_coroutine
// we just use globals...
size_t nb_threads;
struct thread *threads;

void schedule() {
  size_t nb = 0;
  for (;;) {
    struct thread t = threads[nb];
    if (t.status == READY) {
      current_thread = t.couroutine;
      switch_coroutine(&scheduler, t.couroutine);
      // Update the pointer in our thread table, else it will always
      // point to the coroutine start
      threads[nb].couroutine = current_thread;
    }

    if (++nb >= nb_threads) {
      nb = 0;
    }
  }
}

// dummy thread
void thread1() {
  int x = 0, z = 5;
  for (;;) {
    print_str("In thread 1\n");
    x = z | (x + 3);
    z += z;
    yield();
  }
}

// Resets counter after read_char
void thread2() {
  int counter = 0;
  for (;;) {
    print_str("In thread 2 : counter is ");
    print_int(counter);
    putchar('\n');
    ++counter;
    if (getchar() != NO_READ_CHAR) {
      counter = 0;
    }
    yield();
  }
}

// super dummy thread
void thread3() {
loop:
  printf("In thread 3\n");
  goto loop;
}

// ====================================
// ============ Partie 4 ==============
// ====================================

enum state {
  EMPTY = 0,
  FULL = 1,
};

// Global sync variables
enum state state = EMPTY;
int read_char;

void producer() {
  for (;;) {
    if (state != FULL) {
      // Get char from input
      read_char = getchar();
      if (read_char != NO_READ_CHAR) {
        state = FULL;
      }
    }
    yield();
  }
}

// We need to yield at least twice between reads to give the second thread
// a chance to read anything. This wouldn't be a problem if we used a different
// scheduling algorithm
void consumer_A() {
  int chr;
  for (;;) {
    if (state == FULL) {
      chr = read_char;
      state = EMPTY;
      yield();
      print_str("Consumer A read: ");
      // Explicitly print newlines
      if (chr != '\n')
        putchar(chr);
      else
        print_str("\\n");
      putchar('\n');
      yield();
    }
    yield();
  }
}

void consumer_B() {
  int chr;
  for (;;) {
    if (state == FULL) {
      chr = read_char;
      state = EMPTY;
      yield();
      print_str("Consumer B read: ");
      if (chr != '\n')
        putchar(chr);
      else
        print_str("\\n");
      putchar('\n');
      yield();
    }
    yield();
  }
}

// ====================================
// ============== Main ================
// ====================================

#define TEST_INIT 0
#define TEST_SWITCH 1
#define TEST_THREAD 2
#define TEST_PRODUCER 3

// Change TEST to run the different parts of the TP
#define TEST TEST_INIT

int main() {
  // Optionally add guards to stack ends
  for (int i = 1; i <= 10; ++i) {
    void **p = (void **)(stack_1 + STACK_SIZE_FULL);
    *(p - i) = &ret_too_far;
    p = (void **)(stack_2 + STACK_SIZE_FULL);
    *(p - i) = &ret_too_far;
    p = (void **)(stack_3 + STACK_SIZE_FULL);
    *(p - i) = &ret_too_far;
    p = (void **)(stack_4 + STACK_SIZE_FULL);
    *(p - i) = &ret_too_far;
  }

  // Make getchar non I/O blocking (Q10)
  fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

#if TEST == TEST_INIT
  coroutine_t cr = init_coroutine(stack_1, STACK_SIZE, &infinite_loop_print);
  enter_coroutine(cr);

#elif TEST == TEST_SWITCH
  cr1 = init_coroutine(stack_1, STACK_SIZE, &func1);
  cr2 = init_coroutine(stack_2, STACK_SIZE, &func2);
  enter_coroutine(cr1);

#elif TEST == TEST_THREAD
  nb_threads = 3;
  struct thread thrs[3] = {thread_create(stack_2, &thread1),
                           thread_create(stack_3, &thread2),
                           thread_create(stack_4, &thread3)};
  threads = thrs;
  scheduler = init_coroutine(stack_1, STACK_SIZE, &schedule);
  enter_coroutine(scheduler);

#elif TEST == TEST_PRODUCER
  nb_threads = 3;
  struct thread thrs[3] = {thread_create(stack_2, &consumer_A),
                           thread_create(stack_3, &consumer_B),
                           thread_create(stack_4, &producer)};
  threads = thrs;
  scheduler = init_coroutine(stack_1, STACK_SIZE, &schedule);

  enter_coroutine(scheduler);

#endif
}