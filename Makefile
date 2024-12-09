# Compiler and flags
CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -std=c11 -Ilib/cJSON 
LDFLAGS = -lSDL2 -lGL -lGLU -lglut -lSDL2_image

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
LIB_DIR = lib

# Target executable
TARGET = app

# Find all .c files in SRC_DIR and LIB_DIR (excluding lib/cJSON/test.c)
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
LIB_FILES = $(filter-out $(LIB_DIR)/cJSON/test.c, $(wildcard $(LIB_DIR)/**/*.c))
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES)) \
            $(patsubst $(LIB_DIR)/%.c, $(OBJ_DIR)/lib/%.o, $(LIB_FILES))

# Default target
all: $(TARGET) clean

# Link object files to create the executable
$(TARGET): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

# Compile .c files in SRC_DIR to .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile .c files in LIB_DIR to .o files
$(OBJ_DIR)/lib/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR)

# Launch Application
launch: all
	./$(TARGET)

# Phony targets to avoid conflicts with files named 'clean', 'all', etc.
.PHONY: all clean launch
