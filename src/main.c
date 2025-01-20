#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <mpg123.h>

static SDL_Window *window = NULL;
static const Uint32 winWidth = 1000;
static const Uint32 winHeight = 1000;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	SDL_SetAppMetadata("FreeVisualizer", "1.0", "com.free.vis");

        if (argc != 2) {
		SDL_Log("Usage: %s <mp3 file>", argv[0]);
		return SDL_APP_FAILURE;
        }

        const char* music_path = argv[1];

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!(window = SDL_CreateWindow("FreeVisualizert", winWidth, winHeight, 0))) {
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
        
        if (mpg123_init() != MPG123_OK) {
		SDL_Log("Couldn't initialize mpg123");
                return SDL_APP_FAILURE;
        }

        mpg123_handle *mh = mpg123_new(NULL, NULL);
        if (!mh) {
		SDL_Log("Couldn't create mpg123 handle");
                return SDL_APP_FAILURE;
        }

        if (mpg123_open(mh, music_path) != MPG123_OK) {
		SDL_Log("Couldn't open mp3 file: %s", music_path);
                mpg123_delete(mh);
                return 1;
        }

        long rate;
        int channels, encoding;
        if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
		SDL_Log("Couldn't get audio format");
                mpg123_close(mh);
                mpg123_delete(mh);
                return 1;
        }

        SDL_Log("Audio format: %ld Hz, %d channels, encoding: %d\n", rate, channels, encoding);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	if (event->type == SDL_EVENT_KEY_DOWN) {
		if (event->key.scancode == SDL_SCANCODE_Q || event->key.scancode == SDL_SCANCODE_ESCAPE) {
			return SDL_APP_SUCCESS;
		}
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
        mpg123_exit();
}
