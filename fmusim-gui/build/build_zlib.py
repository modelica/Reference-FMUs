import tarfile
from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file


archive = download_file('https://www.zlib.net/fossils/zlib-1.3.tar.gz',
                        checksum='ff0ba4c292013dbc27530b3a81e1f9a813cd39de01ca5e0f8bf355702efa593e')

root = Path(__file__).parent

with tarfile.open(archive) as tf:
    tf.extractall(root)

build_dir = root / f'zlib-x86_64-windows' / 'build'

install_prefix = root / f'zlib-x86_64-windows' / 'install'

cmake_args = [
    '-G', 'Visual Studio 16 2019',
    '-A', 'x64'
]

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
