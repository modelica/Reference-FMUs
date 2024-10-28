from pathlib import Path
from subprocess import check_call
from fmpy import extract, platform_tuple
from fmpy.util import download_file


root = Path(__file__).parent

archive = download_file('https://github.com/GNOME/libxml2/archive/refs/tags/v2.11.5.zip',
                        checksum='711675470075cc85ba450f56aff7424f1ecdef00bc5d1d5dced3ffecd1a9b772')

extract(archive, root)

if platform_tuple == 'x86_64-windows':
    cmake_args = [
        '-G', 'Visual Studio 16 2019',
        '-A', 'x64'
    ]
else:
    cmake_args = []   

build_dir = root / f'libxml2-{platform_tuple}' / 'build'
install_prefix = root / f'libxml2-{platform_tuple}' / 'install'

check_call(
    ['cmake'] +
    cmake_args +
    ['-B', build_dir,
    '-D', f'CMAKE_INSTALL_PREFIX={install_prefix}',
    # '-D', 'BUILD_SHARED_LIBS=OFF',
    '-D', 'LIBXML2_WITH_ICONV=OFF',
    '-D', 'LIBXML2_WITH_LZMA=OFF',
    '-D', 'LIBXML2_WITH_PYTHON=OFF',
    '-D', 'LIBXML2_WITH_ZLIB=OFF',
    '-D', 'LIBXML2_WITH_TESTS=OFF',
    str(root / 'libxml2-2.11.5')]
)

check_call([
    'cmake',
    '--build', str(build_dir),
    '--config', 'Release',
    '--target', 'install'
])
