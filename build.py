# build FMUs and fmusim for all FMI versions

import os
import shutil
import subprocess
from pathlib import Path
import fmpy
import argparse


parent_dir = Path(__file__).parent

parser = argparse.ArgumentParser()

parser.add_argument('--cmake-generator', default='Visual Studio 17 2022' if os.name == 'nt' else 'Unix Makefiles')
parser.add_argument('--cmake-architecture', default='x64' if os.name == 'nt' else None)
parser.add_argument('--arch', default='x86_64')

args, _ = parser.parse_known_args()

toolchain_file = parent_dir / 'aarch64-toolchain.cmake'


def build_fmus(fmi_version, fmi_type=None):

    if fmi_type is not None:
        build_dir = parent_dir / f'fmi{fmi_version}_{fmi_type}'
    else:
        build_dir = parent_dir / f'fmi{fmi_version}'

    if build_dir.exists():
        shutil.rmtree(build_dir)

    os.makedirs(build_dir)

    cmake_args = ['-G', args.cmake_generator]

    if args.cmake_architecture is not None:
        cmake_args += ['-A', args.cmake_architecture]

    install_dir = build_dir / 'install'

    if fmi_type is not None:
        cmake_args += ['-D', f'FMI_TYPE={fmi_type.upper()}']

    if args.arch == 'aarch64':
        cmake_args += ['-D', f'CMAKE_TOOLCHAIN_FILE={ toolchain_file }']

    cmake_args += [
        '-D', f'FMI_VERSION={fmi_version}',
        '-D', f'CMAKE_INSTALL_PREFIX={install_dir}',
        '-D', f'WITH_FMUSIM=ON',
        '..'
    ]

    subprocess.check_call(['cmake'] + cmake_args, cwd=build_dir)
    subprocess.check_call(['cmake', '--build', '.', '--target', 'install', '--config', 'Release'], cwd=build_dir)

    fmus_dir = parent_dir / 'fmus' / f'{fmi_version}.0'

    if fmi_type is not None:
        fmus_dir = fmus_dir / fmi_type

    if fmus_dir.exists():
        shutil.rmtree(fmus_dir)

    os.makedirs(fmus_dir)

    fmusim_dir = parent_dir / 'fmus' / f'fmusim-{fmpy.system}'

    if fmusim_dir.exists():
        shutil.rmtree(fmusim_dir)

    os.makedirs(fmusim_dir)

    for root, dirs, files in os.walk(install_dir):
        for file in files:
            if file.endswith('.fmu'):
                shutil.copyfile(src=install_dir / file, dst=fmus_dir / file)
            elif file.startswith('fmusim'):
                shutil.copyfile(src=install_dir / file, dst=fmusim_dir / file)


if __name__ == '__main__':

    if args.arch == 'x86_64':
        build_fmus(fmi_version=1, fmi_type='me')
        build_fmus(fmi_version=1, fmi_type='cs')
        build_fmus(fmi_version=2)

    build_fmus(fmi_version=3)
