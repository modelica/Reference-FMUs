import os
from itertools import product
from pathlib import Path
from subprocess import check_call

import numpy as np
import pytest
from fmpy.util import read_csv


root = Path(__file__).parent.parent

resources = Path(__file__).parent / 'resources'

work = Path(__file__).parent / 'work'

os.makedirs(work, exist_ok=True)


def call_fmusim(fmi_version, interface_type, test_name, args, model='BouncingBall.fmu'):

    if fmi_version == 1:
        install = root / f'fmi{fmi_version}_{interface_type}' / 'install'
    else:
        install = root / f'fmi{fmi_version}' / 'install'

    output_file = work / f'{test_name}_fmi{fmi_version}_{interface_type}.csv'

    if output_file.exists():
        os.remove(output_file)

    check_call([
        install / 'fmusim',
        '--interface-type', interface_type,
        '--output-file', output_file] +
        args +
        [install / model],
        cwd=work
    )

    return read_csv(output_file)


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_start_time(fmi_version, interface_type):
    result = call_fmusim(fmi_version, interface_type, 'test_start_time', ['--start-time', '0.5'])
    assert result['time'][0] == 0.5


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_stop_time(fmi_version, interface_type):
    result = call_fmusim(fmi_version, interface_type, 'test_stop_time', ['--stop-time', '1.5'])
    assert result['time'][-1] == pytest.approx(1.5)


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_start_value_types(fmi_version, interface_type):

    result = call_fmusim(
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_start_value_types',
        args=[
            '--start-value', 'Float64_continuous_input', '-5e-1',
            '--start-value', 'Int32_input', '2',
            '--start-value', 'Boolean_input', '1',
            '--start-value', 'String_parameter', 'FMI is awesome!'
        ],
        model='Feedthrough.fmu')

    assert result['Float64_continuous_output'][0] == -0.5
    assert result['Int32_output'][0] == 2
    assert result['Boolean_output'][0] == 1


# @pytest.mark.parametrize('interface_type', ['cs', 'me'])
# def test_start_value_arrays(interface_type):
#
#     result = call_fmusim(
#         fmi_version=3,
#         interface_type=interface_type,
#         test_name='test_start_value_arrays',
#         args=['--start-value', 'u', '2 3', '--log-fmi-calls'],
#         model='LinearTransform.fmu'
#     )
#
#     assert result['y[0]'][0] == 2
#     assert result['y[1]'][0] == 3


# @pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
# def test_input_file(fmi_version, interface_type):
#
#     result = call_fmusim(
#         fmi_version=fmi_version,
#         interface_type=interface_type,
#         test_name='test_input_file',
#         args=['--input-file', resources / f'Feedthrough_in.csv'],
#         model='Feedthrough.fmu')
#
#     t = result['time']
#
#     # interpolated values
#     x = result['Float64_continuous_output'][np.logical_and(t > 1.25, t < 1.75)]
#
#     # start value
#     assert result['Float64_continuous_output'][0] == 3
#
#     # interpolation
#     assert np.all(x > 2) and np.all(x < 3)
#
#     # extrapolation
#     assert result['Float64_continuous_output'][-1] == 3
#
#     # start value
#     assert result['Int32_output'][0] == 1
#
#     # extrapolation
#     assert result['Int32_output'][-1] == 2


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_fmi_log_file(fmi_version, interface_type):

    fmi_log_file = work / f'test_fmi_log_file_fmi{fmi_version}_{interface_type}.txt'

    call_fmusim(
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_fmi_log_file',
        args=['--log-fmi-calls', '--fmi-log-file', fmi_log_file]
    )

    assert fmi_log_file.is_file()


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_output_interval(fmi_version, interface_type):

    result = call_fmusim(
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
def test_solver(fmi_version, solver):

    call_fmusim(
        fmi_version=fmi_version,
        interface_type='me',
        test_name='test_solver',
        args=['--solver', solver]
    )


@pytest.mark.parametrize('fmi_version, interface_type', product([1, 2, 3], ['cs', 'me']))
def test_output_variable(fmi_version, interface_type):

    result = call_fmusim(
        fmi_version=fmi_version,
        interface_type=interface_type,
        test_name='test_output_variable',
        args=['--output-variable', 'e', '--output-variable', 'der(h)']
    )

    assert set(result.dtype.names) == {'time', 'e', 'der(h)'}


def test_intermediate_values():

    result = call_fmusim(
        fmi_version=3,
        interface_type='cs',
        test_name='test_intermediate_values',
        args=['--record-intermediate-values']
    )

    assert np.all(np.diff(result['time']) < 0.1)


def test_early_return_state_events():

    result = call_fmusim(
        fmi_version=3,
        interface_type='cs',
        test_name='test_early_return_state_events',
        args=['--early-return-allowed', '--output-interval', '0.5']
    )

    time = result['time']

    assert np.sum(np.logical_and(time > 0, time < 0.5)) == 1


def test_event_mode_input_events():

    input_file = resources / 'Feedthrough_in.csv'

    result = call_fmusim(
        fmi_version=3,
        interface_type='cs',
        test_name='test_event_mode_input_events',
        args=[
            '--event-mode-used',
            '--stop-time', '5',
            '--output-interval', '2.5',
            '--input-file', input_file,
            '--event-mode-used'
        ],
        model='Feedthrough.fmu'
    )

    assert np.all(result['time'] == [0, 1, 1, 2.5, 3, 3, 5])
    assert np.all(result['Float64_continuous_output'] == [3, 3, 2, 3,  3, 3, 3])
    assert np.all(result['Int32_output'] == [1, 1, 1, 1,   1, 2, 2])


def test_event_mode_time_events():

    result = call_fmusim(
        fmi_version=3,
        interface_type='cs',
        test_name='test_event_mode_input_events',
        args=[
            '--event-mode-used',
            '--stop-time', '5',
            '--output-interval', '2.5',
            '--event-mode-used'
        ],
        model='Stair.fmu'
    )

    assert np.all(result['time'] == [0, 1, 1, 2, 2, 2.5, 3, 3, 4, 4, 5, 5])
    assert np.all(result['counter'] == [1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6])
