import os
from itertools import product
from pathlib import Path
from subprocess import check_call
from typing import Iterable

import numpy as np
import pytest

from fmpy.util import read_csv


root = Path(__file__).parent.parent

resources = Path(__file__).parent / 'resources'

work = Path(__file__).parent / 'work'

os.makedirs(work, exist_ok=True)


def call_fmusim(platform: str, fmi_version: int, interface_type: str, test_name: str, args: Iterable[str], model: str = 'BouncingBall.fmu'):

    if fmi_version == 1:
        install = root / 'build' / f'fmi{fmi_version}-{interface_type}-{platform}' / 'install'
    else:
        install = root / 'build' / f'fmi{fmi_version}-{platform}' / 'install'

    output_file = work / f'{test_name}_fmi{fmi_version}_{interface_type}.csv'

    if output_file.exists():
        os.remove(output_file)

    fmusim_args = [install / 'fmusim', '--interface-type', interface_type, '--output-file', output_file] + args + [install / model]

    if platform == 'aarch64-linux':
        fmusim_args = ['qemu-aarch64', '-L', '/usr/aarch64-linux-gnu'] + fmusim_args

    check_call(fmusim_args, cwd=work)

    return read_csv(output_file)


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_start_time(fmi_version, interface_type, arch, platform):

    if fmi_version in {1, 2} and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    result = call_fmusim(platform, fmi_version, interface_type, 'test_start_time', ['--start-time', '0.5'])

    assert result['time'][0] == 0.5


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_stop_time(fmi_version, interface_type, arch, platform):

    if fmi_version in {1, 2} and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    result = call_fmusim(platform, fmi_version, interface_type, 'test_stop_time', ['--stop-time', '1.5'])

    assert result['time'][-1] == pytest.approx(1.5)


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_set_stop_time(fmi_version, interface_type, arch, platform):

    if fmi_version in {1, 2} and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    result = call_fmusim(platform, fmi_version, interface_type, 'test_set_stop_time',
                         ['--stop-time', '3', '--set-stop-time', '--output-interval', '0.7'])

    last = result['time'][-1]

    if fmi_version == 1 and interface_type == 'cs':
        assert last == pytest.approx(2.8)
    else:
        assert last == pytest.approx(3.0)


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_start_value_types(fmi_version, interface_type, arch, platform):

    if fmi_version in {1, 2} and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    args = [
        '--log-fmi-calls',
        '--start-value', 'Float64_continuous_input', '-5e-1',
        '--start-value', 'Int32_input', '2147483647',
        '--start-value', 'Boolean_input', '1',
        '--start-value', 'String_parameter', 'FMI is awesome!',
        '--start-value', 'Enumeration_input', '2',
    ]

    if fmi_version == 3:
        args += [
            '--start-value', 'Float32_continuous_input', '0.2',
            '--start-value', 'Int8_input', '127',
            '--start-value', 'UInt8_input', '255',
            '--start-value', 'Int16_input', '32767',
            '--start-value', 'UInt16_input', '65535',
            '--start-value', 'UInt32_input', '4294967295',
            '--start-value', 'Int64_input', '9223372036854775807',
            '--start-value', 'UInt64_input', '18446744073709551615',
            '--start-value', 'Binary_input', '42696E617279',
        ]

    result = call_fmusim(
        platform=platform,
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_start_value_types',
        args=args,
        model='Feedthrough.fmu'
    )

    assert result['Float64_continuous_output'][0] == -0.5
    assert result['Int32_output'][0] == 2147483647
    assert result['Boolean_output'][0] == 1
    assert result['Enumeration_output'][0] == 2

    if fmi_version == 3:
        assert result['Float32_continuous_output'][0] == 0.2
        assert result['Int8_output'][0] == 127
        assert result['UInt8_output'][0] == 255
        assert result['Int16_output'][0] == 32767
        assert result['UInt16_output'][0] == 65535
        assert result['UInt32_output'][0] == 4294967295
        assert result['Int64_output'][0] == 9223372036854775807
        assert result['UInt64_output'][0] == 18446744073709551615


