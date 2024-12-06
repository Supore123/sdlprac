# Compiler and flags
CC =gcc
CFLAGS = -Iinclude -Wall -Werror -Wextra -std=c11

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Target executable
TARGET = app

# Find all .c files in SRC_DIR and replace .c with .o for object files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $@

# Compile .c files to .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
# Launch Application
launch: all
	./$(TARGET)

# Phony targets to avoid conflicts with files named 'clean', 'all', etc.
.PHONY: all clean launch
