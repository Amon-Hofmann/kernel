# Answers

## Concept check

1. ljmp implicitly flushes the 'shadow register'. The cpu caches the values of the 
   segment registers for speed in the 'shadow registers'.

2. The null descriptor states that no descriptor is loaded. Trying to access anything with
   the null descriptor loaded will result in a General Protection Fault.

3. CPL reflects the RPL value of the loaded executing code. The DPL is checked against
   max(CPL, RPL).

## Mutation exercise

1. After verifying that the segment was loaded into %ds and we actually wrote to a memory 
   address > 0, it is to say that apparently qemu does not enforce segment limit checks
   for ring-0 code. So I cannot verify that it is actually a INT 13 (GPE).
   After running it with -enable-kvm -s -S -d int,cpu_reset -D /tmp/qemu.log
   I can see it exiting (faulting) when trying to write to the faulty_address. not before.
   If it occured when loading the descriptor, one could never load a 0-length descriptor.
   This occurs when translating the write address. It is then checked, so offset < base +
   limit>.


