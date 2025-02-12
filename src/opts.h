#pragma once

#include <SDL3/SDL.h>

#include <getopt.h>

typedef struct {
	char* musicPath;
} OptState;

bool parseOpts(int argc, char *argv[], OptState* res); // returns false if exit condition, else true
