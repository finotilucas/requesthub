# --- Compiler and Tool Definitions ---
CC      := gcc
# -Iinclude: Tells the compiler to look for header files in the /include directory
CFLAGS  := -g -Wall -Wextra -Werror -Iinclude
LDFLAGS := -lcurl

# --- Directory Definitions ---
SRC_DIR   := src
INC_DIR   := include
OBJ_DIR   := obj
BUILD_DIR := build

# --- File Discovery ---
# Finds all .c files in src/ and subdirectories
SOURCES := $(shell find $(SRC_DIR) -name '*.c') main.c
# Maps .c files to .o files inside the obj/ directory
OBJECTS := $(SOURCES:%.c=$(OBJ_DIR)/%.o)

# The final binary name
TARGET := $(BUILD_DIR)/main

# --- Build Rules ---

# Default target
all: $(TARGET)

# Linking: Combines object files into the final executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	@echo "Linking executable: $@"
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compilation: Converts .c files into .o files
# The -c flag tells GCC to compile but not link
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling source: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# --- Utility Targets ---

# Change this line
.PHONY: clean run valgrin

# Removes all generated build and object files
clean:
	@echo "Cleaning workspace..."
	rm -rf $(OBJ_DIR) $(BUILD_DIR)

# Compiles and runs the program
run: all
	./$(TARGET)

# Runs memory leak detection
valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)
