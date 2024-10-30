from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
import tarfile


archive = download_file('https://github.com/LLNL/sundials/releases/download/v7.1.1/cvode-7.1.1.tar.gz',
                        checksum='36eb0ccea5e223ff4fbc528ef996bfb292ec8a1238019b929290ae5d444520ff')

root = Path(__file__).parent

build_dir = root / 'cvode-x86_64-windows' / 'build'
install_prefix = root / 'cvode-x86_64-windows' / 'install'

with tarfile.open(archive) as file:
    file.extractall(root)

cmake_args = [
    '-G', 'Visual Studio 16 2019',
    '-A', 'x64'
]

check_call(
    ['cmake'] +
    cmake_args +
    ['-B', build_dir,
    '-D', 'BUILD_SHARED_LIBS=OFF',
    '-D', 'BUILD_TESTING=OFF',
    '-D', 'EXAMPLES_INSTALL=OFF',
    '-D', 'SUNDIALS_ENABLE_ERROR_CHECKS=OFF',
    '-D', f'CMAKE_INSTALL_PREFIX={ install_prefix }',
    str(root / 'cvode-7.1.1')]
)

check_call([
    'cmake',
    '--build', str(build_dir),
    '--config', 'Release',
    '--target', 'install'
])
