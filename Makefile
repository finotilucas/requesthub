UNAME_S := $(shell uname -s)
ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    WINDOWS    := 1
    CC         := gcc
    EXE        := .exe
    SANITIZE   :=
else
    WINDOWS    := 0
    CC         := gcc
    EXE        :=
    SANITIZE   := -fsanitize=address -fsanitize=undefined
endif

CJSON_PKG_CFLAGS := $(shell pkg-config --cflags libcjson 2>/dev/null || true)
CJSON_PKG_LIBS   := $(shell pkg-config --libs   libcjson 2>/dev/null || true)
ifeq ($(CJSON_PKG_CFLAGS),)
    ifeq ($(WINDOWS),1)
        CJSON_CFLAGS :=
        CJSON_LIBS   := -lcjson
    else
        CJSON_CFLAGS := -I/usr/include/cjson
        CJSON_LIBS   := -lcjson
    endif
else
    CJSON_CFLAGS := $(CJSON_PKG_CFLAGS)
    CJSON_LIBS   := $(CJSON_PKG_LIBS)
endif

GTK_CFLAGS        := $(shell pkg-config --cflags gtk4)
GTK_LIBS          := $(shell pkg-config --libs gtk4)
SOURCEVIEW_CFLAGS := $(shell pkg-config --cflags gtksourceview-5)
SOURCEVIEW_LIBS   := $(shell pkg-config --libs gtksourceview-5)

LIBXML2_CFLAGS    := $(shell pkg-config --cflags libxml-2.0 2>/dev/null || echo "-I/usr/include/libxml2")
LIBXML2_LIBS      := $(shell pkg-config --libs libxml-2.0 2>/dev/null || echo "-lxml2")

YAML_CFLAGS       := $(shell pkg-config --cflags yaml-0.1 2>/dev/null || true)
YAML_LIBS         := $(shell pkg-config --libs yaml-0.1 2>/dev/null || echo "-lyaml")

COMMON_CFLAGS := -Wall -Wextra -Werror -std=gnu11 -Iinclude \
                 $(GTK_CFLAGS) \
                 $(SOURCEVIEW_CFLAGS) \
                 $(CJSON_CFLAGS) \
                 $(LIBXML2_CFLAGS) \
                 $(YAML_CFLAGS)

CFLAGS_DEBUG   := -g $(SANITIZE) $(COMMON_CFLAGS)
CFLAGS_RELEASE := -O3 -DNDEBUG -march=native -flto -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables $(COMMON_CFLAGS)

LDFLAGS_COMMON := $(GTK_LIBS) \
                  $(SOURCEVIEW_LIBS) \
                  $(CJSON_LIBS) \
                  $(LIBXML2_LIBS) \
                  $(YAML_LIBS) \
                  -lcurl -lm

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

TARGET_DEBUG    := $(BUILD_DIR)/requesthub$(EXE)
TARGET_RELEASE  := $(BUILD_DIR)/release/requesthub$(EXE)

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

.PHONY: all debug release clean run run-release rebuild deps-check

deps-check:
	@echo "Checking dependencies..."
	@pkg-config --exists gtk4 || (echo "GTK4 not found" && exit 1)
	@pkg-config --exists gtksourceview-5 || (echo "GtkSourceView-5 not found" && exit 1)
	@pkg-config --exists libxml-2.0 || (echo "libxml2 not found" && exit 1)
	@pkg-config --exists yaml-0.1 || (echo "libyaml not found via pkg-config (trying fallback)")
	@echo "Main dependencies OK"

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
.PHONY: valgrind bear compile_commands

valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET_DEBUG)

bear:
	bear -- $(MAKE) clean all

compile_commands:
	bear -- $(MAKE) clean debug
endif
