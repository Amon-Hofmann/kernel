
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


# -------- Directories -------- #

SRCDIR  := src
ASDIR   := as
INCDIR  := include
OBJDIR  := lib
OUTDIR  := out
ISODIR  := $(OUTDIR)/iso/boot
CFGDIR  := cfg

CSRCS   := $(wildcard $(SRCDIR)/*.c)
ASSRCS  := $(wildcard $(ASDIR)/*.s)
HEADERS := $(wildcard $(INCDIR)/*.h)
COBJS   := $(patsubst $(SRCDIR)/%.c,  $(OBJDIR)/%.o, $(CSRCS))
ASOBJS  := $(patsubst $(ASDIR)/%.s,   $(OBJDIR)/%.o, $(ASSRCS))
OBJS    := $(ASOBJS) $(COBJS)

KERNEL  := $(OUTDIR)/kernel.elf
ISO     := $(OUTDIR)/kernel.iso


# --------- Flags --------- #

GCC_INCLUDES := $(shell $(CC) -print-file-name=include)
CFLAGS  := -std=gnu99 -ffreestanding -Og -ggdb3 -Wall -Wextra -Werror \
           -nostdinc -isystem $(GCC_INCLUDES) -I$(INCDIR)
LDFLAGS := -T cfg/kernel.ld -ffreestanding -Og -nostdlib -lgcc


# -------- Tutorial PDFs -------- #

TUTORIALDIR := tutorial
MDFILES     := $(wildcard $(TUTORIALDIR)/*.md)
PDFFILES    := $(MDFILES:.md=.pdf)

$(TUTORIALDIR)/%.pdf: $(TUTORIALDIR)/%.md
	pandoc $< -o $@

# -------- Targets -------- #
# -------- main Targets -------- #

.PHONY: all iso run debug clean pdf format

all: $(KERNEL)

new: clean $(KERNEL)

$(KERNEL): format $(OBJS) cfg/kernel.ld
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
	#qemu-system-i386 -cdrom $(ISO) -serial stdio -no-reboot
	qemu-system-i386 -cdrom $(ISO) -serial stdio -no-reboot -enable-kvm -d int,cpu_reset -D /tmp/qemu.log

debug: iso
	#qemu-system-i386 -cdrom $(ISO) -serial stdio -no-reboot  -s -S  &
	qemu-system-i386 -cdrom $(ISO) -serial stdio -no-reboot -enable-kvm -s -S -d int,cpu_reset -D /tmp/qemu.log &
	gdb $(KERNEL) -ex "target remote :1234"


# -------- helper Targets -------- #

format : $(CFGDIR)/.clang-format $(CSRCS) $(HEADERS)
	clang-format -i -style=file:$(CFGDIR)/.clang-format $(CSRCS) $(HEADERS)

create_copile_commands : makefile
	bear -- make new



pdf: $(PDFFILES)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(OUTDIR)/kernel.elf $(OUTDIR)/kernel.iso
	rm -rf $(OUTDIR)/iso
