import os
import subprocess
from pathlib import Path
from fmpy import simulate_fmu
from fmpy.validation import validate_fmu


root = Path(__file__).parent.parent


def validate(build_dir, model, fmi_types):

    fmu_filename = os.path.join(build_dir, 'install', model + '.fmu')

    problems = validate_fmu(fmu_filename)

    assert not problems

    for fmi_type in fmi_types:
        simulate_fmu(fmu_filename, fmi_type=fmi_type)


def test_fmi1_me():

    build_dir = root / 'fmi1_me'

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Stair', 'VanDerPol']

    for model in models:
        validate(build_dir, model=model, fmi_types=['ModelExchange'])

    for model in models:
        subprocess.check_call(build_dir / 'temp' / f'{model}_me', cwd=os.path.join(build_dir, 'temp'))


def test_fmi1_cs():

    build_dir = root / 'fmi1_cs'

    models = ['BouncingBall', 'Dahlquist', 'Feedthrough', 'Resource', 'Stair', 'VanDerPol']

    for model in models:
        validate(build_dir, model=model, fmi_types=['CoSimulation'])

    for model in models:
        subprocess.check_call(build_dir / 'temp' / f'{model}_cs', cwd=os.path.join(build_dir, 'temp'))


def test_fmi2():

    build_dir = root / 'fmi2'

    models = ['BouncingBall', 'Dahlquist', 'Resource', 'Stair', 'VanDerPol', 'Feedthrough']

    for model in models:
        validate(build_dir, model=model, fmi_types=['ModelExchange', 'CoSimulation'])

    for model in models:
        for interface_type in ['cs', 'me']:
            subprocess.check_call(build_dir / 'temp' / f'{model}_{interface_type}', cwd=os.path.join(build_dir, 'temp'))


def test_fmi3():

    build_dir = root / 'fmi3'

    models = ['BouncingBall', 'Dahlquist', 'LinearTransform', 'Resource', 'Stair', 'VanDerPol', 'Feedthrough']

    # for model in models:
    #     validate(build_dir, model=model, fmi_types=['ModelExchange', 'CoSimulation'])

    for model in models:
        for interface_type in ['cs', 'me']:
            subprocess.check_call(build_dir / 'temp' / f'{model}_{interface_type}', cwd=os.path.join(build_dir, 'temp'))

    assert not validate_fmu(build_dir / 'install' / 'Clocks.fmu')
