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
		 -L/Users/fd/Projects/ffmpeg/libavfilter \
		 -L/Users/fd/Projects/ffmpeg/libpostproc \
		 -lpostproc \
		 -lavfilter \
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
	   -framework OpenGL \
	   -framework AppKit \
	   -framework QuartzCore \
	   -framework Cocoa \
	   -framework Carbon -v

# -framework OpenGL 
# -framework AppKit 
# -framework Security 
# -framework CoreFoundation 
# -framework CoreVideo 
# -framework CoreMedia 
# -framework QuartzCore 
# -framework VideoDecodeAcceleration 
# -framework Cocoa


EXE:=sdl2_00_only_window.out \
     sdl2_01_player.out \
	 sdl2_02_audio_player.out \
	 sdl2_03_waveform.out \
	 sdl2_04_showwave.out \
	 office_filter_audio.out \
	 resampling_audio.out \
	 filtering_audio.out \
	 muxing.out

	# sdl_app.out	\
    # tutorial01.out tutorial02.out tutorial03.out \

    # tutorial04.out\
	# tutorial05.out tutorial06.out tutorial07.out tutorial08.out
# EXE:=sdl_app.out
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

obj/%.o : %.cpp
	$(CC) $(CFLAGS) $< $(INCLUDES) -c -o $@ $(LIBS)

clean:
	rm -f obj/*
	rm -f bin/*

