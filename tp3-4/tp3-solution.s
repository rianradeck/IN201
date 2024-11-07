/* ======== Question 2 ======== */
.global enter_coroutine /* Makes enter_coroutine visible to the linker */
enter_coroutine:
  mov %rdi,%rsp /* RDI contains the argument to enter_coroutine. */
                /* And is copied to RSP. */
  pop %r15
  pop %r14
  pop %r13
  pop %r12
  pop %rbx
  pop %rbp
  ret /* Pop the program counter */

/* ======== Question 5 ======== */
.global switch_coroutine /* Makes switch_coroutine visible to the linker */
switch_coroutine:
  push %rbp
  push %rbx
  push %r12
  push %r13
  push %r14
  push %r15
  mov %rsp,(%rdi) /* Store the stack pointer to *(first argument) */
  mov %rsi,%rdi
  jmp enter_coroutine /* Call enter_coroutine with the second argument */

/* Tell LD we don't need an executable stack here */
.section .note.GNU-stack,"",@progbits