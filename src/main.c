#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <mpg123.h>

#include "defs.h"
#include "shader.h"
#include "opts.h"

typedef struct
{
	float width, height;
	float time;
	float peak_amp;
	float avg_amp;
} uniformBlock;

typedef struct
{
	// Windowing
	SDL_Window* window;
	Sint32 winWidth;
	Sint32 winHeight;
	bool fullscreen;

	// GPU
	SDL_GPUDevice* gpuDevice;
	SDL_GPUGraphicsPipeline* graphicsPipeline;
	char fragShaderPath[PATH_SIZE];
	char vertShaderPath[PATH_SIZE];

	// Audio
	SDL_AudioStream* stream;

	// Music path
	char* musicPath;
} State;

static double time = 0;
static float peak_amp = 0;
static float avg_amp = 0;

static mpg123_handle *mh = NULL;

// If we want cleanup at exit
static bool cleanUp = true;

// Is music still playing
static bool musicDone = false;

// Audio decoding callback
static void SDLCALL audio_stream_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	unsigned char buffer[MUSIC_BUFFER_SIZE];
	size_t done;

	int err = mpg123_read(mh, buffer, MUSIC_BUFFER_SIZE, &done);
	if (err == MPG123_OK || err == MPG123_DONE) {
		if (done > 0) {
			SDL_PutAudioStreamData(stream, buffer, done);
		} else {
			musicDone = true;
		}
	} else {
		SDL_Log("Error decoding MP3: %s", mpg123_strerror(mh));
	}
}

