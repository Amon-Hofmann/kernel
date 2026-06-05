# Answers

## Concept check

1. As the stack grows downward. if the stack starts at x + 16384 and 5 long-words are pushed,
   %esp will go down to x + 16384 - 20.

   The looping back is kernel functionality and a stack overflow in kernel space will just overwrite 
   everything in the binary starting with COMMON and .data in this case.

2. changing the CHECKSUM to 0 did not change the behaviour at all actually. As it must evaluate
   to 0 anyway. This might become troublesome when the flags are used in the future. 
   Changing FLAGS to -MAGIC, GRUB rejects it and states: invalid flags.
   So if the three fields are summed this needs to be 0 (2^32 overflown to 0)
   magic + flags + checksum == 0

3. 
   - no builtins
   - main is not necessarily the entry point


## Mutation exercise

1. .skip 8 did not create an observable change in behaviour. The 4 bytes of return address
   when calling `kernel_main` did not overflow the stack of 8.
   8 Bytes are not going to be enough in the future as it just barely does not crash.
   there will be local variables and calls.
