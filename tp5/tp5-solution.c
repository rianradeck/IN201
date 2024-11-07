#include <signal.h>
#include <stddef.h>
#include <stdio.h>  // getchar
#include <stdlib.h> // exit
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <asm/signal.h>

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

// print a integer in given base (10 or 16) using putchar
void print_int(long long int x, int base) {
  if (x < 0) {
    putchar('-');
    x = -x;
  }
  if (x == 0) {
    putchar('0');
    return;
  }
  int pos = base;
  while (x >= pos) {
    pos *= base;
  }
  while (pos > 1) {
    pos /= base;
    int digit = (x / pos) % base;
    if (base > 10 && digit >= 10)
      putchar('a' + digit - 10);
    else
      putchar('0' + digit);
  }
}

// The size of our stacks
// Sometimes the signal handler overflows the stacks, so I've doubled theirs
// sizes to avoid this problem.
#define STACK_SIZE_FULL (2 * 4096)

// We will pad the bottom of our stacks with a pointers to dummy function for
// two reasons:
// - It helps detect if our assembly is incorrect (ret is still valid and prints
//   a clearer message then segfault
// - printf sometimes look deep into the stack for variadic arguments, causing
//   segfaults if the stack is too short
#define BOTTOM_PADDING 10

// The size of our stack, minus bottom padding
#define STACK_SIZE (STACK_SIZE_FULL - BOTTOM_PADDING * sizeof(void *))

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

// ==========================================
// ============ TP3 - Question 1 ============
// ============ TP5 - Question 2 ============
// ==========================================

char stack_1[STACK_SIZE_FULL] __attribute__((aligned(STACK_SIZE_FULL)));
char stack_2[STACK_SIZE_FULL] __attribute__((aligned(STACK_SIZE_FULL)));
char stack_3[STACK_SIZE_FULL] __attribute__((aligned(STACK_SIZE_FULL)));
char stack_4[STACK_SIZE_FULL] __attribute__((aligned(STACK_SIZE_FULL)));

// Simple guard function
// We write the pointer of this function at the top of our stack (at the start
// of main), This way, if we accidentaly pop one too many value, we get a
// clearer error message then a segfault.
void ret_too_far() {
  print_str("Error: Out of stack!\n");
  exit(5);
}

// ==========================================
// ============ TP3 - Question 3 ============
// ==========================================

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

// ==========================================
// ============ TP3 - Question 4 ============
// ==========================================

void infinite_loop_print() {
loop:
  print_str("Looping...\n");
  goto loop;
}

// ==========================================
// ============ TP3 - Question 6 ============
// ==========================================

// Store both coroutines as global variables, so we can switch as appropriate
coroutine_t cr1, cr2;

void func1() {
  int x = 0;
  for (;;) {
    ++x;
    print_str("In func1, x = ");
    print_int(x, 10);
    print_str("\n");
    switch_coroutine(&cr1, cr2);
  }
}

void func2() {
  int z = 0;
  for (;;) {
    ++z;
    print_str("In func2, x = ");
    print_int(z, 10);
    print_str("\n");
    switch_coroutine(&cr2, cr1);
  }
}

// ==========================================
// ============ TP3 - Question 7 ============
// ==========================================

enum status {
  READY,
  BLOCKED,
};

struct thread {
  coroutine_t couroutine;
  enum status status;
};

// ==========================================
// ============ TP3 - Question 8 ============
// ==========================================

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

// ==========================================
// ============ TP5 - Question 4 ============
// ==========================================

// Find the base of the current stack
void *find_stack(char *p) {
  return (void *)((int64_t)(p) & ~(STACK_SIZE_FULL - 1));
}

