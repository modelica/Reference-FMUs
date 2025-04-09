from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
import tarfile
import argparse


cvode_version = '7.3.0'

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
args, _ = parser.parse_known_args()

archive = download_file(f'https://github.com/LLNL/sundials/releases/download/v{cvode_version}/cvode-{cvode_version}.tar.gz',
                        checksum='8b15a646882f2414b1915cad4d53136717a077539e7cfc480f2002c5898ae568')

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
        '-G', args.cmake_generator,
        '-D', 'CMAKE_POLICY_DEFAULT_CMP0091=NEW',
        '-D', 'CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'
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
    '-D', 'BUILD_SHARED_LIBS=OFF',
    '-D', 'BUILD_TESTING=OFF',
    '-D', 'EXAMPLES_INSTALL=OFF',
    '-D', 'SUNDIALS_ENABLE_ERROR_CHECKS=OFF',
    '-D', f'CMAKE_INSTALL_PREFIX={ install_prefix }',
    root / f'cvode-{cvode_version}']
)

check_call([
    'cmake',
    '--build', build_dir,
    '--config', 'Release',
    '--target', 'install'
])
