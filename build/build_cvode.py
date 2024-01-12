from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
import tarfile
import argparse


parser = argparse.ArgumentParser()
parser.add_argument(
    'platform',
    choices={'x86-windows', 'x86_64-windows', 'x86_64-linux', 'aarch64-linux', 'x86_64-darwin', 'aarch64-darwin'},
    help="Platform to build for, e.g. x86_64-windows"
)
args, _ = parser.parse_known_args()

archive = download_file('https://github.com/LLNL/sundials/releases/download/v6.4.1/cvode-6.4.1.tar.gz',
                        checksum='0a614e7d7d525d9ec88d5a30c887786d7c3681bd123bb6679fb9a4ec5f4609fe')

root = Path(__file__).parent

with tarfile.open(archive) as file:
    file.extractall(root)

build_dir = root / f'cvode-{args.platform}' / 'build'

install_prefix = root / f'cvode-{args.platform}' / 'install'

cmake_args = []

fmi_platform = args.platform
fmi_architecture, fmi_system = fmi_platform.split('-')

if fmi_system == 'windows':

    cmake_args = [
        '-G', 'Visual Studio 17 2022',
        '-D', 'CMAKE_C_FLAGS_RELEASE=/MT /O2 /Ob2 /DNDEBUG',
        '-D', 'CMAKE_C_FLAGS_DEBUG=/MT /Zi /Ob0 /Od /RTC1',
        '-A'
    ]

    if fmi_architecture == 'x86':
        cmake_args.append('Win32')
    elif fmi_architecture == 'x86_64':
        cmake_args.append('x64')

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
    '-D', f'BUILD_SHARED_LIBS=OFF',
    '-D', f'BUILD_TESTING=OFF',
    '-D', f'EXAMPLES_INSTALL=OFF',
    '-D', f'CMAKE_INSTALL_PREFIX={ install_prefix }',
    root / 'cvode-6.4.1']
)

check_call([
    'cmake',
    '--build', build_dir,
    '--config', 'Release',
    '--target', 'install'
])
