# Answers

## Concept check

1. because the next instruction executes with paging active. The EIP must be
   covered by a valid PTE before this instruction runs. otherwise fetching the
   next opcode fails during address translation and results in a page fault.

2. it stays at 2^32 addresses

3. error code & 1 -> protection violation
   error code & 2 -> write
   error code & 4 -> user


## Mutation exercise

- A:

- B: 