@pytest.mark.parametrize('interface_type', ['cs', 'me'])
def test_start_value_arrays(work_dir, interface_type, platform):

    call_fmusim(
        platform=platform,
        fmi_version=3,
        interface_type=interface_type,
        test_name='test_start_value_arrays',
        args=[
            '--start-value', 'u', '1 2 3',
            '--start-value', 'C', '0 0 0 0 0 0 0 0 0',
            '--start-value', 'D', '1 0 0 0 1 0 0 0 1',
            '--log-fmi-calls',
            '--stop-time', '1',
            '--output-interval', '1',
        ],
        model='StateSpace.fmu'
    )

    with open(work_dir / 'test_start_value_arrays_fmi3_cs.csv') as f:
        file = f.read()

    assert file == '''"time","y"
0,1 2 3
1,1 2 3
'''


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_input_file(fmi_version, interface_type, arch, platform):

    if fmi_version in {1, 2} and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    result = call_fmusim(
        platform=platform,
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_input_file',
        args=['--input-file', resources / f'Feedthrough_in.csv', '--stop-time', '5'],
        model='Feedthrough.fmu'
    )

    t = result['time']

    # interpolated values
    x = result['Float64_continuous_output'][np.logical_and(t > 1.25, t < 1.75)]

    # start value
    assert result['Float64_continuous_output'][0] == 3

    # interpolation
    assert np.all(x > 2) and np.all(x < 3)

    # extrapolation
    assert result['Float64_continuous_output'][-1] == 3

    # start value
    assert result['Int32_output'][0] == 1

    # extrapolation
    assert result['Int32_output'][-1] == 2


def test_arrays(work_dir, platform):

    call_fmusim(
        platform=platform,
        fmi_version=3,
        interface_type='cs',
        test_name='test_array_input',
        args=[
            '--input-file', resources / 'StateSpace_in.csv',
            '--output-interval', '1',
            '--stop-time', '2',
            '--start-value', 'm', '2',
            '--start-value', 'n', '0',
            '--start-value', 'r', '2',
            '--start-value', 'D', '1 0 0 1',
            '--log-fmi-calls'
        ],
        model='StateSpace.fmu'
    )

    with open(work_dir / 'test_array_input_fmi3_cs.csv') as f:
        file = f.read()

    assert file == '''"time","y"
0,1 2
1,1 2
2,3 4
'''


def test_collapsed_array(work_dir, platform):

    call_fmusim(
        platform=platform,
        fmi_version=3,
        interface_type='cs',
        test_name='test_collapsed_array',
        args=[
            '--input-file', resources / 'StateSpace_collapsed_in.csv',
            '--output-interval', '1',
            '--stop-time', '2',
            '--start-value', 'm', '0',
            '--start-value', 'r', '0',
            '--log-fmi-calls',
        ],
        model='StateSpace.fmu'
    )

    with open(work_dir / 'test_collapsed_array_fmi3_cs.csv') as f:
        file = f.read()

    assert file == '''"time","y"
0,
1,
2,
'''


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_fmi_log_file(fmi_version, interface_type, arch, platform):

    if fmi_version in {1, 2} and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    fmi_log_file = work / f'test_fmi_log_file_fmi{fmi_version}_{interface_type}.txt'

    call_fmusim(
        platform=platform,
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_fmi_log_file',
        args=['--log-fmi-calls', '--fmi-log-file', fmi_log_file]
    )

    assert fmi_log_file.is_file()


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_output_interval(fmi_version, interface_type, arch, platform):

    if fmi_version in {1, 2} and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    result = call_fmusim(
        platform=platform,
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_output_interval',
        args=['--output-interval', '0.25']
    )

    if interface_type == 'cs':
        assert np.all(np.diff(result['time']) == 0.25)
    else:
        assert np.all(np.diff(result['time']) <= 0.25)


