# Answers

## Concept check

1. Otherwise the previously pressed key would be printed every time.
   The i8042 keeps irq1 asserted until port 0x60 is drained

2. 0x9E 
```C 
    0x80 = (0x9E & 0x80)
```

3. Caps applies to alphabetic keys only and only inverts the base state. caps &
   shift -> letters revert to lowercase but punctuation remains shifted (which
   would be wrong using an or statement). Pressing 'a' with caps activated and
   shift held, would stay at 'A' instead of going back to 'a'.

## Mutation exercise

- A: Not implemented by qemu

- B: output matched expectation as upper and lowercase letters were flipped
