#
# http://www.gnu.org/software/make/manual/make.html
#
CC:=gcc
INCLUDES:=$(shell pkg-config --cflags sdl) \
         -I/Users/fd/Projects/ffmpeg
CFLAGS:=-Wall -ggdb
LDFLAGS:=$(shell pkg-config --libs sdl) \
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

# EXE:=tutorial01.out \
    # tutorial02.out tutorial03.out tutorial04.out\
	# tutorial05.out tutorial06.out tutorial07.out tutorial08.out
EXE:=tutorial03.out

# 		 -std=c++11 -stdlib=libc++
#  -lstdc++

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

clean:
	rm -f obj/*
	rm -f bin/*

