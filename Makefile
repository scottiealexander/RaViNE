# NOTE: to build libparingbuffer.a:
#  cd <port_audio_dir>/src/common
#  gcc -I./ -c -o pa_ringbuffer.o pa_ringbuffer.c
#  ar rcs ../../lib/.libs/libparingbuffer.a ./pa_ringbuffer.o

#portaudio dependency
ifndef PORTAUDIO_PATH
PORTAUDIO_PATH := /home/pi/Libraries/portaudio
endif

PA_LIBS := $(PORTAUDIO_PATH)/lib/.libs
PA_INCLUDE := $(PORTAUDIO_PATH)/include
PA_COMMON := $(PORTAUDIO_PATH)/src/common

#asio dependency
ifndef ASIO_PATH
ASIO_PATH := /home/pi/Libraries/asio-1.12.2
endif

ASIO_INCLUDE := $(ASIO_PATH)/include

CXX      := -g++
CXXFLAGS := -pedantic-errors -Wall -Wextra -std=c++11 -L$(PA_LIBS)

#make sure to indicate to asio that we are *NOT* using boost
CXXFLAGS += -DASIO_STANDALONE=1

LDFLAGS  := -lm -pthread -lasound -lportaudio -lparingbuffer
BUILD    := ./build
ASSETS   := ./assets
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/app
TARGET   := ravine
INCLUDE  :=				\
	-I./src/filters/	\
	-I./src/packets/	\
	-I./src/sinks/		\
	-I./src/sources/	\
	-I./src/utils/		\
	-I$(PA_INCLUDE)		\
	-I$(PA_COMMON)		\
	-I$(ASIO_INCLUDE)	\

SRC      :=												\
	$(wildcard ./src/utils/ravine_clock.cpp)			\
	$(wildcard ./src/utils/ravine_pink_noise.cpp)		\
	$(wildcard ./src/utils/ravine_spike_waveform.cpp)	\
	$(wildcard ./src/packets/ravine_packets.cpp)		\
	$(wildcard ./src/sources/ravine_video_source.cpp)	\
	$(wildcard ./src/sources/ravine_event_source.cpp)	\
	$(wildcard ./src/filters/ravine_audio_filter.cpp)	\
	$(wildcard ./src/filters/ravine_neuron_filter.cpp)	\
	$(wildcard ./src/sinks/ravine_datafile_sink.cpp)	\
	$(wildcard ./ravine.cpp)                       		\

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
	@mkdir -p $(APP_DIR)/rf
	@mkdir -p $(APP_DIR)/data
	@cp -u $(ASSETS)/*.pgm $(APP_DIR)/rf/
	@cp -u $(ASSETS)/spike.wf $(APP_DIR)/spike.wf

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/$(TARGET)
