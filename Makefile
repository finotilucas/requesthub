CC             := cc

GTK_CFLAGS     := $(shell pkg-config --cflags gtk4)
GTK_LIBS       := $(shell pkg-config --libs gtk4)

CJSON_CFLAGS := -I/usr/include/cjson
CJSON_LIBS   := -lcjson

SOURCEVIEW_CFLAGS := $(shell pkg-config --cflags gtksourceview-5)
SOURCEVIEW_LIBS   := $(shell pkg-config --libs gtksourceview-5)

COMMON_CFLAGS := -Wall -Wextra -Werror -Iinclude $(GTK_CFLAGS) $(CJSON_CFLAGS) $(SOURCEVIEW_CFLAGS)
LDFLAGS       := -fsanitize=address $(GTK_LIBS) -lcurl $(CJSON_LIBS) $(SOURCEVIEW_LIBS)

CFLAGS_DEBUG   := -g -fsanitize=address $(COMMON_CFLAGS)
CFLAGS_RELEASE := -O3 -DNDEBUG $(COMMON_CFLAGS)

SRC_DIR        := src
OBJ_DIR        := obj
BUILD_DIR      := build

SOURCES        := $(shell find $(SRC_DIR) -name '*.c') main.c
OBJECTS        := $(SOURCES:%.c=$(OBJ_DIR)/%.o)

TARGET_DEBUG   := $(BUILD_DIR)/main
TARGET_RELEASE := $(BUILD_DIR)/release/main

CFLAGS := $(CFLAGS_DEBUG)

all: debug

debug: CFLAGS := $(CFLAGS_DEBUG)
debug: $(TARGET_DEBUG)

release: CFLAGS := $(CFLAGS_RELEASE)
release: $(TARGET_RELEASE)

$(TARGET_DEBUG): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(TARGET_RELEASE): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)/release
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all debug release clean run run-release rebuild valgrind bear

clean:
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

bear:
	bear -- $(MAKE) clean all
