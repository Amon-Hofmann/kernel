# Answers

## Concept check

1. The extra Arguments are pushed to the stack right-to-left by the caller
   before the call instruction. va_start computes apionter to the first variadic
   argument by taking the address of the last named parameter and advancing past
   it. va_arg(args, T) reads a T-sized value from that pointer and advances the
   pointer by sizeof(t) rounded up to the stack alignment. -> when calling with
   long long the pointer would advance too far -> undefined behaviour.

2. INT32_MIN would break the naive implementation as it does not have a positive
   representation within int32. (Any negative value breaks it.)

3. 1 and 2 refer to the argument where to start checking for printf format. if
   our printf were to take a 'self'-pointer as first argument it would be 2 and
   3.. It catches mismatches between format specifiers and arguments.


## Mutation exercise

- A: a big positive number is printed instead of a small negative one.

- B: It does work, there is a big number printed for the string "abc" but there
    are no warnings or notes whatsoever.
