from pathlib import Path
from subprocess import check_call
from fmpy.util import download_file
from fmpy import extract


archive = download_file('https://github.com/GNOME/libxml2/archive/refs/tags/v2.13.4.zip',
                        checksum='9d31649a3f8c0274253e57874280647c8962eed36570b714e869939c101347ba')

root = Path(__file__).parent

extract(archive, root)

build_dir = root / 'libxml2-x86_64-windows' / 'build'

install_prefix = root / 'libxml2-x86_64-windows' / 'install'

cmake_args = [
    '-G', 'Visual Studio 16 2019',
    '-A', 'x64'
]

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
    str(root / 'libxml2-2.13.4')]
)

check_call([
    'cmake',
    '--build', str(build_dir),
    '--config', 'Release',
    '--target', 'install'
])
