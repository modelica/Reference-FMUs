import os
from pathlib import Path
from subprocess import check_call

from fmpy.util import download_file
import tarfile

archive = download_file('https://github.com/LLNL/sundials/releases/download/v6.4.1/cvode-6.4.1.tar.gz',
                        checksum='0a614e7d7d525d9ec88d5a30c887786d7c3681bd123bb6679fb9a4ec5f4609fe')

root = Path(__file__).parent

with tarfile.open(archive) as file:
    file.extractall(root)

build_dir = root / 'cvode-6.4.1' / 'build'

install_prefix = build_dir / 'install'

args = []

if os.name == 'nt':
    args = [
        '-G', 'Visual Studio 17 2022',
        '-A', 'x64',
        '-D', 'CMAKE_C_FLAGS_DEBUG=/MT /Zi /Ob0 /Od /RTC1',
        '-D', 'CMAKE_C_FLAGS_RELEASE=/MT /O2 /Ob2 /DNDEBUG'
    ]

check_call(
    ['cmake'] +
    args +
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

os.remove(archive)
