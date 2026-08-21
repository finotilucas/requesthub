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
    SANITIZE_PROBE := $(shell tmp=$$(mktemp 2>/dev/null) && \
        echo 'int main(void){return 0;}' | $(CC) -fsanitize=address -fsanitize=undefined -xc - -o $$tmp 2>/dev/null && \
        echo 1 || echo 0; rm -f $$tmp)
    ifeq ($(SANITIZE_PROBE),1)
        SANITIZE := -fsanitize=address -fsanitize=undefined
    else
        SANITIZE :=
    endif
endif

GTK_CFLAGS        := $(shell pkg-config --cflags gtk4)
GTK_LIBS          := $(shell pkg-config --libs gtk4)
ADW_CFLAGS        := $(shell pkg-config --cflags libadwaita-1)
ADW_LIBS          := $(shell pkg-config --libs libadwaita-1)
SOURCEVIEW_CFLAGS := $(shell pkg-config --cflags gtksourceview-5)
SOURCEVIEW_LIBS   := $(shell pkg-config --libs gtksourceview-5)

LIBXML2_CFLAGS    := $(shell pkg-config --cflags libxml-2.0 2>/dev/null || echo "-I/usr/include/libxml2")
LIBXML2_LIBS      := $(shell pkg-config --libs libxml-2.0 2>/dev/null || echo "-lxml2")

YAML_CFLAGS       := $(shell pkg-config --cflags yaml-0.1 2>/dev/null || true)
YAML_LIBS         := $(shell pkg-config --libs yaml-0.1 2>/dev/null || echo "-lyaml")

CJSON_CFLAGS      := $(shell pkg-config --cflags libcjson 2>/dev/null || true)
CJSON_LIBS        := $(shell pkg-config --libs libcjson 2>/dev/null || echo "-lcjson")

COMMON_CFLAGS := -Wall -Wextra -Werror -std=gnu11 -Iinclude \
                 $(GTK_CFLAGS) \
                 $(ADW_CFLAGS) \
                 $(SOURCEVIEW_CFLAGS) \
                 $(CJSON_CFLAGS) \
                 $(LIBXML2_CFLAGS) \
                 $(YAML_CFLAGS)

RELEASE_MARCH ?= native
RELEASE_MTUNE ?= native

CFLAGS_DEBUG   := -g $(SANITIZE) $(COMMON_CFLAGS)
CFLAGS_RELEASE := -O2 -DNDEBUG -march=$(RELEASE_MARCH) -mtune=$(RELEASE_MTUNE) $(COMMON_CFLAGS)

ifeq ($(WINDOWS),1)
    EXTRA_GIO_LIBS :=
else
    EXTRA_GIO_LIBS := -lgmodule-2.0 -lmount
endif

LDFLAGS_COMMON := $(GTK_LIBS) \
                  $(ADW_LIBS) \
                  $(SOURCEVIEW_LIBS) \
                  $(CJSON_LIBS) \
                  $(LIBXML2_LIBS) \
                  $(YAML_LIBS) \
                  $(EXTRA_GIO_LIBS) \
                  -lcurl -lm

LDFLAGS_DEBUG  := $(SANITIZE) $(LDFLAGS_COMMON)
LDFLAGS_RELEASE := $(LDFLAGS_COMMON)

SRC_DIR         := src
OBJ_DIR         := obj
BUILD_DIR       := build

GRESOURCE_XML   := $(SRC_DIR)/ui/styles/styles.gresource.xml
GRESOURCE_C     := $(SRC_DIR)/ui/styles/styles.gresource.c
GRESOURCE_DEPS  := $(shell glib-compile-resources --sourcedir=$(SRC_DIR)/ui/styles --generate-dependencies $(GRESOURCE_XML) 2>/dev/null)

SOURCES         := $(filter-out $(GRESOURCE_C),$(shell find $(SRC_DIR) -name '*.c')) $(GRESOURCE_C) main.c

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

$(GRESOURCE_C): $(GRESOURCE_XML) $(GRESOURCE_DEPS)
	@echo "Generating GResource: $@"
	glib-compile-resources --target=$@ --generate-source --sourcedir=$(SRC_DIR)/ui/styles $(GRESOURCE_XML)

GLIB_TEST_CFLAGS := $(shell pkg-config --cflags glib-2.0)
GLIB_TEST_LIBS   := $(shell pkg-config --libs   glib-2.0)
CURL_TEST_CFLAGS := $(shell pkg-config --cflags libcurl 2>/dev/null)
CURL_TEST_LIBS   := $(shell pkg-config --libs   libcurl 2>/dev/null || echo "-lcurl")

TEST_DIR        := tests
TEST_OBJ_DIR    := $(OBJ_DIR)/test
TEST_BIN_DIR    := $(BUILD_DIR)/test

TEST_HISTORY_TARGET     := $(TEST_BIN_DIR)/test_history$(EXE)
TEST_HISTORY_SOURCES    := $(TEST_DIR)/test_history.c \
                           $(SRC_DIR)/history/history.c \
                           $(SRC_DIR)/history/history_persistence.c
