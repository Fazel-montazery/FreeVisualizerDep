src := src/main.c src/shader.c
libs := -lSDL3 -lmpg123
flags := -std=c99 -Wall -O2
target := FreeVisualizer.out
cc := gcc

all: main

main: ${src}
	${cc} -o ${target} ${src} ${libs} ${flags}
