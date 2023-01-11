#pragma once

#include <stdbool.h>

int FMIPathAppend(char* path, const char* more);

const char* FMICreateTemporaryDirectory();

int FMIExtractArchive(const char* filename, const char* unzipdir);

int FMIRemoveDirectory(const char* path);
