
# -------- Make options -------- #

SHELL := /usr/bin/bash
.SHELLFLAGS := -euo pipefail -c
.ONESHELL:
.DELETEONERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules


# -------- Project directory layout -------- #

HEADERDIR=include
HEADERS=$(wildcard $(HEADERDIR)/*.h)

CONFIGDIR = cfg

SOURCEDIR=src
SOURCES=$(wildcard $(SOURCEDIR)/*.c)

OBJDIR = lib
OBJFILES  = $(patsubst $(SOURCEDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

OUTDIR = out
main = main


# -------- Toolchain options -------- #

CC = gcc
#CC = clang

TESTOPTS =-DTESTING

CFLAGS = -Wall -Wextra -Werror -Wpedantic -Wno-nonnull-compare -Wno-empty-translation-unit
CFLAGS += -I $(HEADERDIR)
CDEBUG = -ggdb3 -O0 #-Og -ggdb 
CFLAGS += $(CDEBUG)
CPROFILING = -fprofile-abs-path -fprofile-arcs -ftest-coverage -pg -fprofile-generate # --coverage
#CFLAGS += $(CPROFILING)
COPTIMIZE = -O2
#CFLAGS += $(COPTIMIZE)
CFLAGS += $(TESTOPTS)
LIBS = 

OBJS = $(OBJFILES)

# -------- recipies -------- #

# ---- build ----
build : format  $(main) # easier to type


$(main) : objs
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBS)


new : clean build


# ---- compile ----

# all sourcecode
objs : tags $(OBJS)


# cfiles
$(OBJDIR)/%.o : $(SOURCEDIR)/%.c $(HEADERDIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

# cfiles without headerfile
$(OBJDIR)/%.o : $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


# ---- run ----
#
run : $(main)
	./$(main)

debug : $(main)
	gdb $(main)

# ---- misc ----
#
tags : $(SOURCES) $(HEADERS)
	ctags -R .

create_copile_commands : makefile
	bear -- make new

heap : $(main)
	valgrind --leak-check=full --show-leak-kinds=all ./$(main)

check : $(SOURCES) $(HEADERS)
	cppcheck --enable=all -I $(HEADERDIR) $(SOURCEDIR) --suppress=missingIncludeSystem

.PHONY : format
format : $(CONFIGDIR)/.clang-format $(SOURCES) $(HEADERS)
	clang-format -i -style=file:$(CONFIGDIR)/.clang-format $(SOURCES) $(HEADERS)

profile : $(main) run
	cp $(main) a.out
	gprof
	rm a.out

clean :
	rm -f $(OBJDIR)/*
	rm -f out/*
	rm -f $(main)
	rm -f a.out
	rm -f gmon.out
	rm -f .cache/clangd/index/*

