from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
from fmpy import extract
import argparse


parser = argparse.ArgumentParser()
parser.add_argument(
    'platform',
    choices={'x86-windows', 'x86_64-windows', 'x86_64-linux', 'aarch64-linux', 'x86_64-darwin', 'aarch64-darwin'},
    help="Platform to build for, e.g. x86_64-windows"
)
parser.add_argument(
    '--cmake-generator',
    choices={'Visual Studio 17 2022', 'Visual Studio 16 2019'},
    default='Visual Studio 17 2022',
    help="CMake generator for Windows"
)
(args, _) = parser.parse_known_args()

archive = download_file('https://github.com/GNOME/libxml2/archive/refs/tags/v2.11.5.zip',
                        checksum='711675470075cc85ba450f56aff7424f1ecdef00bc5d1d5dced3ffecd1a9b772')

root = Path(__file__).parent

extract(archive, root)

build_dir = root / f'libxml2-{args.platform}' / 'build'

install_prefix = root / f'libxml2-{args.platform}' / 'install'

cmake_args = []

fmi_platform = args.platform
fmi_architecture, fmi_system = fmi_platform.split('-')

if fmi_system == 'windows':

    cmake_args = [
        '-G', args.cmake_generator,
        '-D', 'CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded',
    ]

    if fmi_architecture == 'x86':
        cmake_args += ['-A', 'Win32']
    elif fmi_architecture == 'x86_64':
        cmake_args += ['-A', 'x64']

elif fmi_platform == 'aarch64-linux':

    toolchain_file = root / 'aarch64-linux-toolchain.cmake'
    cmake_args += ['-D', f'CMAKE_TOOLCHAIN_FILE={toolchain_file}']

elif fmi_platform == 'x86_64-darwin':

    cmake_args += ['-D', 'CMAKE_OSX_ARCHITECTURES=x86_64']

elif fmi_platform == 'aarch64-darwin':

    cmake_args += ['-D', 'CMAKE_OSX_ARCHITECTURES=arm64']

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
