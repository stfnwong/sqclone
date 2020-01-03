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

# Objects 
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
$(OBJECTS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# =============== PROGRAMS 
PROGRAMS=repl
PROGRAM_SOURCES := $(wildcard $(PROGRAM_DIR)/*.c)
PROGRAM_OBJECTS := $(PROGRAM_SOURCES:$(PROGRAM_DIR)/%.c=$(OBJ_DIR)/%.o)

$(PROGRAM_OBJECTS): $(OBJ_DIR)/%.o : $(PROGRAM_DIR)/%.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@ 

$(PROGRAMS): $(OBJECTS) $(PROGRAM_OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) $(OBJ_DIR)/$@.o\
		-o bin/$@ $(LIBS) $(TEST_LIBS)

# =============== BDD TESTS 
TESTS=table_spec
TEST_SOURCES=$(wildcard test/*.c)	
TEST_OBJECTS  := $(TEST_SOURCES:test/%.c=$(OBJ_DIR)/%.o)

$(TEST_OBJECTS): $(OBJ_DIR)/%.o : test/%.c 
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@ 

$(TESTS): $(TEST_OBJECTS) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(OBJ_DIR)/$@.o\
		-o bin/test/$@ $(LIBS) $(TEST_LIBS)


# ======== REAL TARGETS ========== #
.PHONY: clean

all : programs tests 

tests : $(OBJECTS) $(TESTS)
	
obj: $(OBJECTS)

programs: $(PROGRAMS)


clean:
	rm -rfv *.o $(OBJ_DIR)/*.o 
	rm -fv bin/test/test_*

print-%:
	@echo $* = $($*)
