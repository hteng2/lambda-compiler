# ------------------------------------------------------------------------------
# Project Variables
# ------------------------------------------------------------------------------
BIN      := lambda
SRCDIR   := src
INCDIR   := include
BUILDDIR := build

CC       := clang
CFLAGS   := -Werror -Wall -Wextra -Wpedantic -Wconversion -Wunused
CFLAGS   += -std=c23 -g -Og -fno-inline
CFLAGS   += -Iinclude -MMD

# ------------------------------------------------------------------------------
# Automatic Source & Object File Discovery
# ------------------------------------------------------------------------------
# Recursively find all .c files in SRCDIR (handles nested subdirectories)
SRCS := $(shell find $(SRCDIR) -type f -name '*.c')

# Map src/main.c -> build/main.o, src/sub/file.c -> build/sub/file.o
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

# Dependency files (.d) are generated alongside the .o files
DEPS := $(OBJS:.o=.d)

# ------------------------------------------------------------------------------
# Phony Targets
# ------------------------------------------------------------------------------
.PHONY: all clean

# Default target
all: $(BIN)

# ------------------------------------------------------------------------------
# Link the Final Executable
# ------------------------------------------------------------------------------
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# ------------------------------------------------------------------------------
# Compile Source Files into Object Files
# ------------------------------------------------------------------------------
# Ensure the output directory exists before compiling
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Create the root build directory (order-only prerequisite)
$(BUILDDIR):
	mkdir -p $@

# ------------------------------------------------------------------------------
# Include Auto-Generated Dependency Files
# ------------------------------------------------------------------------------
# The '-' prefix ignores errors if .d files don't exist yet (fresh build)
-include $(DEPS)

# ------------------------------------------------------------------------------
# Clean up build artifacts
# ------------------------------------------------------------------------------
clean:
	rm -rf $(BUILDDIR) $(BIN)
