#ifndef FMI_ZIP_H
#define FMI_ZIP_H

#include <stdbool.h>

int FMIPathAppend(char* path, const char* more);

const char* FMICreateTemporaryDirectory();

int FMIExtractArchive(const char* filename, const char* unzipdir);

int FMIRemoveDirectory(const char* path);

#endif  // FMI_ZIP_H