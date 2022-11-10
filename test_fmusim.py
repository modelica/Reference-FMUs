import subprocess
import os
from fmpy.util import read_csv
from pathlib import Path
import numpy as np
import plotly.graph_objects as go
from plotly.subplots import make_subplots


path_var = os.pathsep.join([os.environ.get('PATH', os.defpath), r'E:\Development\Reference-FMUs\mambaforge\Library\bin'])
env = dict(os.environ, PATH=path_var)
workdir = Path(r'E:\Development\Reference-FMUs\work')

executable = r'E:\Development\Reference-FMUs\sim\Debug\fmusim.exe'


def create_plot(result, filename):

    figure = make_subplots(rows=len(result.dtype.names) - 1, cols=1, shared_xaxes=True)

    for i, name in enumerate(result.dtype.names[1:]):
        t = result['time']
        y = result[name]

        trace2 = go.Scatter(x=t, y=y, line_color='blue', mode='lines+markers', name=name)

        figure.add_trace(trace2, row=i + 1, col=1)

        figure.write_html(filename)


def test_fmusim():

    for fmi_version in ['fmi2', 'fmi3']:

        models = [
            'BouncingBall',
            'Dahlquist',
            'Feedthrough',
            'Resource',
            'Stair',
            'VanDerPol',
        ]

        if fmi_version == 'fmi3':
            models.append('LinearTransform')

        for model in models:

            for interface_type in ['CS', 'ME']:

                os.makedirs(workdir / fmi_version / interface_type, exist_ok=True)

                subprocess.check_call([executable,
                                       '--interface-type', interface_type,
                                       r'--output-file', workdir / fmi_version / interface_type / f'{model}_out.csv',
                                       fr'E:\Development\Reference-FMUs\{fmi_version}\dist\{model}.fmu'],
                                      cwd=workdir,
                                      env=env)

                result = read_csv(workdir / fmi_version / interface_type / f'{model}_out.csv')

                create_plot(result, workdir / fmi_version / interface_type / f'{model}.html')
