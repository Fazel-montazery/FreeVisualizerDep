#include "opts.h"

#define OP_STRING "hs"

static const struct option opts[] = {
	{"help", no_argument, 0, 'h'},
	{"scene", 1, 0, 's'},
	{0, 0, 0, 0}
};

bool parseOpts(int argc, char *argv[], OptState* res)
{
	int opt;
	int indx = 0;

	while ((opt = getopt_long(argc, argv, OP_STRING, opts, &indx)) != -1) {
		switch (opt) {
		case 'h':
			SDL_Log("Usage: %s [OPTIONS] <mp3 file>\n\n"
					"Options:\n", argv[0]);
			SDL_Log("  %-20s%s\n", "-h, --help", "Print this help message");
			SDL_Log("  %-20s%s\n", "-s, --scene", "Which scene(shader) to use");
			return false;

		case 's':
			break;

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
