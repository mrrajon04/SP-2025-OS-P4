TARGET_EXEC ?= myprogram
TARGET_TEST ?= test-lab

BUILD_DIR ?= build
TEST_DIR ?= tests
SRC_DIR ?= src
EXE_DIR ?= app

# Scan and gather all core C source files
SRCS := $(shell find $(SRC_DIR) -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Scan and collect all test C files
TEST_SRCS := $(shell find $(TEST_DIR) -name *.c)
TEST_OBJS := $(TEST_SRCS:%=$(BUILD_DIR)/%.o)
TEST_DEPS := $(TEST_OBJS:.o=.d)

# Source scan for app executables
EXE_SRCS := $(shell find $(EXE_DIR) -name *.c)
EXE_OBJS := $(EXE_SRCS:%=$(BUILD_DIR)/%.o)
EXE_DEPS := $(EXE_OBJS:.o=.d)

# Compiler settings and optional tools (warnings, debug flags, etc.)
CFLAGS ?= -Wall -Wextra -MMD -MP
DEBUG ?= -g
SANATIZE ?= -fno-omit-frame-pointer -fsanitize=address

# Uncomment and add libraries here if needed by linking stage
LDFLAGS ?= -pthread -lreadline

# Primary build target – builds both main and test executables
all: $(TARGET_EXEC) $(TARGET_TEST)

# Specialized build with sanitizers and debug hooks enabled
debug: CFLAGS += $(SANATIZE)
debug: CFLAGS += $(DEBUG)
debug: $(TARGET_EXEC) $(TARGET_TEST)

# Compilation recipe to link main binary with all components
$(TARGET_EXEC): $(OBJS) $(EXE_OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(EXE_OBJS) -o $@ $(LDFLAGS)

# Compile test binary from core + test code
$(TARGET_TEST): $(OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(TEST_OBJS) -o $@ $(LDFLAGS)

# Create object files from C source files with directory protection
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Run test binary with memory checker enabled
check: $(TARGET_TEST)
	ASAN_OPTIONS=detect_leaks=1 ./$<

# Shortcut to purge all compiled/intermediate artifacts
.PHONY: clean
clean:
	$(RM) -rf $(BUILD_DIR) $(TARGET_EXEC) $(TARGET_TEST)

# Optional helper to install Git email dependencies (for Codespaces or similar)
.PHONY: install-deps
install-deps:
	sudo apt-get update -y
	sudo apt-get install -y libio-socket-ssl-perl libmime-tools-perl

# Pull in any generated dependency metadata silently
-include $(DEPS) $(TEST_DEPS) $(EXE_DEPS)

