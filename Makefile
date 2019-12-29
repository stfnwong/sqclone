# SQLITE CLONE
# Makefile for an sqlit3 clone
# 
# Stefan Wong 2019

# Compilation options 
DEBUG=1

# DIRECTORIES 
SRC_DIR=./src
OBJ_DIR=./obj
BIN_DIR=./bin
TEST_DIR=./test 
TEST_BIN_DIR=bin/test	# TODO : directory expansion issue?
PROGRAM_DIR=./programs
##TEST_BIN_DIR=$(BIN_DIR)/test

ASM_STYLE=intel

# TOOLS 
CC=gcc
ifeq ($(DEBUG), 1)
OPT=-O0 -g2
endif 
CFLAGS = -Wall -std=c99 -D_REENTRANT -pthread $(OPT)
CFLAGS += -D_GNU_SOURCE
LDFLAGS=
LIBS=
TEST_LIBS=-lcheck

INCS=-I$(SRC_DIR)
# Sources
SOURCES=$(wildcard $(SRC_DIR)/*.c)
# Unit test sources 
TEST_SOURCES=$(wildcard $(TEST_DIR)/*.c)	# issue here with directory name..

# Objects 
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
$(OBJECTS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Target to generate assembler output
#$(ASSEM_OBJECTS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
#	$(CC) $(CFLAGS) -S $< -o $(OBJ_DIR)/$@.asm -masm=$(ASM_STYLE)

# =============== PROGRAMS 
PROGRAM_SOURCES = $(wildcard $(PROGRAM_DIR)/*.c)
PROGRAM_OBJECTS := $(PROGRAM_SOURCES:$(PROGRAM_DIR)/%.c=$(OBJ_DIR)/%.o)

$(PROGRAM_OBJECTS): $(OBJ_DIR)/%.o : $(PROGRAM_DIR)/%.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@ 

$(PROGRAMS): $(PROGRAM_OBJECTS) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(OBJ_DIR)/$@.o\
		-o bin/$@ $(LIBS) $(TEST_LIBS)

PROGRAMS = repl

# =============== TESTS 
TEST_OBJECTS  := $(TEST_SOURCES:$(TEST_DIR)/%.c=$(OBJ_DIR)/%.o)
$(TEST_OBJECTS): $(OBJ_DIR)/%.o : $(TEST_DIR)/%.c 
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@ 

# Unit test targets 
TESTS = 

$(TESTS): $(TEST_OBJECTS) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(OBJ_DIR)/$@.o\
		-o bin/test/$@ $(LIBS) $(TEST_LIBS)


# ======== REAL TARGETS ========== #
.PHONY: clean

all : program test 

test : $(OBJECTS) $(TESTS)
	
obj: $(OBJECTS)

program: $(OBJECTS) $(PROGRAMS)


clean:
	rm -rfv *.o $(OBJ_DIR)/*.o 
	rm -fv bin/test/test_*

print-%:
	@echo $* = $($*)
