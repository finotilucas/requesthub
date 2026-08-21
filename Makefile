# ==== Toolchain ==============================================================
ifeq ($(origin CC),default)
    CC := $(shell command -v zig >/dev/null 2>&1 && echo 'zig cc' || echo gcc)
endif
SAN_CC ?= $(shell command -v clang >/dev/null 2>&1 && echo clang || echo gcc)

UNAME_S := $(shell uname -s)
ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    EXE            := .exe
    SANITIZE       :=
    EXTRA_GIO_LIBS :=
else
    EXE            :=
    EXTRA_GIO_LIBS := -lgmodule-2.0 -lmount
    SANITIZE       := $(shell tmp=$$(mktemp 2>/dev/null) && \
        echo 'int main(void){return 0;}' | $(SAN_CC) -fsanitize=address,undefined -xc - -o $$tmp 2>/dev/null && \
        echo '-fsanitize=address,undefined'; rm -f $$tmp)
endif

# ==== Dependencies ==============================================

PKGS           := gtk4 libadwaita-1 gtksourceview-5
PKG_CFLAGS     := $(shell pkg-config --cflags $(PKGS))
PKG_LIBS       := $(shell pkg-config --libs $(PKGS))

LIBXML2_CFLAGS := $(shell pkg-config --cflags libxml-2.0 2>/dev/null || echo -I/usr/include/libxml2)
LIBXML2_LIBS   := $(shell pkg-config --libs libxml-2.0 2>/dev/null || echo -lxml2)
YAML_CFLAGS    := $(shell pkg-config --cflags yaml-0.1 2>/dev/null)
YAML_LIBS      := $(shell pkg-config --libs yaml-0.1 2>/dev/null || echo -lyaml)
CJSON_CFLAGS   := $(shell pkg-config --cflags libcjson 2>/dev/null)
CJSON_LIBS     := $(shell pkg-config --libs libcjson 2>/dev/null || echo -lcjson)

# ==== Flags ==================================================================

WARNINGS       := -Wall -Wextra -Werror
CFLAGS_COMMON  := -std=gnu11 $(WARNINGS) -MMD -MP \
                  $(PKG_CFLAGS) $(CJSON_CFLAGS) $(LIBXML2_CFLAGS) $(YAML_CFLAGS)
ARCH_FLAGS     ?=
CFLAGS_DEBUG   := -g $(SANITIZE) $(CFLAGS_COMMON)
CFLAGS_RELEASE := -O2 -DNDEBUG $(ARCH_FLAGS) $(CFLAGS_COMMON)

LDLIBS         := $(PKG_LIBS) $(CJSON_LIBS) $(LIBXML2_LIBS) $(YAML_LIBS) \
                  $(EXTRA_GIO_LIBS) -lcurl -lm

# ==== Layout =================================================================

SRC_DIR         := src
OBJ_DIR         := obj
BUILD_DIR       := build

GRESOURCE_XML   := $(SRC_DIR)/ui/styles/styles.gresource.xml
GRESOURCE_C     := $(SRC_DIR)/ui/styles/styles.gresource.c
GRESOURCE_DEPS  := $(shell glib-compile-resources --sourcedir=$(SRC_DIR)/ui/styles --generate-dependencies $(GRESOURCE_XML) 2>/dev/null)

SOURCES         := $(filter-out $(GRESOURCE_C),$(shell find $(SRC_DIR) -name '*.c')) $(GRESOURCE_C) main.c

DEBUG_OBJECTS   := $(SOURCES:%.c=$(OBJ_DIR)/debug/%.o)
RELEASE_OBJECTS := $(SOURCES:%.c=$(OBJ_DIR)/release/%.o)

TARGET_DEBUG    := $(BUILD_DIR)/requesthub$(EXE)
TARGET_RELEASE  := $(BUILD_DIR)/release/requesthub$(EXE)

# ==== App ====================================================================

all: debug
debug: $(TARGET_DEBUG)
release: $(TARGET_RELEASE)

$(TARGET_DEBUG): $(DEBUG_OBJECTS)
	@mkdir -p $(@D)
	$(SAN_CC) $(SANITIZE) $^ -o $@ $(LDLIBS)

