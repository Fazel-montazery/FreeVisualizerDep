INSDIR ?= /usr/local
BINDIR = $(INSDIR)/bin
TARGET := fv

src := src/main.c src/shader.c src/fs.c
libs := -lSDL3 -lmpg123
cc := gcc

all: main

main: ${src}
	${cc} -o ${TARGET} ${src} ${libs} -std=c99 -Wall -O2

debug: ${src}
	${cc} -o ${TARGET} ${src} ${libs} -std=c99 -Wall -O0 -g3 -fsanitize=address

clean: ${TARGET}
	rm ${TARGET}

install: ${TARGET}
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)

uninstall:
	rm -f $(BINDIR)/$(TARGET)
