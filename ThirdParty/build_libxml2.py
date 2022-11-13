import os
from pathlib import Path
from subprocess import check_call

from fmpy.util import download_file
from fmpy import extract


archive = download_file('https://github.com/GNOME/libxml2/archive/refs/tags/v2.10.3.zip')

root = Path(__file__).parent

extract(archive, root)

build_dir = root / 'libxml2-2.10.3' / 'windows-x64'

install_prefix = build_dir / 'install'

check_call([
    'cmake',
    '-B', build_dir,
    '-G', 'Visual Studio 17 2022',
    '-A', 'x64',
    '-D', 'CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded',
    '-D', f'CMAKE_INSTALL_PREFIX={ install_prefix }',
    '-D', 'BUILD_SHARED_LIBS=OFF',
    '-D', 'LIBXML2_WITH_ICONV=OFF',
    '-D', 'LIBXML2_WITH_LZMA=OFF',
    '-D', 'LIBXML2_WITH_PYTHON=OFF',
    '-D', 'LIBXML2_WITH_ZLIB=OFF',
    root / 'libxml2-2.10.3'
])

check_call([
    'cmake',
    '--build', build_dir,
    '--config', 'Release',
    '--target', 'INSTALL'
])

os.remove(archive)
