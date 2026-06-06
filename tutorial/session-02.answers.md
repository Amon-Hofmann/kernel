# Answers

## Concept check

1. Because the VGA Controller maps its video buffer to the physical address 0xB8000 via
   MemoryMappedIO. With paging not configured 0xB8000 addresses the physical address in
   memory.

2. Given that we only ever just write to the array at 0xB8000 any writes preceeding the
   last write before a read would be obsolete and the compiler will just throw them out or
   reorder them.

3. I managed to go without strlen. It will be implemented eventually any way. 
   -fno-tree-loop-distribute-patterns would be the flag you're looking for. In case gcc
   emits calls to memcpy, memset or memmove without them being present the linker will
   fail to link the objects to a binary and stop.
   Ive implemented them in string.c so there is no need to worry about whether gcc generates
   calls or not.

## Mutation exercise

1. see vgaterm.c
