import tarfile
from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
import argparse


zlib_version = '1.3.1'

parser = argparse.ArgumentParser()
parser.add_argument(
    'platform',
    choices={'x86-windows', 'x86_64-windows', 'x86_64-linux', 'aarch64-linux', 'x86_64-darwin', 'aarch64-darwin', 'universal64-darwin'},
    help="Platform to build for, e.g. x86_64-windows"
)
parser.add_argument(
    '--cmake-generator',
    choices={'Visual Studio 17 2022', 'Visual Studio 16 2019'},
    default='Visual Studio 17 2022',
    help="CMake generator for Windows"
)
args, _ = parser.parse_known_args()

archive = download_file(f'https://www.zlib.net/fossils/zlib-{zlib_version}.tar.gz',
                        checksum='9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23')

root = Path(__file__).parent

with tarfile.open(archive) as tf:
    tf.extractall(root)

build_dir = root / f'zlib-{args.platform}' / 'build'

install_prefix = root / f'zlib-{args.platform}' / 'install'

cmake_args = []

fmi_platform = args.platform
fmi_architecture, fmi_system = fmi_platform.split('-')

if fmi_system == 'windows':

    cmake_args += ['-G', args.cmake_generator]

    if fmi_architecture == 'x86':
        cmake_args += ['-A', 'Win32']
    elif fmi_architecture == 'x86_64':
        cmake_args += ['-A', 'x64']

    cmake_args += [
        '-D', 'CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'
        #'-D', 'CMAKE_C_FLAGS_DEBUG=/MT /Zi /Ob0 /Od /RTC1',
        #'-D', 'CMAKE_C_FLAGS_RELEASE=/MT /O2 /Ob2 /DNDEBUG'
    ]

elif fmi_platform == 'aarch64-linux':

    toolchain_file = root / 'aarch64-linux-toolchain.cmake'
    cmake_args += ['-D', f'CMAKE_TOOLCHAIN_FILE={ toolchain_file }']

elif fmi_platform == 'x86_64-darwin':

    cmake_args += ['-D', 'CMAKE_OSX_ARCHITECTURES=x86_64']

elif fmi_platform == 'aarch64-darwin':

    cmake_args += ['-D', 'CMAKE_OSX_ARCHITECTURES=arm64']

elif fmi_platform == 'universal64-darwin':

    cmake_args += ['-D', 'CMAKE_OSX_ARCHITECTURES=arm64;x86_64']

check_call(
    ['cmake'] +
    cmake_args +
    ['-B', build_dir,
    '-D', f'CMAKE_INSTALL_PREFIX={ install_prefix }',
    root / f'zlib-{zlib_version}']
)

check_call([
    'cmake',
    '--build', build_dir,
    '--config', 'Release',
    '--target', 'install'
])
