# Answers

## Concept check

1. some of the interrupts do not have a corresponding error-code so they did not push them. For stack uniformity the stubs
   for the interrupts not pushing a code, push 0 as a dummy value.

2. Interrupts are disabled for the duration of the interrupt-exception-handling. For Traps it stays enabled.
   To prevent nested interrupts, we use interrupt gates.

3. iret restores %eip, %cs and %eflags as they were pushed to the stack earlier. ret pops only %eip.

## Mutation exercise

- A: eip points to 0x001005B7, somewhere in the kernel-code address space. The error-code is 0x00000000.

- B: EFLAGS does not differ as we have not called sti so far anyways.