$(TARGET_RELEASE): $(RELEASE_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $^ -o $@ $(LDLIBS)

$(OBJ_DIR)/debug/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling (debug):   $<"
	@$(SAN_CC) $(CFLAGS_DEBUG) -c $< -o $@

$(OBJ_DIR)/release/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling (release): $<"
	@$(CC) $(CFLAGS_RELEASE) -c $< -o $@

$(GRESOURCE_C): $(GRESOURCE_XML) $(GRESOURCE_DEPS)
	@echo "Generating GResource: $@"
	glib-compile-resources --target=$@ --generate-source --sourcedir=$(SRC_DIR)/ui/styles $(GRESOURCE_XML)

# ==== Tests ==================================================================

TEST_CFLAGS  := -g $(SANITIZE) -std=gnu11 $(WARNINGS) -MMD -MP \
                $(shell pkg-config --cflags gio-2.0) $(CJSON_CFLAGS) \
                $(shell pkg-config --cflags libcurl 2>/dev/null)
TEST_LDLIBS  := $(shell pkg-config --libs gio-2.0) $(CJSON_LIBS) \
                $(shell pkg-config --libs libcurl 2>/dev/null || echo -lcurl) -lpthread

TESTS                   := test_history test_history_service test_request \
                           test_response test_http_pool test_http_perform
test_history_SRCS       := $(SRC_DIR)/history/history.c \
                           $(SRC_DIR)/history/history_persistence.c \
                           $(SRC_DIR)/models/request_data.c
test_history_service_SRCS := $(SRC_DIR)/services/history_service.c \
                           $(SRC_DIR)/history/history.c \
                           $(SRC_DIR)/history/history_persistence.c \
                           $(SRC_DIR)/models/request_data.c
test_request_SRCS       := $(SRC_DIR)/http/request.c $(SRC_DIR)/http/methods.c
test_response_SRCS      := $(SRC_DIR)/http/response.c
test_http_pool_SRCS     := $(SRC_DIR)/http/http_pool.c
test_http_perform_SRCS  := $(SRC_DIR)/http/http.c $(SRC_DIR)/http/http_pool.c \
                           $(SRC_DIR)/http/methods.c $(SRC_DIR)/http/request.c \
                           $(SRC_DIR)/http/response.c

TEST_TARGETS := $(TESTS:%=$(BUILD_DIR)/test/%$(EXE))

$(OBJ_DIR)/test/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling (test):    $<"
	@$(SAN_CC) $(TEST_CFLAGS) -c $< -o $@

define TEST_template
$(BUILD_DIR)/test/$(1)$(EXE): $(patsubst %.c,$(OBJ_DIR)/test/%.o,tests/$(1).c $($(1)_SRCS))
	@mkdir -p $$(@D)
	$$(SAN_CC) $$(SANITIZE) $$^ -o $$@ $$(TEST_LDLIBS)
endef
$(foreach t,$(TESTS),$(eval $(call TEST_template,$(t))))

test: $(TEST_TARGETS)
	@for t in $(TEST_TARGETS); do echo "==> $$t"; $$t || exit $$?; done

# ==== Utilities ==============================================================

REQUIRED_PKGS := $(PKGS) libxml-2.0 libcurl

deps-check:
	@for p in $(REQUIRED_PKGS); do \
		pkg-config --exists $$p || { echo "$$p not found"; exit 1; }; \
	done
	@pkg-config --exists yaml-0.1 || echo "yaml-0.1 not found via pkg-config (using -lyaml fallback)"
	@pkg-config --exists libcjson || echo "libcjson not found via pkg-config (using -lcjson fallback)"
	@echo "Dependencies OK"

info:
	@echo "cc        $(CC)"
	@echo "san cc    $(SAN_CC)"
	@echo "sanitize  $(SANITIZE)"
	@echo "arch      $(if $(ARCH_FLAGS),$(ARCH_FLAGS),(baseline))"

run: debug
	./$(TARGET_DEBUG)

run-release: release
	./$(TARGET_RELEASE)

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR) $(GRESOURCE_C)

rebuild:
	$(MAKE) clean
	$(MAKE) release

GIT_VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)
VERSION ?= $(GIT_VERSION)

appimage:
	@VERSION=$(VERSION) bash packaging/appimage/build-appimage.sh

appimage-clean:
	rm -rf $(BUILD_DIR)/AppDir $(BUILD_DIR)/appimage-tools $(BUILD_DIR)/RequestHub-*.AppImage

ifeq ($(EXE),)
valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET_DEBUG)

compile_commands:
	bear -- $(MAKE) clean debug
endif

-include $(DEBUG_OBJECTS:.o=.d) $(RELEASE_OBJECTS:.o=.d) \
         $(TEST_TARGETS:$(BUILD_DIR)/test/%$(EXE)=$(OBJ_DIR)/test/tests/%.d)

.PHONY: all debug release test clean run run-release rebuild deps-check info \
        appimage appimage-clean valgrind compile_commands