// Audio proccessing callback
static void SDLCALL audio_proccess_callback(void *userdata, const SDL_AudioSpec *spec, float *buffer, int buflen)
{
	int num_samples = buflen / sizeof(float);
	float peak_amplitude = 0.0f;
	float avg_amplitude = 0.0f;
	for (int i = 0; i < num_samples; ++i) {
		float abs_amp = SDL_fabs(buffer[i]);
		avg_amplitude += abs_amp;
		if (abs_amp > peak_amplitude) {
			peak_amplitude = SDL_fabs(buffer[i]);
		}
	}
	avg_amplitude /= num_samples;

	avg_amp = avg_amplitude;
	peak_amp = peak_amplitude;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	SDL_SetAppMetadata("FreeVisualizer", "1.0", "com.free.vis");

	// Initializing app state
	State state = DEFAULT_STATE;
	State* statep = SDL_malloc(sizeof(State));
	if (!statep) {
		SDL_Log("Couldn't allocate memory for the state of the app: %s", SDL_GetError());
		cleanUp = false;
		return SDL_APP_FAILURE;
	}
	*appstate = statep;

	// Handling cli arguments
	if (!parseOpts(argc, argv, &state.musicPath, state.fragShaderPath, state.vertShaderPath, PATH_SIZE, &state.fullscreen)) {
		cleanUp = false;
		return SDL_APP_SUCCESS;
	}

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		cleanUp = false;
		return SDL_APP_FAILURE;
	}

	SDL_WindowFlags windowFlags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_FOCUS;
	if (state.fullscreen) windowFlags |= SDL_WINDOW_FULLSCREEN;
	if (!(state.window = SDL_CreateWindow("FreeVisualizer", state.winWidth, state.winHeight, windowFlags))) {
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	state.gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
	if (!state.gpuDevice) {
		SDL_Log("Couldn't create gpuDevice: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_Log("GpuDeviceDriver => %s", SDL_GetGPUDeviceDriver(state.gpuDevice));

	if (!SDL_ClaimWindowForGPUDevice(state.gpuDevice, state.window))
	{
		SDL_Log("Couldn't assign window to GPU: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* vertShader = loadShader(state.gpuDevice, SDL_GPU_SHADERSTAGE_VERTEX, state.vertShaderPath, 0, 0, 0, 0);
	if (!vertShader) {
		SDL_Log("Couldn't create vertex shader: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* fragShader = loadShader(state.gpuDevice,SDL_GPU_SHADERSTAGE_FRAGMENT,  state.fragShaderPath, 0, 1, 0, 0);
	if (!fragShader) {
		SDL_Log("Couldn't create fragment shader: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
		.target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(state.gpuDevice, state.window),
			}},
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
		.vertex_shader = vertShader,
		.fragment_shader = fragShader
	};

	state.graphicsPipeline = SDL_CreateGPUGraphicsPipeline(state.gpuDevice, &pipelineCreateInfo);
	if (!state.graphicsPipeline) {
		SDL_Log("Couldn't create graphics pipeline: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_ReleaseGPUShader(state.gpuDevice, vertShader);
	SDL_ReleaseGPUShader(state.gpuDevice, fragShader);

	if (mpg123_init() != MPG123_OK) {
		SDL_Log("Couldn't initialize mpg123");
		return SDL_APP_FAILURE;
	}

	mh = mpg123_new(NULL, NULL);
	if (!mh) {
		SDL_Log("Couldn't create mpg123 handle");
		return SDL_APP_FAILURE;
	}

	if (mpg123_open(mh, state.musicPath) != MPG123_OK) {
		SDL_Log("Couldn't open mp3 file: %s", state.musicPath);
		return 1;
	}

	long rate;
	int channels, encoding;
	if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
		SDL_Log("Couldn't get audio format");
		return 1;
	}

	SDL_Log("Music format: %ld Hz, %d channels, encoding: %d\n", rate, channels, encoding);

	SDL_AudioSpec spec = {
		.channels = channels,
		.format = SDL_AUDIO_S16,
		.freq = rate
	};

	state.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_stream_callback, NULL);
	if (!state.stream) {
		SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_SetAudioPostmixCallback(SDL_GetAudioStreamDevice(state.stream), audio_proccess_callback, NULL);

	SDL_ResumeAudioStreamDevice(state.stream);

	// Assigning state to sdl appstate
	*statep = state;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	State* state = appstate;

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	if (event->type == SDL_EVENT_KEY_DOWN) {
		if (event->key.scancode == SDL_SCANCODE_Q || event->key.scancode == SDL_SCANCODE_ESCAPE) {
			return SDL_APP_SUCCESS;
		} else if (event->key.scancode == SDL_SCANCODE_SPACE) {
			if (SDL_AudioStreamDevicePaused(state->stream)) {
				SDL_ResumeAudioStreamDevice(state->stream);
			} else {
				SDL_PauseAudioStreamDevice(state->stream);
			}
		} else if (event->key.scancode == SDL_SCANCODE_F) {
			state->fullscreen = !state->fullscreen;
			SDL_SetWindowFullscreen(state->window, state->fullscreen);
		}
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	if (musicDone) return SDL_APP_SUCCESS;

	State* state = appstate;

	time = ((double) SDL_GetTicks()) / 1000.0;
	SDL_GetWindowSizeInPixels(state->window, &state->winWidth, &state->winHeight);

	SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(state->gpuDevice);
	if (cmdbuf == NULL)
	{
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTexture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, state->window, &swapchainTexture, NULL, NULL)) {
		SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != NULL)
	{
		SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
		colorTargetInfo.texture = swapchainTexture;
		colorTargetInfo.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
		colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);
		SDL_BindGPUGraphicsPipeline(renderPass, state->graphicsPipeline);

		// Uploading uniforms
		SDL_PushGPUFragmentUniformData(cmdbuf, 0, &(uniformBlock) {state->winWidth, state->winHeight, time, peak_amp, avg_amp} , sizeof(uniformBlock));

		SDL_DrawGPUPrimitives(renderPass, 4, 1, 0, 0);

		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(cmdbuf);
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	if (cleanUp) {
		State* state = appstate;

		mpg123_close(mh);
		mpg123_delete(mh);
		mpg123_exit();
		SDL_DestroyAudioStream(state->stream);

		SDL_ReleaseGPUGraphicsPipeline(state->gpuDevice, state->graphicsPipeline);
		SDL_ReleaseWindowFromGPUDevice(state->gpuDevice, state->window);
		SDL_DestroyGPUDevice(state->gpuDevice);
		SDL_DestroyWindow(state->window);

		SDL_free(state);
	}
}
