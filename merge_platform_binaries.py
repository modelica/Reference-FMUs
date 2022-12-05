import shutil
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

root = Path(__file__).parent

dist_merged = root / 'dist-merged'

os.makedirs(dist_merged, exist_ok=True)

fmusim = root / f'dist-{fmpy.system}' / f'fmusim-{fmpy.system}' / 'fmusim'

parameters = {
    'BouncingBall': [
        '--output-interval', '0.05',
        '--stop-time', '3'
    ],
    'Dahlquist': [
        '--output-interval', '0.2',
        '--stop-time', '10'
    ],
    'Feedthrough': [
        '--output-interval', '1',
        '--stop-time', '2'
    ],
    'LinearTransform': [
        '--output-interval', '1',
        '--stop-time', '2'
    ],
    'Resource': [
        '--output-interval', '1',
        '--stop-time', '2'
    ],
    'Stair':  [
        '--output-interval', '1',
        '--stop-time', '10'
    ],
    'VanDerPol':  [
        '--output-interval', '0.2',
        '--stop-time', '20'
    ]
}


def merge_fmus(version):

    for dirpath, dirnames, filenames in os.walk(root / 'dist-windows' / version):

        for filename in filenames:

            model_name, _ = os.path.splitext(filename)

            tempdir = Path(mkdtemp())

            for platform in ['windows', 'linux', 'darwin']:

                platform_fmu = root / f'dist-{platform}' / version / filename

                with zipfile.ZipFile(platform_fmu, 'r') as archive:
                    archive.extractall(path=tempdir)

            if model_name in parameters:

                output_filename = root / 'dist-merged' / version / f'{model_name}_ref.csv'
                os.makedirs(tempdir / 'documentation', exist_ok=True)
                plot_filename = tempdir / 'documentation' / 'result.svg'

                interface_type = 'cs' if version == '1.0/cs' else 'me'

                command = [
                    str(fmusim),
                    '--interface-type', interface_type,
                    '--solver', 'cvode',
                    '--output-file', str(output_filename),
                ] + parameters[model_name] + [
                    str(root / f'dist-{fmpy.system}' / version / filename)
                ]

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
                model_description = read_model_description(root / f'dist-{fmpy.system}' / version / filename)
                loader = jinja2.FileSystemLoader(searchpath=root)
                environment = jinja2.Environment(loader=loader, trim_blocks=True)
                template = environment.get_template('template.html')
                template.globals.update({'get_unit': get_unit})
                md_file = root / model_name / 'readme.md'
                html_file = tempdir / 'documentation' / 'index.html'
                content = markdown2.markdown_path(md_file, extras=['tables', 'fenced-code-blocks'])
                html = template.render(model_name=model_name, content=content, model_description=model_description)
                with open(html_file, 'w') as f:
                    f.write(html)

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
for system in ['windows', 'linux', 'darwin']:
    shutil.copytree(src=root / f'dist-{system}' / f'fmusim-{system}', dst=root / 'dist-merged' / f'fmusim-{system}')

# copy license and readme
for file in ['LICENSE.txt', 'README.md']:
    shutil.copyfile(src=root / file, dst=root / 'dist-merged' / file)
