import tarfile
from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file


archive = download_file('https://www.zlib.net/fossils/zlib-1.3.1.tar.gz',
                        checksum='9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23')

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
    str(root / 'zlib-1.3.1')]
)

check_call([
    'cmake',
    '--build', str(build_dir),
    '--config', 'Release',
    '--target', 'install'
])
