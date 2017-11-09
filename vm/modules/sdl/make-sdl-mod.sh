#!/bin/sh

gcc -Wall -fPIC -g -c sdl.c -O3 -fomit-frame-pointer -g0 -s
gcc -shared -Wl,-soname,libl1vmsdl.so.1 -o libl1vmsdl.so.1.0 sdl.o -lSDL -lSDL_gfx
cp libl1vmsdl.so.1.0 libl1vmsdl.so
