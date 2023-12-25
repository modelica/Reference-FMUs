import os
import tarfile
from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
import argparse

parser = argparse.ArgumentParser()
parser.add_argument(
    'platform',
    choices={'x86-windows', 'x86_64-windows', 'x86_64-linux', 'aarch64-linux', 'x86_64-darwin'},
    help="Platform to build for, e.g. x86_64-windows"
)
args, _ = parser.parse_known_args()

archive = download_file('https://www.zlib.net/fossils/zlib-1.3.tar.gz',
                        checksum='ff0ba4c292013dbc27530b3a81e1f9a813cd39de01ca5e0f8bf355702efa593e')

root = Path(__file__).parent

with tarfile.open(archive) as tf:
    tf.extractall(root)

build_dir = root / f'zlib-{args.platform}' / 'build'

install_prefix = root / f'zlib-{args.platform}' / 'install'

cmake_args = []

if args.platform in {'x86-windows', 'x86_64-windows'}:

    cmake_args += ['-G', 'Visual Studio 17 2022', '-A']

    if args.platform == 'x86_64-windows':
        cmake_args.append('x64')
    elif args.platform == 'x86-windows':
        cmake_args.append('Win32')

    cmake_args += [
        '-D', 'CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'
        #'-D', 'CMAKE_C_FLAGS_DEBUG=/MT /Zi /Ob0 /Od /RTC1',
        #'-D', 'CMAKE_C_FLAGS_RELEASE=/MT /O2 /Ob2 /DNDEBUG'
    ]


elif args.platform == 'aarch64-linux':

    toolchain_file = root.parent / 'aarch64-linux-toolchain.cmake'
    cmake_args += ['-D', f'CMAKE_TOOLCHAIN_FILE={ toolchain_file }']

check_call(
    ['cmake'] +
    cmake_args +
    ['-B', build_dir,
    '-D', f'CMAKE_INSTALL_PREFIX={ install_prefix }',
    root / 'zlib-1.3']
)

check_call([
    'cmake',
    '--build', build_dir,
    '--config', 'Release',
    '--target', 'install'
])
