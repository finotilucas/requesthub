UNAME_S := $(shell uname -s)

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    WINDOWS    := 1
    CC         := gcc
    EXE        := .exe
    SANITIZE   :=
else
    WINDOWS    := 0
    CC         := cc
    EXE        :=
    SANITIZE   := -fsanitize=address
endif

GTK_CFLAGS        := $(shell pkg-config --cflags gtk4)
GTK_LIBS          := $(shell pkg-config --libs gtk4)

SOURCEVIEW_CFLAGS := $(shell pkg-config --cflags gtksourceview-5)
SOURCEVIEW_LIBS   := $(shell pkg-config --libs gtksourceview-5)

ifeq ($(WINDOWS),1)
    CJSON_CFLAGS := $(shell pkg-config --cflags libcjson)
    CJSON_LIBS   := $(shell pkg-config --libs libcjson)
else
    CJSON_CFLAGS := -I/usr/include/cjson
    CJSON_LIBS   := -lcjson
endif

COMMON_CFLAGS := -Wall -Wextra -Werror -Iinclude \
                 $(GTK_CFLAGS) \
                 $(SOURCEVIEW_CFLAGS) \
                 $(CJSON_CFLAGS)

CFLAGS_DEBUG   := -g $(SANITIZE) $(COMMON_CFLAGS)
CFLAGS_RELEASE := -O3 -DNDEBUG $(COMMON_CFLAGS)

LDFLAGS_COMMON := $(GTK_LIBS) $(SOURCEVIEW_LIBS) $(CJSON_LIBS) -lcurl
LDFLAGS_DEBUG  := $(SANITIZE) $(LDFLAGS_COMMON)
LDFLAGS_RELEASE := $(LDFLAGS_COMMON)

SRC_DIR         := src
OBJ_DIR         := obj
BUILD_DIR       := build

SOURCES         := $(shell find $(SRC_DIR) -name '*.c') main.c

DEBUG_OBJ_DIR   := $(OBJ_DIR)/debug
RELEASE_OBJ_DIR := $(OBJ_DIR)/release

DEBUG_OBJECTS   := $(SOURCES:%.c=$(DEBUG_OBJ_DIR)/%.o)
RELEASE_OBJECTS := $(SOURCES:%.c=$(RELEASE_OBJ_DIR)/%.o)

TARGET_DEBUG    := $(BUILD_DIR)/main$(EXE)
TARGET_RELEASE  := $(BUILD_DIR)/release/main$(EXE)

all: debug

debug: $(TARGET_DEBUG)

release: $(TARGET_RELEASE)

$(TARGET_DEBUG): $(DEBUG_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $@ $(LDFLAGS_DEBUG)

$(TARGET_RELEASE): $(RELEASE_OBJECTS)
	@mkdir -p $(BUILD_DIR)/release
	$(CC) $^ -o $@ $(LDFLAGS_RELEASE)

$(DEBUG_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling (debug):   $<"
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

$(RELEASE_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling (release): $<"
	$(CC) $(CFLAGS_RELEASE) -c $< -o $@

.PHONY: all debug release clean run run-release rebuild

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR)

run: debug
	./$(TARGET_DEBUG)

run-release: release
	./$(TARGET_RELEASE)

rebuild:
	$(MAKE) clean
	$(MAKE) release

ifneq ($(WINDOWS),1)
.PHONY: valgrind bear

valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET_DEBUG)

bear:
	bear -- $(MAKE) clean all
endif