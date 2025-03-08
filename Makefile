# Makefile for Aether Project (Windows-friendly)

# Compiler variables
CC = gcc
AS = as
CFLAGS = -O2 -lpthread

# Files and paths
AE_COMPILER = aetherc.exe
AEC_SRC = aetherc.c
AE_SRC = src/main.ae
AE_GEN = build/main.c
ASM_SRC = asm/syscalls.s
ASM_OBJ = asm/syscalls.o
OUTPUT = aether_program.exe

.PHONY: all clean run

all: $(OUTPUT)

$(OUTPUT): $(AE_GEN) $(ASM_OBJ)
	$(CC) $(AE_GEN) $(ASM_OBJ) -o $(OUTPUT) $(CFLAGS)

$(AE_GEN): $(AE_SRC) $(AE_COMPILER)
	./$(AE_COMPILER) $(AE_SRC) $(AE_GEN)

$(ASM_OBJ): $(ASM_SRC)
	$(AS) --64 $(ASM_SRC) -o $(ASM_OBJ)

$(AE_COMPILER): $(AEC_SRC)
	$(CC) $(CFLAGS) $(AEC_SRC) -o $(AE_COMPILER)

clean:
	@echo Cleaning build files...
	del /Q build\main.c
	del /Q asm\syscalls.o
	del /Q $(OUTPUT)
	del /Q $(AE_COMPILER)

run: all
	./$(OUTPUT)
