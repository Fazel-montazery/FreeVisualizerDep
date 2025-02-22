#include "opts.h"

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

static bool fileExists = false; // TODO: WTF IS THIS? GET RID OF GLOBAL
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

#define OP_STRING "hs:LS:d:"

static const struct option opts[] = {
	{"help", no_argument, 0, 'h'},
	{"scene", required_argument, 0, 's'},
	{"ls", no_argument, 0, 'L'},
	{"yt-search", required_argument, 0, 'S'},
	{"yt-dl", required_argument, 0, 'd'},
	{0, 0, 0, 0}
};

#define SEARCH_COMMAND "yt-dlp --no-playlist ytsearch10:'%s official music' --get-title"
#define DOWNLOAD_COMMAND "yt-dlp -f bestaudio --no-playlist --extract-audio --audio-format mp3 --audio-quality 0 -o '%%(title)s' -P '%s' 'ytsearch: %s official music'"

static const char loadingChars[] = {'/', '-', '\\'};
static int loadingCharIndx = 0;
static volatile bool loading_should_exit = false;
static int SDLCALL loading(void *data)
{
	loading_should_exit = false;
	while (!loading_should_exit) {
		fputc(loadingChars[loadingCharIndx % 3], stdout);
		fputc('\r', stdout);
		fflush(stdout);
		SDL_Delay(200);
		loadingCharIndx++;
	}
	return 0;
}

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

	char musicDir[PATH_SIZE] = { 0 };
	SDL_snprintf(musicDir, PATH_SIZE, "%s%s/%s", home, DATA_DIR, MUSIC_DIR);

	if (!SDL_CreateDirectory(musicDir)) {
		SDL_Log("Couldn't retrive/create music directory: %s\n", SDL_GetError());
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
			SDL_Log("  %-20s%s\n", "-L, --ls", "List scenes");
			SDL_Log("  %-20s%s\n", "-S, --yt-search", "Search youtube and return 10 results");
			SDL_Log("  %-20s%s\n", "-d, --yt-dl", "Download the audio of a YouTube video by title");
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

		case 'S':
			SDL_Thread* loadingThread =  SDL_CreateThread(loading, "loading", NULL);

			char search[PATH_SIZE] = { 0 };
			SDL_snprintf(search, PATH_SIZE, SEARCH_COMMAND, optarg);

			if (system(search) == -1) {
				SDL_Log("Couldn't search youtube: %s\n", strerror(errno));
			}

			loading_should_exit = true;
			SDL_WaitThread(loadingThread, NULL);
			return false;

		case 'd':
			char download[PATH_SIZE] = { 0 };
			SDL_snprintf(download, PATH_SIZE, DOWNLOAD_COMMAND, musicDir, optarg);

			if (system(download) == -1) {
				SDL_Log("Couldn't download from youtube: %s\n", strerror(errno));
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
