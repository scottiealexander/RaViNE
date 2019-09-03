CXX      := -g++
CXXFLAGS := -pedantic-errors -Wall -Wextra -std=c++11
#LDFLAGS  := -L/usr/lib -lstdc++ -lm -pthread
LDFLAGS  := -lm -pthread
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/app
TARGET   := ravine
INCLUDE  := -I./
SRC      :=                              \
	$(wildcard ./ravine_video_source.cpp) \
	$(wildcard ./ravine_file_sink.cpp)    \
	$(wildcard ./ravine_frame_buffer.cpp) \
	$(wildcard ./ravine_packets.cpp)      \
	$(wildcard ./ravine_video_test2.cpp)   \

OBJECTS := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LDFLAGS) -o $(APP_DIR)/$(TARGET) $(OBJECTS)

.PHONY: all build clean debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*
