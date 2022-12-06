import os
import shutil
import subprocess
from pathlib import Path

import fmpy
from fmpy import simulate_fmu
from fmpy.util import compile_platform_binary
from fmpy.validation import validate_fmu


def validate(build_dir, fmi_types, models, compile=False):

    for model in models:

        fmu_filename = os.path.join(build_dir, 'install', model + '.fmu')

        problems = validate_fmu(fmu_filename)

        assert not problems

        for fmi_type in fmi_types:

            if compile:
                if model == 'Resource' and os.name == 'nt':
                    continue
                compile_platform_binary(fmu_filename)

            simulate_fmu(fmu_filename, fmi_type=fmi_type)


def build_fmus(build_dir, fmi_version, fmi_type='ME'):

    import argparse

    if build_dir.exists():
        shutil.rmtree(build_dir)

    os.makedirs(build_dir)

    parser = argparse.ArgumentParser()

    parser.add_argument('--cmake-generator', default='Visual Studio 17 2022' if os.name == 'nt' else 'Unix Makefiles')
    parser.add_argument('--cmake-architecture', default='x64' if os.name == 'nt' else None,)

    args, _ = parser.parse_known_args()

    cmake_options = ['-G', args.cmake_generator]

    if args.cmake_architecture is not None:
        cmake_options += ['-A', args.cmake_architecture]

    install_dir = build_dir / 'install'

    cmake_options += [
        '-D', f'FMI_VERSION={fmi_version}',
        '-D', f'FMI_TYPE={fmi_type}',
        '-D', f'CMAKE_INSTALL_PREFIX={install_dir}',
        '-D', f'WITH_FMUSIM=ON',
        '..'
    ]

    subprocess.call(['cmake'] + cmake_options, cwd=build_dir)
    subprocess.call(['cmake', '--build', '.', '--target', 'install', '--config', 'Release'], cwd=build_dir)


def test_fmi1_me():

    parent_dir = Path(__file__).parent
    build_dir = parent_dir / 'fmi1_me'
    fmus_dir = parent_dir / 'fmus' / '1.0' / 'me'

    build_fmus(build_dir, fmi_version=1, fmi_type='ME')

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Stair', 'VanDerPol']

    validate(build_dir, fmi_types=['ModelExchange'], models=models)

    os.makedirs(fmus_dir, exist_ok=True)

    for model in models:
        example = f'{model}_me'
        print(f"Running {example}...")
        filename = os.path.join(build_dir, 'temp', example)
        subprocess.check_call(filename, cwd=os.path.join(build_dir, 'temp'))
        shutil.copyfile(src=build_dir / 'install' / f'{model}.fmu', dst=fmus_dir / f'{model}.fmu')


def test_fmi1_cs():

    parent_dir = Path(__file__).parent
    build_dir = parent_dir / 'fmi1_cs'
    fmus_dir = parent_dir / 'fmus' / '1.0' / 'cs'

    build_fmus(build_dir, fmi_version=1, fmi_type='CS')

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Resource', 'Stair', 'VanDerPol']

    validate(build_dir, fmi_types=['CoSimulation'], models=models)

    os.makedirs(fmus_dir, exist_ok=True)

    for model in models:
        example = f'{model}_cs'
        print(f"Running {example}...")
        subprocess.check_call(build_dir / 'temp' / example, cwd=build_dir / 'temp')
        shutil.copyfile(src=build_dir / 'install' / f'{model}.fmu', dst=fmus_dir / f'{model}.fmu')


def test_fmi2():

    parent_dir = Path(__file__).parent
    build_dir = parent_dir / 'fmi2'
    fmus_dir = parent_dir / 'fmus' / '2.0'

    build_fmus(build_dir, fmi_version=2)

    models = ['BouncingBall', 'Dahlquist', 'Resource', 'Stair', 'VanDerPol', 'Feedthrough']

    validate(build_dir, fmi_types=['CoSimulation', 'ModelExchange'], models=models)
    validate(build_dir, fmi_types=['CoSimulation', 'ModelExchange'], models=models, compile=True)

    os.makedirs(fmus_dir, exist_ok=True)

    for model in models:
        for interface_type in ['cs', 'me']:
            example = f'{model}_{interface_type}'
            print(f"Running {example}...")
            filename = os.path.join(build_dir, 'temp', example)
            subprocess.check_call(filename, cwd=os.path.join(build_dir, 'temp'))
            shutil.copyfile(src=build_dir / 'install' / f'{model}.fmu', dst=fmus_dir / f'{model}.fmu')


def test_fmi3():

    parent_dir = Path(__file__).parent
    build_dir = parent_dir / 'fmi3'
    fmus_dir = parent_dir / 'fmus' / '3.0'

    build_fmus(build_dir, fmi_version=3)

    # run examples
    examples = [
        'cs_early_return',
        'cs_event_mode',
        'cs_intermediate_update',
        'BouncingBall_cs',
        'BouncingBall_me',
        'import_shared_library',
        'import_static_library',
        'jacobian',
        'scs_synchronous',
        'Stair_cs',
        'Stair_me'
    ]

    if os.name == 'nt':
        examples.append('scs_threaded')  # runs only on Windows

    for example in examples:
        print("Running %s example..." % example)
        subprocess.check_call(build_dir / 'temp' / example, cwd=build_dir / 'temp')

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Resource', 'Stair', 'VanDerPol']

    validate(build_dir, fmi_types=['CoSimulation', 'ModelExchange'], models=models)
    validate(build_dir, fmi_types=['CoSimulation', 'ModelExchange'], models=models, compile=True)

    for model in ['Clocks', 'LinearTransform']:
        problems = validate_fmu(filename=build_dir / 'install' / f'{model}.fmu')
        assert len(problems) == 0

    os.makedirs(fmus_dir, exist_ok=True)

    for model in models + ['Clocks', 'LinearTransform']:
        shutil.copyfile(src=build_dir / 'install' / f'{model}.fmu', dst=fmus_dir / f'{model}.fmu')

    if fmpy.system == 'darwin':
        os.makedirs(parent_dir / 'fmus' / 'fmusim-darwin', exist_ok=True)
        shutil.copyfile(src=build_dir / 'install' / 'fmusim', dst=parent_dir / 'fmus' / 'fmusim-darwin' / 'fmusim')
    elif fmpy.system == 'linux':
        os.makedirs(parent_dir / 'fmus' / 'fmusim-linux', exist_ok=True)
        shutil.copyfile(src=build_dir / 'install' / 'fmusim', dst=parent_dir / 'fmus' / 'fmusim-linux' / 'fmusim')
    elif fmpy.system == 'windows':
        os.makedirs(parent_dir / 'fmus' / 'fmusim-windows', exist_ok=True)
        shutil.copyfile(src=build_dir / 'install' / 'fmusim.exe', dst=parent_dir / 'fmus' / 'fmusim-windows' / 'fmusim.exe')