@pytest.mark.parametrize('fmi_version, solver', product([1, 2, 3], ['euler', 'cvode']))
def test_solver(fmi_version, solver, arch, platform):

    if fmi_version in {1, 2} and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    call_fmusim(
        platform=platform,
        fmi_version=fmi_version,
        interface_type='me',
        test_name='test_solver',
        args=['--solver', solver]
    )


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_output_variable(fmi_version, interface_type, arch, platform):

    if fmi_version in {1, 2} and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    result = call_fmusim(
        platform=platform,
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_output_variable',
        args=['--output-variable', 'e', '--output-variable', 'der(h)']
    )

    assert set(result.dtype.names) == {'time', 'e', 'der(h)'}


def test_intermediate_values(platform):

    result = call_fmusim(
        platform=platform,
        fmi_version=3,
        interface_type='cs',
        test_name='test_intermediate_values',
        args=['--record-intermediate-values']
    )

    assert np.all(np.diff(result['time']) < 0.1)


def test_early_return_state_events(platform):

    result = call_fmusim(
        platform=platform,
        fmi_version=3,
        interface_type='cs',
        test_name='test_early_return_state_events',
        args=['--early-return-allowed', '--output-interval', '0.5']
    )

    time = result['time']

    assert np.sum(np.logical_and(time > 0, time < 0.5)) == 1


def test_event_mode_input_events(platform):

    input_file = resources / 'Feedthrough_in.csv'

    result = call_fmusim(
        platform=platform,
        fmi_version=3,
        interface_type='cs',
        test_name='test_event_mode_input_events',
        args=[
            '--event-mode-used',
            '--stop-time', '5',
            '--output-interval', '2.5',
            '--input-file', input_file,
        ],
        model='Feedthrough.fmu'
    )

    assert np.all(result['time']                      == [0, 1, 1, 2, 2, 2.5, 3, 3, 5])
    assert np.all(result['Float64_continuous_output'] == [3, 3, 2, 3, 3, 3,   3, 3, 3])
    assert np.all(result['Float64_discrete_output']   == [3, 3, 2, 2, 3, 3,   3, 3, 3])
    assert np.all(result['Int32_output']              == [1, 1, 1, 1, 1, 1,   1, 2, 2])


def test_event_mode_time_events(platform):

    result = call_fmusim(
        platform=platform,
        fmi_version=3,
        interface_type='cs',
        test_name='test_event_mode_input_events',
        args=[
            '--stop-time', '5',
            '--output-interval', '2.5',
            '--event-mode-used',
            '--early-return-allowed'
        ],
        model='Stair.fmu'
    )

    assert np.all(result['time']    == [0, 1, 1, 2, 2, 2.5, 3, 3, 4, 4, 5, 5])
    assert np.all(result['counter'] == [1, 1, 2, 2, 3, 3,   3, 4, 4, 5, 5, 6])


@pytest.mark.parametrize('fmi_version, interface_type', product([2, 3], ['cs', 'me']))
def test_restore_fmu_state(fmi_version, interface_type, arch, platform):

    if fmi_version == 2 and arch not in {'x86', 'x86_64'}:
        pytest.skip(f"FMI version {fmi_version} is not supported on {arch}.")

    result1 = call_fmusim(
        platform=platform,
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_restore_fmu_state',
        args=[
            '--stop-time', '1',
            '--final-fmu-state-file',
            f'FMUState_{fmi_version}_{interface_type}.bin'
        ],
        model='BouncingBall.fmu'
    )

    assert result1['time'][-1] == 1

    result2 = call_fmusim(
        platform=platform,
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_fmu_state',
        args=[
            '--start-time', '1',
            '--stop-time', '2',
            '--initial-fmu-state-file',
            f'FMUState_{fmi_version}_{interface_type}.bin'
        ],
        model='BouncingBall.fmu'
    )

    assert result2['time'][0] == 1
    assert result2['h'][0] == result1['h'][-1]
