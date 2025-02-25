.PHONY: install uninstall clean

INSDIR ?= /usr/local
BINDIR = ${INSDIR}/bin
USR_HOME ?= $(shell getent passwd $(SUDO_USER) | cut -d: -f6)
DATADIR ?= ${USR_HOME}/.FreeVisualizer
SHADERDIR := ${DATADIR}/shaders
TARGET := fv

src := src/main.c src/shader.c src/opts.c
libs := -lSDL3 -lmpg123
cc := gcc

ifeq ($(OS),Windows_NT)
	detected_OS := Windows
else
	detected_OS := $(shell uname)
endif

ifeq (${detected_OS}, Windows)
	SHADER_FORMAT := DXIL
else ifeq (${detected_OS}, Darvin)
	SHADER_FORMAT := MSL
else
	SHADER_FORMAT := SPIRV
endif

SHADERS := shaders/out/${SHADER_FORMAT}/*

all: main

main: ${src}
	${cc} -o ${TARGET} ${src} ${libs} -std=c99 -Wall -O2

debug: ${src}
	${cc} -o ${TARGET} ${src} ${libs} -std=c99 -Wall -O0 -g3 -fsanitize=address

clean: ${TARGET}
	rm ${TARGET}

install: ${TARGET}
	install -d ${BINDIR}
	install -m 755 ${TARGET} ${BINDIR}/${TARGET}
	install -d ${SHADERDIR}
	install -m 644  ${SHADERS} ${SHADERDIR}/

uninstall:
	rm -f ${BINDIR}/${TARGET}
	rm -rf ${DATADIR}
