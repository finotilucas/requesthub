CC             := gcc
CFLAGS_DEBUG   := -g -Wall -Wextra -Werror -Iinclude
CFLAGS_RELEASE := -O3 -Wall -Wextra -Werror -Iinclude -DNDEBUG
LDFLAGS        := -lcurl

SRC_DIR     := src
INC_DIR     := include
OBJ_DIR     := obj
BUILD_DIR   := build
RELEASE_DIR := $(BUILD_DIR)/release

SOURCES := $(shell find $(SRC_DIR) -name '*.c') main.c
OBJECTS := $(SOURCES:%.c=$(OBJ_DIR)/%.o)

TARGET_DEBUG  := $(BUILD_DIR)/main
TARGET_RELEASE := $(RELEASE_DIR)/main

all: debug

debug: CFLAGS := $(CFLAGS_DEBUG)
debug: $(TARGET_DEBUG)

$(TARGET_DEBUG): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	@echo "Linking executable (debug): $@"
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)


release: CFLAGS := $(CFLAGS_RELEASE)
release: $(TARGET_RELEASE)

$(TARGET_RELEASE): $(OBJECTS)
	@mkdir -p $(RELEASE_DIR)
	@echo "Linking executable (release): $@"
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)


$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling source: $<"
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all debug release clean run valgrind

clean:
	@echo "Cleaning workspace..."
	rm -rf $(OBJ_DIR) $(BUILD_DIR)

run: all
	./$(TARGET_DEBUG)

run-release: release
	./$(TARGET_RELEASE)

rebuild:
	$(MAKE) clean
	$(MAKE) release

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET_DEBUG)