TEST_HISTORY_OBJECTS    := $(TEST_HISTORY_SOURCES:%.c=$(TEST_OBJ_DIR)/%.o)

TEST_REQUEST_TARGET     := $(TEST_BIN_DIR)/test_request$(EXE)
TEST_REQUEST_SOURCES    := $(TEST_DIR)/test_request.c \
                           $(SRC_DIR)/http/request.c \
                           $(SRC_DIR)/http/methods.c
TEST_REQUEST_OBJECTS    := $(TEST_REQUEST_SOURCES:%.c=$(TEST_OBJ_DIR)/%.o)

TEST_RESPONSE_TARGET    := $(TEST_BIN_DIR)/test_response$(EXE)
TEST_RESPONSE_SOURCES   := $(TEST_DIR)/test_response.c \
                           $(SRC_DIR)/http/response.c
TEST_RESPONSE_OBJECTS   := $(TEST_RESPONSE_SOURCES:%.c=$(TEST_OBJ_DIR)/%.o)

TEST_HTTP_POOL_TARGET   := $(TEST_BIN_DIR)/test_http_pool$(EXE)
TEST_HTTP_POOL_SOURCES  := $(TEST_DIR)/test_http_pool.c \
                           $(SRC_DIR)/http/http_pool.c
TEST_HTTP_POOL_OBJECTS  := $(TEST_HTTP_POOL_SOURCES:%.c=$(TEST_OBJ_DIR)/%.o)

TEST_HTTP_PERFORM_TARGET  := $(TEST_BIN_DIR)/test_http_perform$(EXE)
TEST_HTTP_PERFORM_SOURCES := $(TEST_DIR)/test_http_perform.c \
                             $(SRC_DIR)/http/http.c \
                             $(SRC_DIR)/http/http_pool.c \
                             $(SRC_DIR)/http/methods.c \
                             $(SRC_DIR)/http/request.c \
                             $(SRC_DIR)/http/response.c
TEST_HTTP_PERFORM_OBJECTS := $(TEST_HTTP_PERFORM_SOURCES:%.c=$(TEST_OBJ_DIR)/%.o)

TEST_TARGETS    := $(TEST_HISTORY_TARGET) \
                   $(TEST_REQUEST_TARGET) \
                   $(TEST_RESPONSE_TARGET) \
                   $(TEST_HTTP_POOL_TARGET) \
                   $(TEST_HTTP_PERFORM_TARGET)

TEST_CFLAGS     := -g $(SANITIZE) -Wall -Wextra -Werror -std=gnu11 \
                   $(GLIB_TEST_CFLAGS) $(CJSON_CFLAGS) $(CURL_TEST_CFLAGS)

TEST_LDFLAGS    := $(SANITIZE) $(GLIB_TEST_LIBS) $(CJSON_LIBS) \
                   $(CURL_TEST_LIBS) -lpthread

$(TEST_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling (test):    $<"
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(TEST_HISTORY_TARGET): $(TEST_HISTORY_OBJECTS)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $^ -o $@ $(TEST_LDFLAGS)

$(TEST_REQUEST_TARGET): $(TEST_REQUEST_OBJECTS)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $^ -o $@ $(TEST_LDFLAGS)

$(TEST_RESPONSE_TARGET): $(TEST_RESPONSE_OBJECTS)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $^ -o $@ $(TEST_LDFLAGS)

$(TEST_HTTP_POOL_TARGET): $(TEST_HTTP_POOL_OBJECTS)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $^ -o $@ $(TEST_LDFLAGS)

$(TEST_HTTP_PERFORM_TARGET): $(TEST_HTTP_PERFORM_OBJECTS)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $^ -o $@ $(TEST_LDFLAGS)

test: $(TEST_TARGETS)
	@for t in $(TEST_TARGETS); do echo "==> $$t"; $$t || exit $$?; done

.PHONY: all debug release clean run run-release rebuild deps-check test appimage appimage-clean

deps-check:
	@echo "Checking dependencies..."
	@pkg-config --exists gtk4 || (echo "GTK4 not found" && exit 1)
	@pkg-config --exists libadwaita-1 || (echo "libadwaita-1 not found" && exit 1)
	@pkg-config --exists gtksourceview-5 || (echo "GtkSourceView-5 not found" && exit 1)
	@pkg-config --exists libxml-2.0 || (echo "libxml2 not found" && exit 1)
	@pkg-config --exists libcurl || (echo "libcurl not found" && exit 1)
	@pkg-config --exists yaml-0.1 || (echo "libyaml not found via pkg-config (trying fallback)")
	@pkg-config --exists libcjson || (echo "libcjson not found via pkg-config (trying fallback)")
	@echo "Main dependencies OK"

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR) $(GRESOURCE_C)

GIT_VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)
VERSION ?= $(GIT_VERSION)

appimage:
	@VERSION=$(VERSION) RELEASE_MARCH=x86-64 RELEASE_MTUNE=generic \
		bash packaging/appimage/build-appimage.sh

appimage-clean:
	rm -rf $(BUILD_DIR)/AppDir $(BUILD_DIR)/appimage-tools $(BUILD_DIR)/RequestHub-*.AppImage

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
