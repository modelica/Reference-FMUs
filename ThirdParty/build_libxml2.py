import os
from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
from fmpy import extract
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--build-for-aarch64', type=bool)
(args, _) = parser.parse_known_args()

archive = download_file('https://github.com/GNOME/libxml2/archive/refs/tags/v2.11.5.zip',
                        checksum='711675470075cc85ba450f56aff7424f1ecdef00bc5d1d5dced3ffecd1a9b772')

root = Path(__file__).parent

extract(archive, root)

build_dir = root / 'libxml2-2.11.5' / 'build'

install_prefix = build_dir / 'install'

toolchain_file = root.parent / 'aarch64-toolchain.cmake'

cmake_args = []

if os.name == 'nt':
    cmake_args = [
        '-G', 'Visual Studio 17 2022',
        '-A', 'x64',
        '-D', 'CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'
    ]

if args.build_for_arm:
    cmake_args += ['-D', f'CMAKE_TOOLCHAIN_FILE={toolchain_file}']

check_call(
    ['cmake'] +
    cmake_args +
    ['-B', build_dir,
    '-D', f'CMAKE_INSTALL_PREFIX={install_prefix}',
    '-D', 'BUILD_SHARED_LIBS=OFF',
    '-D', 'LIBXML2_WITH_ICONV=OFF',
    '-D', 'LIBXML2_WITH_LZMA=OFF',
    '-D', 'LIBXML2_WITH_PYTHON=OFF',
    '-D', 'LIBXML2_WITH_ZLIB=OFF',
    '-D', 'LIBXML2_WITH_TESTS=OFF',
    root / 'libxml2-2.11.5']
)

check_call([
    'cmake',
    '--build', build_dir,
    '--config', 'Release',
    '--target', 'install'
])

os.remove(archive)
