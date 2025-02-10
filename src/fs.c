#include "fs.h"

const char* getHomeDir()
{
	const char* home = getenv("HOME");
	if (!home) {
		struct passwd* pw = getpwuid(getuid());
		if (!pw) return NULL;
		home = pw->pw_dir;
	}
	return home;
}

void getConfigPath(const char* homeDir, const char* configName, char dest[], size_t destsiz)
{
	snprintf(dest, destsiz, "%s/%s", homeDir, configName);
}

static void _getDataDir(const char* homeDir, const char* dataDirName, char dest[], size_t destsiz, int i)
{
	if (i == 2) {
		return;
	}

	snprintf(dest, destsiz, "%s/%s", homeDir, dataDirName);
	struct stat st;
	if (stat(dest, &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			return;
		} else {
			remove(dest);
			_getDataDir(homeDir, dataDirName, dest, destsiz, i + 1);
			return;
		}
	} else {
		if (mkdir(dest, 0777) == 0) {
			return;
		} else {
			_getDataDir(homeDir, dataDirName, dest, destsiz, i + 1);
			return;
		}
	}
}

void getDataDir(const char* homeDir, const char* dataDirName, char dest[], size_t destsiz)
{
	_getDataDir(homeDir, dataDirName, dest, destsiz, 0);
}
