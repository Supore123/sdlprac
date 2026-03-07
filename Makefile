CXX      = g++
CXXFLAGS = -Iinclude -Wall -Werror -Wextra -std=c++17

SDL2_FLAGS  = $(shell sdl2-config --cflags --libs)
GL_FLAGS    = -lGL -lGLEW
MIXER_FLAGS = -lSDL2_mixer

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
TARGET  = app

SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $@ $(SDL2_FLAGS) $(GL_FLAGS) $(MIXER_FLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(SDL2_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
