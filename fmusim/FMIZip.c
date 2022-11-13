#include <errno.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <process.h>
#include <strsafe.h>

#include "miniunzip.h"

#include "FMIZip.h"


const char* FMICreateTemporaryDirectory() {
    
    const char* tempfile = _tempnam(NULL, NULL);

    if (!tempfile) {
        return NULL;
    }

    BOOL res = CreateDirectoryA(tempfile, 0);

    if (!PathFileExistsA(tempfile)) {
        free((void*)tempfile);
        return NULL;
    }

    return tempfile;
}

int FMIPathAppend(char* path, const char* more) {

    return PathAppendA(path, "modelDescription.xml");
}

int FMIExtractArchive(const char* filename, const char* unzipdir) {

    const char* argv[6] = { "miniunz", "-x", "-o", filename, "-d", unzipdir };

    return miniunz_main(6, argv);
}

int FMIRemoveDirectory(const char* path) {

    char command[4096];

    sprintf(command, "rmdir \"%s\" /s /q", path);

    return system(command);
}
