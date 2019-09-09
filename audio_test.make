
ifndef PORTAUDIO_PATH
PORTAUDIO_PATH := /home/pi/Libraries/portaudio
endif
PA_LIBS := $(PORTAUDIO_PATH)/lib/.libs
PA_INCLUDE := $(PORTAUDIO_PATH)/include

CXX      := -g++
CXXFLAGS := -pedantic-errors -Wall -Wextra -std=c++11 -L$(PA_LIBS)
# LDFLAGS  := -L$(PA_LIBS) -lm -pthread -lasound -lportaudio
LDFLAGS  := -lm -pthread -lasound -lportaudio
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/app
TARGET   := ravine_audio_test
INCLUDE  := -I./ -I$(PA_INCLUDE)
SRC      :=                                 \
	$(wildcard ./ravine_pink_noise.cpp)     \
	$(wildcard ./ravine_spike_waveform.cpp) \
	$(wildcard ./ravine_packets.cpp)        \
    $(wildcard ./ravine_audio_filter.cpp)   \
	$(wildcard ./ravine_audio_test1.cpp)    \

OBJECTS := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

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
