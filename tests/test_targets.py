import os
import platform
import subprocess
from pathlib import Path

import pytest

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


def validate(build_dir, model, fmi_types, simulate=True):

    fmu_filename = os.path.join(build_dir, 'install', model + '.fmu')

    problems = validate_fmu(fmu_filename)

    assert not problems

    if simulate:
        for fmi_type in fmi_types:
            simulate_fmu(fmu_filename, fmi_type=fmi_type)


def test_fmi1_me(arch):

    if arch not in {'x86', 'x86_64'}:
        pytest.skip(f"{arch} not supported")

    build_dir = root / 'fmi1_me'

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Stair', 'VanDerPol']

    for model in models:
        validate(build_dir, model=model, fmi_types=['ModelExchange'])

    for model in models:
        run_example(build_dir / 'temp' / f'{model}_me')


def test_fmi1_cs(arch):

    if arch not in {'x86', 'x86_64'}:
        pytest.skip(f"{arch} not supported")

    build_dir = root / 'fmi1_cs'

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Resource', 'Stair', 'VanDerPol']

    for model in models:
        validate(build_dir, model=model, fmi_types=['CoSimulation'])

    for model in models:
        run_example(build_dir / 'temp' / f'{model}_cs')


def test_fmi2(arch):

    if arch not in {'x86', 'x86_64'}:
        pytest.skip(f"{arch} not supported")

    build_dir = root / 'fmi2'

    models = ['BouncingBall', 'Dahlquist', 'Resource', 'Stair', 'VanDerPol', 'Feedthrough']

    for model in models:
        validate(build_dir, model=model, fmi_types=['ModelExchange', 'CoSimulation'])

    for model in models:
        for interface_type in ['cs', 'me']:
            run_example(build_dir / 'temp' / f'{model}_{interface_type}')


def test_fmi3(arch):

    build_dir = root / 'fmi3'

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Resource', 'Stair', 'StateSpace', 'VanDerPol']

    for model in models:
        validate(build_dir, model=model, fmi_types=['ModelExchange', 'CoSimulation'], simulate=arch in {'x86', 'x86_64'})

    for model in models:
        for interface_type in ['cs', 'me']:
            run_example(build_dir / 'temp' / f'{model}_{interface_type}')

    assert not validate_fmu(build_dir / 'install' / 'Clocks.fmu')


def test_cs_reconfiguration():

    build_dir = root / 'fmi3' / 'temp'

    subprocess.check_call(build_dir / 'cs_reconfiguration', cwd=build_dir)

    with open(build_dir / 'cs_reconfiguration_out.csv') as f:
        file = f.read()

    assert file == '''time,y
0,0 1
0.1,0.1 1.1
0.2,0.2 1.2
0.3,0.3 1.3
0.4,0.4 1.4
0.5,0.5 1.5
0.6,0.6 1.6 2.6
0.7,0.7 1.7 2.7
0.8,0.8 1.8 2.8
0.9,0.9 1.9 2.9
1,1 2 3
'''
