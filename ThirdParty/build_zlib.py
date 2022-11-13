import os
from pathlib import Path
from subprocess import check_call

from fmpy.util import download_file
from fmpy import extract


archive = download_file('https://www.zlib.net/zlib1213.zip',
                        checksum='d233fca7cf68db4c16dc5287af61f3cd01ab62495224c66639ca3da537701e42')

root = Path(__file__).parent

extract(archive, root)

build_dir = root / 'zlib-1.2.13' / 'windows-x64'

install_prefix = build_dir / 'install'

check_call([
    'cmake',
    '-B', build_dir,
    '-G', 'Visual Studio 17 2022',
    '-A', 'x64',
    '-D', 'CMAKE_C_FLAGS_RELEASE=/MT /O2 /Ob2 /DNDEBUG',
    '-D', f'CMAKE_INSTALL_PREFIX={ install_prefix }',
    root / 'zlib-1.2.13'
])

check_call([
    'cmake',
    '--build', build_dir,
    '--config', 'Release',
    '--target', 'INSTALL'
])

os.remove(archive)
