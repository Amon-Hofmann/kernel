
# -------- Make options -------- #

SHELL := /usr/bin/bash
.SHELLFLAGS := -euo pipefail -c
.ONESHELL:
.DELETEONERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules


# -------- Toolchain -------- #

CROSS   := $(HOME)/cross/opt/bin
CC      := $(CROSS)/i686-elf-gcc
AS      := $(CROSS)/i686-elf-as
LD      := $(CROSS)/i686-elf-ld
OBJDUMP := $(CROSS)/i686-elf-objdump

CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude
LDFLAGS := -T cfg/kernel.ld -ffreestanding -O2 -nostdlib -lgcc


# -------- Directories -------- #

SRCDIR  := src
ASDIR   := as
INCDIR  := include
OBJDIR  := lib
OUTDIR  := out
ISODIR  := $(OUTDIR)/iso/boot

CSRCS   := $(wildcard $(SRCDIR)/*.c)
ASSRCS  := $(wildcard $(ASDIR)/*.s)
COBJS   := $(patsubst $(SRCDIR)/%.c,  $(OBJDIR)/%.o, $(CSRCS))
ASOBJS  := $(patsubst $(ASDIR)/%.s,   $(OBJDIR)/%.o, $(ASSRCS))
OBJS    := $(ASOBJS) $(COBJS)

KERNEL  := $(OUTDIR)/kernel.elf
ISO     := $(OUTDIR)/kernel.iso


# -------- Targets -------- #

.PHONY: all iso run debug clean

all: $(KERNEL)

$(KERNEL): $(OBJS) cfg/kernel.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: $(ASDIR)/%.s
	$(AS) $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

iso: $(KERNEL)
	mkdir -p $(ISODIR)/grub
	cp $(KERNEL) $(ISODIR)/
	cp cfg/grub.cfg $(ISODIR)/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(OUTDIR)/iso 2>/dev/null

run: iso
	qemu-system-i386 -cdrom $(ISO) -serial stdio -no-reboot

debug: iso
	qemu-system-i386 -cdrom $(ISO) -serial stdio -no-reboot -s -S &
	gdb $(KERNEL) -ex "target remote :1234"

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(OUTDIR)/kernel.elf $(OUTDIR)/kernel.iso
	rm -rf $(OUTDIR)/iso
