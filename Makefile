# Compiler and flags
CXX := g++
CXXFLAGS := -Wall -std=c++11

# Directories
SRC_DIR := src
BUILD_DIR := build
LIB_DIR := lib

# List of source files
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)

# Generate object file names from source files
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Name of the static library
LIBRARY_NAME := asabru-parsers.a

# Targets
all: $(LIB_DIR)/$(LIBRARY_NAME)

$(LIB_DIR)/$(LIBRARY_NAME): $(OBJ_FILES)
	ar rcs $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(BUILD_DIR)/*.o $(LIB_DIR)/$(LIBRARY_NAME)

.PHONY: all clean