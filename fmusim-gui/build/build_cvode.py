from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
import tarfile


archive = download_file('https://github.com/LLNL/sundials/releases/download/v6.4.1/cvode-6.4.1.tar.gz',
                        checksum='0a614e7d7d525d9ec88d5a30c887786d7c3681bd123bb6679fb9a4ec5f4609fe')

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
