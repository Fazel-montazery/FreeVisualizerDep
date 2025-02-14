#include "opts.h"

#define PATH_SIZE 512
#define DATA_DIR ".FreeVisualizer"
#define SHADER_DIR "shaders"

#if defined(_WIN32) || defined(_WIN64)
#define SHADER_EXT ".frag.dxil"
#elif defined(APPLE)
#define SHADER_EXT ".frag.msl"
#else
#define SHADER_EXT ".frag.spv"
#endif

#define OP_STRING "hs:L"

static SDL_EnumerationResult SDLCALL directory_enum_callback(void *userdata, const char *dirname, const char *fname)
{
	if (!fname) return SDL_ENUM_CONTINUE;

	char* ext = strchr(fname, '.');
	if (!ext) {
		SDL_Log("[Bad ext] => %s\n", fname);
		return SDL_ENUM_CONTINUE;
	}

	if (strcmp(ext, SHADER_EXT) != 0) return SDL_ENUM_CONTINUE;

	int n = ext - fname;
	if (n <= 0) {
		SDL_Log("[Bad name] => %s\n", fname);
		return SDL_ENUM_CONTINUE;
	}

	SDL_Log("%.*s\n", n, fname);

	return SDL_ENUM_CONTINUE;
}

static const struct option opts[] = {
	{"help", no_argument, 0, 'h'},
	{"scene", 1, 0, 's'},
	{"ls", no_argument, 0, 'L'},
	{0, 0, 0, 0}
};

bool parseOpts(int argc, char *argv[], OptState* res)
{
	int opt;
	int indx = 0;

	while ((opt = getopt_long(argc, argv, OP_STRING, opts, &indx)) != -1) {
		switch (opt) {
		case 0:
			break;

		case 'h':
			SDL_Log("Usage: %s [OPTIONS] <mp3 file>\n\n"
					"Options:\n", argv[0]);
			SDL_Log("  %-20s%s\n", "-h, --help", "Print this help message");
			SDL_Log("  %-20s%s\n", "-s, --scene", "Which scene(shader) to use");
			SDL_Log("  %-20s%s\n", "-L, --ls", "list scenes");
			return false;

		case 's':
			break;

		case 'L':
			const char* home = SDL_GetUserFolder(SDL_FOLDER_HOME);
			if (!home) {
				SDL_Log("Couldn't retrive home directory: %s\n", SDL_GetError());
			}

			char shaderDir[PATH_SIZE];
			SDL_snprintf(shaderDir, PATH_SIZE, "%s/%s/%s", home, DATA_DIR, SHADER_DIR);
			
			if (!SDL_CreateDirectory(shaderDir)) {
				SDL_Log("Couldn't retrive/create shader directory: %s\n", SDL_GetError());
			}

			if (!SDL_EnumerateDirectory(shaderDir, directory_enum_callback, NULL)) {
				SDL_Log("Couldn't list shaders: %s\n", SDL_GetError());
			}

			return false;

		case '?':
			return false;

		default:
			return false;
		}
	}

	if (optind < argc) {
		res->musicPath = argv[optind];
	} else {
		SDL_Log("Usage: %s [OPTIONS] <mp3 file>\n"
			"run '%s -h' for help\n", argv[0], argv[0]);
		return false;
	}

	return true;
}
