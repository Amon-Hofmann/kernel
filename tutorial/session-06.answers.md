# Answers

## Concept check

1. The compiler might optimize the read/writes inside the spin-loop away, making
   it an infinite loop.

2. Each tick introduces a small error, so we can compensate it.

3. The comparison can wrap around and then be wrong. the substraction
   compensates that behaviour.

## Mutation exercise

A: with the command-byte set to 0x30 only one initial tick is observed.
   Mode 0 does not auto reload. it fires once and stops.

B: It still works with the volatile stripped of pit_ticks and -O2.


