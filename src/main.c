#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <math.h>
#include <mpg123.h>

#include "shader.h"

#define BUFFER_SIZE 4096

typedef struct
{
	float x, y, z;
} PositionVertex;

typedef struct
{
        float width, height;
        float time;
        float peak_amp;
        float avg_amp;
} uniformBlock;

static SDL_Window *window = NULL;
static Sint32 winWidth = 1000;
static Sint32 winHeight = 1000;
static bool fullscreen = false;

static SDL_GPUDevice* gpuDevice;

static SDL_GPUBuffer* vertexBuffer;
static SDL_GPUBuffer* indexBuffer;
static SDL_GPUGraphicsPipeline* graphicsPipeline;

static double time = 0;
static float peak_amp = 0;
static float avg_amp = 0;

static mpg123_handle *mh = NULL;
static SDL_AudioStream *stream = NULL;

// Audio decoding callback
static void SDLCALL audio_stream_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) 
{
        unsigned char buffer[BUFFER_SIZE];
        size_t done;

        int err = mpg123_read(mh, buffer, BUFFER_SIZE, &done);
        if (err == MPG123_OK || err == MPG123_DONE) {
                if (done > 0) {
                        SDL_PutAudioStreamData(stream, buffer, done);
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
                float abs_amp = fabs(buffer[i]);
                avg_amplitude += abs_amp;
                if (abs_amp > peak_amplitude) {
                        peak_amplitude = fabs(buffer[i]);
                }
        }
        avg_amplitude /= num_samples;

        avg_amp = avg_amplitude;
        peak_amp = peak_amplitude;
}

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

	if (!(window = SDL_CreateWindow("FreeVisualizert", winWidth, winHeight, SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_FOCUS))) {
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

        gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
	if (!gpuDevice) {
		SDL_Log("Couldn't create gpuDevice: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_Log("GpuDeviceDriver => %s", SDL_GetGPUDeviceDriver(gpuDevice));

	if (!SDL_ClaimWindowForGPUDevice(gpuDevice, window))
	{
		SDL_Log("GPUClaimWindow failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* vertShader = LoadShader(gpuDevice, "simple.vert", 0, 0, 0, 0);
	if (!vertShader) {
		SDL_Log("Failed to create vertex shader: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* fragShader = LoadShader(gpuDevice, "simple.frag", 0, 1, 0, 0);
	if (!fragShader) {
		SDL_Log("Failed to create fragment shader: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
		.target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(gpuDevice, window),
			}},
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(PositionVertex)
			}},
			.num_vertex_attributes = 1,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 0,
				.offset = 0
			}}
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                .vertex_shader = vertShader,
		.fragment_shader = fragShader
	};

	graphicsPipeline = SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineCreateInfo);
	if (!graphicsPipeline) {
		SDL_Log("Failed to create graphics pipeline: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_ReleaseGPUShader(gpuDevice, vertShader);
	SDL_ReleaseGPUShader(gpuDevice, fragShader);

	vertexBuffer = SDL_CreateGPUBuffer(
		gpuDevice,
		&(SDL_GPUBufferCreateInfo) {
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = sizeof(PositionVertex) * 4
		}
	);

	indexBuffer = SDL_CreateGPUBuffer(
		gpuDevice,
		&(SDL_GPUBufferCreateInfo) {
			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = sizeof(Uint16) * 6
		}
	);

	SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
		gpuDevice,
		&(SDL_GPUTransferBufferCreateInfo) {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (sizeof(PositionVertex) * 4) + (sizeof(Uint16) * 6)
		}
	);

	PositionVertex* transferData = SDL_MapGPUTransferBuffer(
		gpuDevice,
		transferBuffer,
		false
	);

	transferData[0] = (PositionVertex) { -1, -1, 0 };
	transferData[1] = (PositionVertex) { 1, -1, 0 };
	transferData[2] = (PositionVertex) { -1, 1, 0 };
	transferData[3] = (PositionVertex) { 1, 1, 0 };

	Uint16* indexData = (Uint16*) &transferData[4];
        indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 2;
	indexData[4] = 1;
	indexData[5] = 3;

	SDL_UnmapGPUTransferBuffer(gpuDevice, transferBuffer);

	SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);


	SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transferBuffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = vertexBuffer,
			.offset = 0,
			.size = sizeof(PositionVertex) * 4
		},
		false
	);

	SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transferBuffer,
			.offset = sizeof(PositionVertex) * 4
		},
		&(SDL_GPUBufferRegion) {
			.buffer = indexBuffer,
			.offset = 0,
			.size = sizeof(Uint16) * 6
		},
		false
	);

	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
	SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);
        
        if (mpg123_init() != MPG123_OK) {
		SDL_Log("Couldn't initialize mpg123");
                return SDL_APP_FAILURE;
        }

        mh = mpg123_new(NULL, NULL);
        if (!mh) {
		SDL_Log("Couldn't create mpg123 handle");
                return SDL_APP_FAILURE;
        }

        if (mpg123_open(mh, music_path) != MPG123_OK) {
		SDL_Log("Couldn't open mp3 file: %s", music_path);
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

        stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_stream_callback, NULL);
        if (!stream) {
                SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
                return SDL_APP_FAILURE;
        }
        SDL_SetAudioPostmixCallback(SDL_GetAudioStreamDevice(stream), audio_proccess_callback, NULL);

        SDL_ResumeAudioStreamDevice(stream);

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
		} else if (event->key.scancode == SDL_SCANCODE_SPACE) {
                        if (SDL_AudioStreamDevicePaused(stream)) {
                                SDL_ResumeAudioStreamDevice(stream);
                        } else {
                                SDL_PauseAudioStreamDevice(stream);
                        }
                } else if (event->key.scancode == SDL_SCANCODE_F) {
                        fullscreen = !fullscreen;
                        SDL_SetWindowFullscreen(window, fullscreen);
                }
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
        time = ((double) SDL_GetTicks()) / 1000.0;
        SDL_GetWindowSizeInPixels(window, &winWidth, &winHeight);

	SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(gpuDevice);
	if (cmdbuf == NULL)
	{
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTexture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, window, &swapchainTexture, NULL, NULL)) {
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

		SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline);
		SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding){ .buffer = vertexBuffer, .offset = 0 }, 1);
		SDL_BindGPUIndexBuffer(renderPass, &(SDL_GPUBufferBinding){ .buffer = indexBuffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT);

                // Uploading uniforms
                SDL_PushGPUFragmentUniformData(cmdbuf, 0, &(uniformBlock) {winWidth, winHeight, time, peak_amp, avg_amp} , sizeof(uniformBlock));

		SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(cmdbuf);
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        SDL_DestroyAudioStream(stream);
}
