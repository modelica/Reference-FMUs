from pathlib import Path
from subprocess import check_call
import tarfile
from fmpy import platform_tuple
from fmpy.util import download_file


root = Path(__file__).parent

archive = download_file('https://github.com/LLNL/sundials/releases/download/v6.4.1/cvode-6.4.1.tar.gz',
                        checksum='0a614e7d7d525d9ec88d5a30c887786d7c3681bd123bb6679fb9a4ec5f4609fe')

with tarfile.open(archive) as file:
    file.extractall(root)


if platform_tuple == 'x86_64-windows':
    cmake_args = [
        '-G', 'Visual Studio 16 2019',
        '-A', 'x64'
    ]
else:
    cmake_args = [] 

build_dir = root / f'cvode-{platform_tuple}' / 'build'
install_prefix = root / f'cvode-{platform_tuple}' / 'install'

check_call(
    ['cmake'] +
    cmake_args +
    ['-B', build_dir,
    '-D', f'BUILD_SHARED_LIBS=OFF',
    '-D', f'BUILD_TESTING=OFF',
    '-D', f'EXAMPLES_INSTALL=OFF',
    '-D', f'CMAKE_INSTALL_PREFIX={ install_prefix }',
    str(root / 'cvode-6.4.1')]
)

check_call([
    'cmake',
    '--build', str(build_dir),
    '--config', 'Release',
    '--target', 'install'
])
