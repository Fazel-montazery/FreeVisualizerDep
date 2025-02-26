#pragma once

#define MUSIC_BUFFER_SIZE 4096
#define MUSIC_CONTROLL_COEFFICIENT 2

#define PATH_SIZE 1024
#define DATA_DIR ".FreeVisualizer"
#define SHADER_DIR "shaders"

#if defined(_WIN32) || defined(_WIN64)
#define VERT_SHADER_NAME "vertex.vert.dxil"
#elif defined(APPLE)
#define VERT_SHADER_NAME "vertex.vert.msl"
#else
#define VERT_SHADER_NAME "vertex.vert.spv"
#endif

#if defined(_WIN32) || defined(_WIN64)
#define SHADER_EXT ".frag.dxil"
#elif defined(APPLE)
#define SHADER_EXT ".frag.msl"
#else
#define SHADER_EXT ".frag.spv"
#endif

#define DEFAULT_STATE \
    (State) {  \
	.winWidth = 1000,	\
	.winHeight = 1000,	\
	.fullscreen = false,	\
    }
