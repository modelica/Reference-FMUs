import os
import platform
import subprocess
from pathlib import Path
from fmpy import simulate_fmu
from fmpy.validation import validate_fmu


root = Path(__file__).parent.parent


def run_example(name):
    example_args = [name]

    if platform.system() == 'Linux':

        result = subprocess.check_output(['file', name])

        if b'aarch64' in result:
            example_args = ['qemu-aarch64', '-L', '/usr/aarch64-linux-gnu'] + example_args

    subprocess.check_call(example_args, cwd=name.parent)


def validate(build_dir, model, fmi_types, simulate):

    fmu_filename = os.path.join(build_dir, 'install', model + '.fmu')

    problems = validate_fmu(fmu_filename)

    assert not problems

    if simulate:
        for fmi_type in fmi_types:
            simulate_fmu(fmu_filename, fmi_type=fmi_type)


def test_fmi1_me():

    build_dir = root / 'fmi1_me'

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Stair', 'VanDerPol']

    for model in models:
        validate(build_dir, model=model, fmi_types=['ModelExchange'], simulate=False)

    for model in models:
        run_example(build_dir / 'temp' / f'{model}_me')


def test_fmi1_cs():

    build_dir = root / 'fmi1_cs'

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Resource', 'Stair', 'VanDerPol']

    for model in models:
        validate(build_dir, model=model, fmi_types=['CoSimulation'], simulate=False)

    for model in models:
        run_example(build_dir / 'temp' / f'{model}_cs')


def test_fmi2():

    build_dir = root / 'fmi2'

    models = ['BouncingBall', 'Dahlquist', 'Resource', 'Stair', 'VanDerPol', 'Feedthrough']

    for model in models:
        validate(build_dir, model=model, fmi_types=['ModelExchange', 'CoSimulation'], simulate=False)

    for model in models:
        for interface_type in ['cs', 'me']:
            run_example(build_dir / 'temp' / f'{model}_{interface_type}')


def test_fmi3():

    build_dir = root / 'fmi3'

    models = ['BouncingBall', 'Dahlquist', 'LinearTransform', 'Resource', 'Stair', 'VanDerPol', 'Feedthrough']

    for model in models:
        validate(build_dir, model=model, fmi_types=['ModelExchange', 'CoSimulation'], simulate=False)

    for model in models:
        for interface_type in ['cs', 'me']:
            run_example(build_dir / 'temp' / f'{model}_{interface_type}')

    assert not validate_fmu(build_dir / 'install' / 'Clocks.fmu')