void schedule() {
  size_t nb = 0;
  for (;;) {
    struct thread t = threads[nb];
    if (t.status == READY) {
      current_thread = t.couroutine;
      // Find the base of the current stack to set protections
      void *stack = find_stack(current_thread);
      // print_str("In stack: ");
      // print_int((size_t)stack, 16);
      // putchar('\n');
      mprotect(stack, STACK_SIZE_FULL, PROT_READ | PROT_WRITE | PROT_EXEC);
      switch_coroutine(&scheduler, t.couroutine);
      mprotect(stack, STACK_SIZE_FULL, PROT_NONE);
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
    print_int(counter, 10);
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
  print_str("In thread 3\n");
  goto loop;
}

// ==========================================
// ============ TP3 - Partie 4 ==============
// ==========================================

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
        // ==========================================
        // ============ TP5 - Question 5 ============
        // ==========================================
        if (read_char == 'e') {
          stack_2[125] = 23;
        }
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

// ==========================================
// ============ TP5 - Question 6 ============
// ==========================================

void signal_handler(int x) {
  // Reinstall signal handler when done
  sigset_t set;
  sigfillset(&set);
  sigprocmask(SIG_UNBLOCK, &set, 0);

  // Print error message
  int use_color = isatty(STDOUT_FILENO);
  if (use_color)
    // error message in red using ANSI escape codes
    print_str("\033[31m");
  print_str("Caught signal: ");
  print_int(x, 10);

  void *stack = find_stack(current_thread);
  print_str(" from stack 0x");
  print_int((size_t)stack, 16);
  putchar('\n');
  if (use_color)
    // reset terminal color
    print_str("\033[0m");

  // ==========================================
  // ============ TP5 - Question 7 ============
  // ==========================================

  // This works because we know statically which thread is in which stack
  // A true kernel will have a dynamic data-structure to store that information.
  if (stack == stack_2) {
    print_str("This is stack_2, which runs consumer_A\n");
    threads[0] = thread_create(stack_2, &consumer_A);
    current_thread = threads[0].couroutine;
  } else if (stack == stack_3) {
    print_str("This is stack_3, which runs consumer_B\n");
    threads[1] = thread_create(stack_3, &consumer_B);
    current_thread = threads[1].couroutine;
  } else if (stack == stack_4) {
    print_str("This is stack_4, which runs producer\n");
    threads[2] = thread_create(stack_4, &producer);
    current_thread = threads[2].couroutine;
  } else {
    print_str("This is an unknown stack, exiting\n");
    exit(8);
  }
  print_str("Thread restarted\n");
  enter_coroutine(scheduler);
}

// ====================================
// ============== Main ================
// ====================================

#define TEST_INIT 0
#define TEST_SWITCH 1
#define TEST_THREAD 2
#define TEST_PRODUCER 3

// Change TEST to run the different parts of the TP
#define TEST TEST_PRODUCER

int main() {
  // Optionally add guards to stack ends
  for (int i = 1; i <= BOTTOM_PADDING; ++i) {
    void **p = (void **)(stack_1 + STACK_SIZE_FULL);
    *(p - i) = &ret_too_far;
    p = (void **)(stack_2 + STACK_SIZE_FULL);
    *(p - i) = &ret_too_far;
    p = (void **)(stack_3 + STACK_SIZE_FULL);
    *(p - i) = &ret_too_far;
    p = (void **)(stack_4 + STACK_SIZE_FULL);
    *(p - i) = &ret_too_far;
  }

  // ==========================================
  // ============ TP5 - Question 1 ============
  // ==========================================

  if ((int64_t)(stack_1) % STACK_SIZE_FULL) {
    printf("Stack 1 missaligned\n");
    exit(3);
  }
  // Equivalent check, avoiding division
  if (((int64_t)(stack_2) & (STACK_SIZE_FULL - 1))) {
    printf("Stack 2 missaligned\n");
    exit(3);
  }
  if ((int64_t)(stack_3) % STACK_SIZE_FULL) {
    printf("Stack 3 missaligned\n");
    exit(3);
  }
  if ((int64_t)(stack_4) % STACK_SIZE_FULL) {
    printf("Stack 4 missaligned\n");
    exit(3);
  }

  // alternative version, with a loop and a table
  char *stacks[] = {stack_1, stack_2, stack_3, stack_4};
  for (u_int32_t i = 0; i < 4; ++i) {
    if ((int64_t)(stacks[i]) & (STACK_SIZE_FULL - 1)) {
      printf("Stack %d missaligned\n", i + 1);
      exit(3);
    }
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

  // printf("%lx, %lx, %lx\n", (size_t)stack_1, (size_t)stack_2,
  // (size_t)stack_3);

  // ==========================================
  // ============ TP5 - Question 3 ============
  // ==========================================

  mprotect(stack_2, STACK_SIZE_FULL, PROT_NONE);
  mprotect(stack_3, STACK_SIZE_FULL, PROT_NONE);
  mprotect(stack_4, STACK_SIZE_FULL, PROT_NONE);

  signal(SIGSEGV, &signal_handler);
  // sleep(4);

  enter_coroutine(scheduler);

#endif
}