# =========================================================
#  Portable Raylib + w64devkit Makefile
#  Everything is pointed at the thumb drive so this project
#  builds the same on any machine you plug D: into.
# =========================================================

# ---- Portable toolchain / library locations ----
# Default to 64-bit build
RAYLIB_PATH    := C:/raylib-6.0_win64_mingw-w64
COMPILER_PATH  := C:/w64devkit

CXX   := $(COMPILER_PATH)/bin/g++.exe
CC    := $(COMPILER_PATH)/bin/gcc.exe
RM    := $(COMPILER_PATH)/bin/rm.exe
MKDIR := $(COMPILER_PATH)/bin/mkdir.exe
CP    := $(COMPILER_PATH)/bin/cp.exe

# ---- Project layout ----
SRC_DIR   := src
INC_DIR   := include
RES_DIR   := resources
BUILD_DIR := build
BIN_DIR   := bin
TARGET    := $(BIN_DIR)/game.exe

# ---- Recursive wildcard helper ----
# Walks every sub-directory of $1 looking for files matching $2
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# ---- Gather every .cpp and .c file under src/ (any depth) ----
SRCS := $(call rwildcard,$(SRC_DIR)/,*.cpp) $(call rwildcard,$(SRC_DIR)/,*.c)

# ---- Separate C and C++ object files ----
C_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SRCS)))
CPP_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter %.cpp,$(SRCS)))
OBJS := $(C_OBJS) $(CPP_OBJS)
DEPS := $(OBJS:.o=.d)

# ---- Gather every folder under include/ that contains a .hpp or .h ----
INCLUDE_SUBDIRS := $(sort $(dir $(call rwildcard,$(INC_DIR)/,*.hpp)) $(dir $(call rwildcard,$(INC_DIR)/,*.h)))
SRC_SUBDIRS      := $(sort $(dir $(call rwildcard,$(SRC_DIR)/,*.hpp)) $(dir $(call rwildcard,$(SRC_DIR)/,*.h)))

INCLUDE_DIRS := $(INC_DIR) $(SRC_DIR) $(INCLUDE_SUBDIRS) $(SRC_SUBDIRS)

# ---- Compiler / linker flags ----
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -MMD -MP \
            $(addprefix -I,$(INCLUDE_DIRS)) \
            -I$(RAYLIB_PATH)/include

CFLAGS := -std=c17 -Wall -Wextra -O2 -I$(RAYLIB_PATH)/include

LDFLAGS := -L$(RAYLIB_PATH)/lib
LDLIBS  := -lraylib -lopengl32 -lgdi32 -lwinmm -static -static-libgcc -static-libstdc++

# =========================================================
#  Targets
# =========================================================

.PHONY: all run clean resources

all: $(TARGET) resources

# Link
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

# Compile C++ files (mirrors src/ subfolder structure into build/)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C files (mirrors src/ subfolder structure into build/)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@$(MKDIR) -p $(BUILD_DIR)

$(BIN_DIR):
	@$(MKDIR) -p $(BIN_DIR)

# Copy resources next to the exe so relative asset paths work
resources: | $(BIN_DIR)
	@$(CP) -r $(RES_DIR) $(BIN_DIR)/ 2>/dev/null || true

run: all
	cd $(BIN_DIR) && ./game.exe

clean:
	@$(RM) -rf $(BUILD_DIR) $(BIN_DIR)

# Auto-generated header dependency files
-include $(DEPS)
