import itertools

from pathlib import Path
import subprocess
import os
import shutil
from fmpy import simulate_fmu, platform
from fmpy.util import compile_platform_binary
from fmpy.validation import validate_fmu


test_fmus_version = '0.0.16'


def validate(build_dir, fmi_types, models, compile=False):

    from fmpy.util import read_csv, validate_result

    test_fmus_dir = Path(__file__).parent

    for model in models:

        print(model)

        fmu_filename = os.path.join(build_dir, 'dist', model + '.fmu')

        problems = validate_fmu(fmu_filename)

        assert not problems

        if model == 'Feedthrough':
            start_values = {'Float64_fixed_parameter': 1, 'String_parameter': "FMI is awesome!"}
            in_csv = os.path.join(test_fmus_dir, model, model + '_in.csv')
            input = read_csv(in_csv)
        else:
            start_values = {}
            input = None

        ref_csv = os.path.join(test_fmus_dir, model, model + '_ref.csv')

        for fmi_type in fmi_types:

            ref = read_csv(ref_csv)

            if compile:
                if model == 'Resource' and os.name == 'nt':
                    continue
                compile_platform_binary(fmu_filename)

            result = simulate_fmu(fmu_filename,
                                  fmi_type=fmi_type,
                                  start_values=start_values,
                                  input=input,
                                  solver='Euler')

            dev = validate_result(result, ref)

            assert dev < 0.2, "Failed to validate " + model


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

    cmake_options += ['-D', f'FMI_VERSION={fmi_version}', '-D', f'FMI_TYPE={fmi_type}', '..']

    subprocess.call(['cmake'] + cmake_options, cwd=build_dir)
    subprocess.call(['cmake', '--build', '.', '--config', 'Release'], cwd=build_dir)


def copy_to_cross_check(dist_dir, model_names, fmi_version, fmi_types):

    parent_dir = Path(__file__).parent

    for fmi_type, model in itertools.product(fmi_types, model_names):
        target_dir = parent_dir / 'fmus' / fmi_version / fmi_type / platform / 'Reference-FMUs' / test_fmus_version / model
        os.makedirs(target_dir, exist_ok=True)
        shutil.copy(dist_dir / f'{model}.fmu', target_dir)
        shutil.copy(parent_dir / model / f'{model}_ref.csv', target_dir)
        shutil.copy(parent_dir / model / f'{model}_ref.opt', target_dir)


def test_fmi1_me():

    build_dir = Path(__file__).parent / 'fmi1_me'

    build_fmus(build_dir, fmi_version=1, fmi_type='ME')

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Stair', 'VanDerPol']

    validate(build_dir, fmi_types=['ModelExchange'], models=models)

    for model in models:
        example = f'{model}_me'
    print(f"Running {example}...")
    filename = os.path.join(build_dir, 'temp', example)
    subprocess.check_call(filename, cwd=os.path.join(build_dir, 'temp'))

    copy_to_cross_check(dist_dir=build_dir / 'dist', model_names=models, fmi_version='1.0', fmi_types=['me'])


def test_fmi1_cs():

    build_dir = Path(__file__).parent / 'fmi1_cs'

    build_fmus(build_dir, fmi_version=1, fmi_type='CS')

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Resource', 'Stair', 'VanDerPol']

    validate(build_dir, fmi_types=['CoSimulation'], models=models)

    for model in models:
        example = f'{model}_cs'
        print(f"Running {example}...")
        filename = os.path.join(build_dir, 'temp', example)
        subprocess.check_call(filename, cwd=os.path.join(build_dir, 'temp'))

    copy_to_cross_check(dist_dir=build_dir / 'dist', model_names=models, fmi_version='1.0', fmi_types=['cs'])


def test_fmi2():

    build_dir = Path(__file__).parent / 'fmi2'

    build_fmus(build_dir, fmi_version=2)

    models = ['BouncingBall', 'Dahlquist', 'Resource', 'Stair', 'VanDerPol', 'Feedthrough']

    validate(build_dir, fmi_types=['CoSimulation', 'ModelExchange'], models=models)
    validate(build_dir, fmi_types=['CoSimulation', 'ModelExchange'], models=models, compile=True)

    for model in models:
        for interface_type in ['cs', 'me']:
            example = f'{model}_{interface_type}'
            print(f"Running {example}...")
            filename = os.path.join(build_dir, 'temp', example)
            subprocess.check_call(filename, cwd=os.path.join(build_dir, 'temp'))

    copy_to_cross_check(dist_dir=build_dir / 'dist', model_names=models, fmi_version='2.0', fmi_types=['cs', 'me'])


def test_fmi3():

    build_dir = Path(__file__).parent / 'fmi3'

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
        problems = validate_fmu(filename=build_dir / 'dist' / f'{model}.fmu')
        assert len(problems) == 0

    copy_to_cross_check(dist_dir=build_dir / 'dist', model_names=models, fmi_version='3.0', fmi_types=['cs', 'me'])
    copy_to_cross_check(dist_dir=build_dir / 'dist', model_names=['Clocks'], fmi_version='3.0', fmi_types=['se'])
