#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>

const char* getHomeDir();
void getConfigPath(const char* homeDir, const char* configName, char dest[], size_t destsiz);
void getDataDir(const char* homeDir, const char* dataDirName, char dest[], size_t destsiz);
