src := src/main.c src/shader.c
libs := -lSDL3 -lmpg123
target := FreeVisualizer.out
cc := gcc

all: main

main: ${src}
	${cc} -o ${target} ${src} ${libs} -std=c99 -Wall -O2

debug: ${src}
	${cc} -o ${target} ${src} ${libs} -std=c99 -Wall -O0 -g3 -fsanitize=address
