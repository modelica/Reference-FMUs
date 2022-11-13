#ifdef _WIN32
#include <direct.h>
#include <errno.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <process.h>
#include <strsafe.h>
#endif

#include "miniunzip.h"

#include "FMIZip.h"


const char* FMICreateTemporaryDirectory() {
    
    const char* tempfile = _tempnam(NULL, NULL);

    if (!tempfile) {
        return NULL;
    }

#ifdef _WIN32
    BOOL res = CreateDirectoryA(tempfile, 0);

    if (!PathFileExistsA(tempfile)) {
        free((void*)tempfile);
        return NULL;
    }
#else
    // TODO  
#endif

    return tempfile;
}

int FMIPathAppend(char* path, const char* more) {
#ifdef _WIN32
    return PathAppendA(path, "modelDescription.xml");
#endif
}

int FMIExtractArchive(const char* filename, const char* unzipdir) {

    char cd[2048] = "";

#ifdef _WIN32
    if (!_getcwd(cd, 2048)) {
        return 1;
    }
#else
    // TODO  
#endif

    const char* argv[6] = { "miniunz", "-x", "-o", filename, "-d", unzipdir };

    const int status = miniunz_main(6, argv);

#ifdef _WIN32
    if (_chdir(cd)) {
        return 1;
    }
#else
    // TODO  
#endif

    return status;
}

int FMIRemoveDirectory(const char* path) {

    char command[4096];

#ifdef _WIN32
    snprintf(command, 4096, "rmdir /s /q \"%s\"", path);
#else
    snprintf(command, 4096, "rm -rf \"%s\"", path);
#endif

    return system(command);
}
