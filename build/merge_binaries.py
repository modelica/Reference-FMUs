from datetime import datetime
from glob import glob

import pytz
import shutil
import subprocess
import zipfile
from pathlib import Path
from subprocess import check_call
from tempfile import mkdtemp
from shutil import rmtree, make_archive
import os
import fmpy
import jinja2
import markdown2
from fmpy import read_csv, plot_result, read_model_description

root = Path(__file__).parent.parent

dist_merged = root / 'dist-merged'

os.makedirs(dist_merged, exist_ok=True)

fmusim = root / f'dist-{fmpy.platform_tuple}' / f'fmusim-{fmpy.platform_tuple}' / 'fmusim'

parameters = {
    'BouncingBall': [
        '--output-interval', '0.05',
    ],
    'Dahlquist': [
        '--output-interval', '0.2',
    ],
    'Feedthrough': [
        '--output-interval', '1',
    ],
    'StateSpace': [
        '--output-interval', '1',
    ],
    'Resource': [
        '--output-interval', '1',
    ],
    'Stair':  [
        '--output-interval', '1',
    ],
    'VanDerPol':  [
        '--output-interval', '0.2',
    ]
}


def set_tool_version(filename, git_executable='git'):
    """ Set the Git tag or hash in the generationTool and generationDateAndTime attributes
        if the repo is clean """

    cwd = os.path.dirname(__file__)

    changed_files = subprocess.check_output([git_executable, 'status', '--porcelain', '--untracked=no'],
                                            cwd=cwd).decode('ascii').strip()

    if changed_files:
        return

    version = subprocess.check_output([git_executable, 'tag', '--contains'], cwd=cwd).decode('ascii').strip()

    if not version:
        version = subprocess.check_output([git_executable, 'rev-parse', '--short', 'HEAD'], cwd=cwd).decode(
            'ascii').strip()

    if not version:
        return

    with open(filename, 'r') as f:
        lines = f.read()

    isodate = datetime.now(pytz.utc).isoformat()

    lines = lines.replace('"Reference FMUs (development build)"',
                          f'"Reference FMUs ({version})"\n  generationDateAndTime="{isodate}"')

    with open(filename, 'w') as f:
        f.write(lines)


def merge_fmus(version):

    for dirpath, dirnames, filenames in os.walk(root / 'dist-x86_64-windows' / version):

        for filename in filenames:

            if not filename.endswith('.fmu'):
                continue

            model_name, _ = os.path.splitext(filename)

            tempdir = Path(mkdtemp())

            platforms = ['x86-windows', 'x86_64-windows', 'x86_64-linux', 'x86_64-darwin']

            if version == '3.0':
                platforms.append('aarch64-linux')

            for platform in platforms:

                platform_fmu = root / f'dist-{platform}' / version / filename

                with zipfile.ZipFile(platform_fmu, 'r') as archive:
                    archive.extractall(path=tempdir)

            if model_name in parameters:

                output_filename = root / f'dist-{fmpy.platform_tuple}' / version / f'{model_name}_ref.csv'
                os.makedirs(tempdir / 'documentation', exist_ok=True)
                plot_filename = tempdir / 'documentation' / 'result.svg'

                if version == '1.0/cs':
                    params = ['--interface-type', 'cs']
                else:
                    params = ['--interface-type', 'me', '--solver', 'cvode']

                params += parameters[model_name]

                command = [str(fmusim)] + params + ['--output-file', str(output_filename), str(root / f'dist-{fmpy.platform_tuple}' / version / filename)]

                print(' '.join(command))

                check_call(command)

                result = read_csv(output_filename)

                # create plot
                plot_result(result, markers=True, events=True, filename=plot_filename)

                def get_unit(variable):
                    if variable.unit is not None:
                        return variable.unit
                    elif variable.declaredType is not None:
                        return variable.declaredType.unit
                    else:
                        return ''

                # generate index.html
                model_description = read_model_description(root / f'dist-{fmpy.platform_tuple}' / version / filename)
                loader = jinja2.FileSystemLoader(searchpath=root)
                environment = jinja2.Environment(loader=loader, trim_blocks=True)
                template = environment.get_template('template.html')
                template.globals.update({'get_unit': get_unit})
                md_file = root / model_name / 'readme.md'
                html_file = tempdir / 'documentation' / 'index.html'
                content = markdown2.markdown_path(md_file, extras=['tables', 'fenced-code-blocks'])
                html = template.render(
                    model_name=model_name,
                    content=content,
                    model_description=model_description,
                    params=' '.join(params)
                )
                with open(html_file, 'w') as f:
                    f.write(html)
                for svg_file in glob(f'{str(root / model_name)}/*.svg'):
                    shutil.copy(svg_file, tempdir / 'documentation')

                # set tool version
                set_tool_version(tempdir / 'modelDescription.xml')

            # create archive
            merged_fmu = os.path.join(root, 'dist-merged', version, filename)
            make_archive(merged_fmu, 'zip', tempdir)
            os.rename(merged_fmu + '.zip', merged_fmu)

            # clean up
            rmtree(tempdir, ignore_errors=True)


for version in ['1.0/cs', '1.0/me', '2.0', '3.0']:
    os.makedirs(dist_merged / version)
    merge_fmus(version)

results_dir = root / 'dist-merged' / 'results'

os.makedirs(results_dir, exist_ok=True)

# copy fmusim
for system in ['x86-windows', 'x86_64-windows', 'x86_64-linux', 'aarch64-linux', 'x86_64-darwin']:
    shutil.copytree(src=root / f'dist-{system}' / f'fmusim-{system}', dst=root / 'dist-merged' / f'fmusim-{system}')

# copy license and readme
for file in ['LICENSE.txt', 'README.md']:
    shutil.copyfile(src=root / file, dst=root / 'dist-merged' / file)
