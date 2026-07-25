# Answers

## Concept check

1.  Starting from "all used" is the safe choice: any gap in the memory map (a region
    not listed at all) stays reserved. Starting from "all free" would require knowing
    and enumerating every reserved region — which cannot be exhaustively.

2.  Frame N = 1000 is at address 1000 * 4096. in Word N / 32 = 31.25 = 31. Is at
    bit N % 32 = 8.

3.  the uint32_t will overflow and this will result in the intended (address &
    uint32MAX) in the first MiB being marked as available.


## Mutation exercise

- A: the first allocated frame starts directly at 1Mib
     Any write to these frames corrupts the running kernel image.

- B: This time the allocate frames start at address 0. allocating 64 pages
    starting from address 0 ranges until 0x0003F000. By that i covers many
    standard (OSDevWiki) regions IVT, BDA. BootSector. Writing to any of 
    these regions is a catastrophic error resulting in system state corruption.
