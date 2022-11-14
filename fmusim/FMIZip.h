#ifndef FMI_ZIP_H
#define FMI_ZIP_H

/**************************************************************
 *  Copyright (c) Modelica Association Project "FMI".         *
 *  All rights reserved.                                      *
 *  This file is part of the Reference FMUs. See LICENSE.txt  *
 *  in the project root for license information.              *
 **************************************************************/

#include <stdbool.h>

int FMIPathAppend(char* path, const char* more);

const char* FMICreateTemporaryDirectory();

int FMIExtractArchive(const char* filename, const char* unzipdir);

int FMIRemoveDirectory(const char* path);

#endif  // FMI_ZIP_H