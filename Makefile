# Makefile


CC       = gcc
CFLAGS   = -march=native -Ofast -pipe -std=c99 -Wall
includes = CHIP/CHIP/Hexint.c CHIP/CHIP/Hexarray.c

FFMPEG_LIBS = libavdevice   \
              libavformat   \
              libavfilter   \
              libavcodec    \
              libswresample \
              libswscale    \
              libavutil     \
              opencv

# export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)
LDLIBS := $(shell pkg-config --libs   $(FFMPEG_LIBS)) $(LDLIBS) \
          	-lGL -lGLU -lglut -lm -lpthread -lSOIL


.PHONY: Hex-Player

Hex-Player: Hex-Player.c
	$(CC) $(CFLAGS) Hex-Player.c -o Hex-Player $(includes) $(LDLIBS)

help:
	@echo "/*****************************************************************************"
	@echo " * help : Hex-Player - v1.0 - 07.11.2017"
	@echo " *****************************************************************************"
	@echo " * + update : Aktualisierung abhängiger Repos (CHIP)"
	@echo " * + test   : Testlauf"
	@echo " * + clean  : Aufräumen"
	@echo " *****************************************************************************/"

clean:
	find . -maxdepth 1 ! -name "*.c" ! -name "*.man" ! -name "COPYING" \
		! -name "Makefile" -type f -delete

_update:
	git pull
	@if [ -d CHIP ]; then \
		cd CHIP;               \
		echo "CHIP: git pull"; \
		git pull;              \
		cd ..;                 \
	else \
		git clone https://github.com/TSchlosser13/CHIP CHIP; \
	fi

update: _update Hex-Player

test:
	./Hex-Player Testset/Hex-Muxer/jellyfish.mkv
