
CXX      := -g++
CXXFLAGS := -pedantic-errors -Wall -Wextra -std=c++11
LDFLAGS  := -lm -pthread
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/app
TARGET   := ravine_video_test2
INCLUDE  :=				\
	-I./src/filters/	\
	-I./src/packets/	\
	-I./src/sinks/		\
	-I./src/sources/	\
	-I./src/utils/		\

SRC      :=                                       		\
	$(wildcard ./src/utils/ravine_clock.cpp)        	\
	$(wildcard ./src/packets/ravine_packets.cpp)      	\
	$(wildcard ./src/sources/ravine_video_source.cpp)	\
    $(wildcard ./src/packets/ravine_frame_buffer.cpp)	\
	$(wildcard ./src/sinks/ravine_file_sink.cpp)		\
	$(wildcard ./src/tests/ravine_video_test2.cpp)		\

OBJECTS := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

#generate dependency files... i think?
DEPENDS := $(SRC:%.cpp=$(OBJ_DIR)/%.d)

all: build $(APP_DIR)/$(TARGET)

#include dependencies in the makefile, not really sure what this does... /  how
#it does the "inclusion", but it seems to work so far...
-include $(DEPENDS)

#note the -MMD -MP, these apparently trigger re-building the .o when any file
#listed in the corresponding .d (dependency) file changes... I think...
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -MMD -MP -c $<

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) -o $(APP_DIR)/$(TARGET) $(INCLUDE) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS)

.PHONY: all build clean debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(APP_DIR)/frames

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/$(TARGET)
