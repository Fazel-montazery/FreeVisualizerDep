#pragma once

#include <SDL3/SDL.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <getopt.h>

#include "defs.h"

// returns false if exit condition, else true
// caller must allocate paths buffers
// bufferSiz is the size of PathBuffers
bool parseOpts( int argc, 
		char *argv[],
		char* musicPath,
		char* fragShaderPathBuf,
		char* vertShaderPathBuf,
		size_t bufferSiz);
