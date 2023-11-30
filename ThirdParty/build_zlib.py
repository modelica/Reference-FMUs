import os
import tarfile
from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--build-for-aarch64', type=bool)
args, _ = parser.parse_known_args()

archive = download_file('https://www.zlib.net/fossils/zlib-1.3.tar.gz',
                        checksum='ff0ba4c292013dbc27530b3a81e1f9a813cd39de01ca5e0f8bf355702efa593e')

root = Path(__file__).parent

with tarfile.open(archive) as tf:
    tf.extractall(root)

build_dir = root / 'zlib-1.3' / 'build'

install_prefix = build_dir / 'install'

toolchain_file = root.parent / 'aarch64-toolchain.cmake'

cmake_args = []

if os.name == 'nt':
    cmake_args = [
        '-G', 'Visual Studio 17 2022',
        '-A', 'x64',
        # '-D', 'CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'
        '-D', 'CMAKE_C_FLAGS_DEBUG=/MT /Zi /Ob0 /Od /RTC1',
        '-D', 'CMAKE_C_FLAGS_RELEASE=/MT /O2 /Ob2 /DNDEBUG'
    ]


if args.build_for_arm:
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

os.remove(archive)
