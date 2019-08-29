#
# http://www.gnu.org/software/make/manual/make.html
#
# sdl 与 sdl2 不兼容
CC:=gcc
INCLUDES:=$(shell pkg-config --cflags sdl2 sdl2_image) \
         -I/Users/fd/Projects/ffmpeg
CFLAGS:=-Wall -ggdb
LDFLAGS:=$(shell pkg-config --libs sdl2 sdl2_image) \
         -L/Users/fd/Projects/ffmpeg/libavformat \
		 -L/Users/fd/Projects/ffmpeg/libavcodec \
		 -L/Users/fd/Projects/ffmpeg/libswresample \
		 -L/Users/fd/Projects/ffmpeg/libswscale \
		 -L/Users/fd/Projects/ffmpeg/libavutil \
		 -lavformat \
		 -lavcodec \
		 -lswresample \
		 -lswscale \
		 -lavutil \
		 -lstdc++ \
		 -lbz2 \
		 -lz \
		 -liconv \
		 -llzma \
		 -lm

LIBS:= -framework VideoToolbox \
       -framework CoreAudio \
	   -framework CoreVideo \
	   -framework AudioToolbox \
	   -framework Security \
	   -framework CoreMedia \
	   -framework AudioUnit \
	   -framework Carbon -v

EXE:=hello_world.out \
	 x01_hello_world.out \

#
# This is here to prevent Make from deleting secondary files.
#
.SECONDARY:
	

#
# $< is the first dependency in the dependency list
# $@ is the target name
#
all: dirs $(addprefix bin/, $(EXE)) 

dirs:
	mkdir -p obj
	mkdir -p bin

bin/%.out: obj/%.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@ $(LIBS)

obj/%.o : %.c
	$(CC) $(CFLAGS) $< $(INCLUDES) -c -o $@ $(LIBS)

obj/%.o : %.cpp
	$(CC) $(CFLAGS) $< $(INCLUDES) -c -o $@ $(LIBS)

clean:
	rm -f obj/*
	rm -f bin/*

