#pragma once

#include <SDL3/SDL.h>

#include <string.h>
#include <errno.h>

#include <getopt.h>

typedef struct {
	char* musicPath;
	char* fragShaderPath;
	char* vertShaderPath;
} OptState;

bool parseOpts(int argc, char *argv[], OptState* res); // returns false if exit condition, else true
