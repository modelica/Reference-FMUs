#ifdef _WIN32
#include <direct.h>
#include <errno.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <process.h>
#include <strsafe.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "miniunzip.h"

#include "FMIZip.h"


const char* FMICreateTemporaryDirectory() {
    
#ifdef _WIN32
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
#else
    char template[1024] = "/tmp/fmusim.XXXXXX";
    const char* tempdir = mkdtemp(template);
    const char* tempdir2 = strdup(tempdir);
    return tempdir2;
#endif
}

int FMIPathAppend(char* path, const char* more) {
#ifdef _WIN32
    return PathAppendA(path, "modelDescription.xml");
#else
    sprintf(path, "%s/%s", path, more);
    return 1;
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

    const char* argv[6];
    
    argv[0] = "miniunz";
    argv[1] = "-x";
    argv[2] = "-o";
    argv[3] = filename;
    argv[4] = "-d";
    argv[5] = unzipdir;

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
