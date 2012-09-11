EXE=a.out

CXXFLAGS=-g --std=c++11 -stdlib=libc++ -Werror -Weverything -pedantic -Wno-c++98-compat -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors `sdl-config --cflags`
CFLAGS=`sdl-config --cflags`
LDFLAGS=-g --std=c++11 -stdlib=libc++ -Werror -Weverything

$(EXE): fontstash.c main.c fontstash.h stb_truetype.h
	clang fontstash.c main.c -o $(EXE) $(CFLAGS) `sdl-config --libs` -lSDL_ttf -framework OpenGL -framework Cocoa -lpthread
