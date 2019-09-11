# NOTE: to build libparingbuffer.a:
#  cd <port_audio_dir>/src/common
#  gcc -I./ -c -o pa_ringbuffer.o pa_ringbuffer.c
#  ar rcs ../../lib/.libs/libparingbuffer.a ./pa_ringbuffer.o

ifndef PORTAUDIO_PATH
PORTAUDIO_PATH := /home/pi/Libraries/portaudio
endif

PA_LIBS := $(PORTAUDIO_PATH)/lib/.libs
PA_INCLUDE := $(PORTAUDIO_PATH)/include
PA_COMMON := $(PORTAUDIO_PATH)/src/common

CXX      := -g++
CXXFLAGS := -pedantic-errors -Wall -Wextra -std=c++11 -L$(PA_LIBS)
LDFLAGS  := -lm -pthread -lasound -lportaudio -lparingbuffer
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/app
TARGET   := ravine_audio_test
INCLUDE  := -I./ -I$(PA_INCLUDE) -I$(PA_COMMON)
SRC      :=                                 \
	$(wildcard ./ravine_pink_noise.cpp)     \
	$(wildcard ./ravine_spike_waveform.cpp) \
	$(wildcard ./ravine_packets.cpp)        \
	$(wildcard ./ravine_audio_filter.cpp)   \
	$(wildcard ./ravine_datafile_sink.cpp)  \
	$(wildcard ./ravine_audio_test1.cpp)    \

OBJECTS := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

#generate dependency files... i think?
DEPENDS := $(SRC:%.cpp=$(OBJ_DIR)/%.d)

all: build $(APP_DIR)/$(TARGET)

#include dependencies in the makefile, not really sure what this does...
-include $(DEPENDS)

#note the -MMD -MP, these apparently trigger re-building the .o when any file
#listed in the corresponding .d (dependency) file changes... I think..
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
