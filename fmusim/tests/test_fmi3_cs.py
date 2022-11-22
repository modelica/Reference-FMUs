from subprocess import check_call

import numpy as np
from fmpy.util import read_csv


def test_early_return_state_events(executable, dist, work):

    output_file = work / 'test_early_return_state_events.csv'

    fmu_filename = dist / '3.0' / 'BouncingBall.fmu'

    check_call([
        executable,
        '--interface-type', 'cs',
        '--early-return-allowed',
        '--output-interval', '0.5',
        '--output-file', output_file,
        fmu_filename]
    )

    result = read_csv(output_file)

    time = result['time']

    assert np.sum(np.logical_and(time > 0, time < 0.5)) == 1


def test_event_mode_input_events(executable, dist, resources, work):

    input_file = resources / 'Feedthrough_in.csv'
    output_file = work / 'test_event_mode_input_events.csv'

    fmu_filename = dist / '3.0' / 'Feedthrough.fmu'

    check_call([
        executable,
        '--interface-type', 'cs',
        '--event-mode-used',
        '--stop-time', '5',
        '--output-interval', '2.5',
        '--input-file', input_file,
        '--output-file', output_file,
        '--event-mode-used',
        fmu_filename]
    )

    result = read_csv(output_file)

    assert np.all(result['time'] == [0, 1, 1, 2.5, 3, 3, 5])
    assert np.all(result['Float64_continuous_output'] == [3, 3, 2, 3,   3, 3, 3])
    assert np.all(result['Int32_output'] == [1, 1, 1, 1,   1, 2, 2])


def test_event_mode_time_events(executable, dist, resources, work):

    output_file = work / 'test_event_mode_time_events.csv'

    fmu_filename = dist / '3.0' / 'Stair.fmu'

    check_call([
        executable,
        '--interface-type', 'cs',
        '--event-mode-used',
        '--stop-time', '5',
        '--output-interval', '2.5',
        '--output-file', output_file,
        '--event-mode-used',
        fmu_filename]
    )

    result = read_csv(output_file)

    assert np.all(result['time'] == [0, 1, 1, 2, 2, 2.5, 3, 3, 4, 4, 5, 5])
    assert np.all(result['counter'] == [1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6])
