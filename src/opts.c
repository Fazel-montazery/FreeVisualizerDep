#include "opts.h"

#define OP_STRING "hs:L"

static SDL_EnumerationResult SDLCALL log_scenes(void *userdata, const char *dirname, const char *fname)
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

static bool fileExists = false; // FOR_LATER: WTF IS THIS? GET RID OF GLOBAL
static SDL_EnumerationResult SDLCALL check_file_exist(void *userdata, const char *dirname, const char *fname)
{
	if (!fname) return SDL_ENUM_CONTINUE;

	char* fragShaderPathBuf = userdata;
	if (strcmp(fname, fragShaderPathBuf) == 0) {
		fileExists = true;
		return SDL_ENUM_SUCCESS;
	}

	return SDL_ENUM_CONTINUE;
}

static const struct option opts[] = {
	{"help", no_argument, 0, 'h'},
	{"scene", required_argument, 0, 's'},
	{"ls", no_argument, 0, 'L'},
	{0, 0, 0, 0}
};

bool parseOpts( int argc, 
		char *argv[],
		char** musicPath,
		char* fragShaderPathBuf,
		char* vertShaderPathBuf,
		size_t bufferSiz
)
{
	const char* home = SDL_GetUserFolder(SDL_FOLDER_HOME);
	if (!home) {
		SDL_Log("Couldn't retrive home directory: %s\n", SDL_GetError());
		return false;
	}

	char shaderDir[PATH_SIZE] = { 0 };
	SDL_snprintf(shaderDir, PATH_SIZE, "%s%s/%s", home, DATA_DIR, SHADER_DIR);
	
	if (!SDL_CreateDirectory(shaderDir)) {
		SDL_Log("Couldn't retrive/create shader directory: %s\n", SDL_GetError());
		return false;
	}

	bool sceneSet = false;
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
			fileExists = false;
			char sceneName[PATH_SIZE] = { 0 };
			SDL_snprintf(sceneName, PATH_SIZE, "%s%s", optarg, SHADER_EXT);
			if (!SDL_EnumerateDirectory(shaderDir, check_file_exist, sceneName)) {
				SDL_Log("Couldn't fetch scenes: %s\n", SDL_GetError());
				return false;
			}

			if (!fileExists) {
				SDL_Log("Scene Doesn't exist!\n"
					"Available scenes:\n");

				if (!SDL_EnumerateDirectory(shaderDir, log_scenes, NULL)) {
					SDL_Log("Couldn't list scenes: %s\n", SDL_GetError());
					return false;
				}
				return false;
			}

			SDL_snprintf(fragShaderPathBuf, PATH_SIZE, "%s/%s", shaderDir, sceneName);
			sceneSet = true;

			break;

		case 'L':
			if (!SDL_EnumerateDirectory(shaderDir, log_scenes, NULL)) {
				SDL_Log("Couldn't list scenes: %s\n", SDL_GetError());
				return false;
			}
			return false;

		case '?':
			return false;

		default:
			return false;
		}
	}

	SDL_snprintf(vertShaderPathBuf, PATH_SIZE, "%s/%s", shaderDir, VERT_SHADER_NAME);
	if (!sceneSet) { // TODO: Check if it's a file or directory
		int count = 0;
		char** files = SDL_GlobDirectory(shaderDir, "*" SHADER_EXT, SDL_GLOB_CASEINSENSITIVE, &count);
		if (!files) {
			SDL_Log("Couldn't fetch scenes: %s\n", SDL_GetError());
			return false;
		}
		
		SDL_snprintf(fragShaderPathBuf, PATH_SIZE, "%s/%s", shaderDir, files[SDL_rand(count)]);

		SDL_free(files);
	}

	if (optind < argc) {
		*musicPath = argv[optind];
	} else {
		SDL_Log("Usage: %s [OPTIONS] <mp3 file>\n"
			"run '%s -h' for help\n", argv[0], argv[0]);
		return false;
	}

	return true;
}
