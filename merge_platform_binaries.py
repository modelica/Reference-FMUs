import shutil
import zipfile
from pathlib import Path
from tempfile import mkdtemp
from shutil import rmtree, make_archive
import os


root = Path(__file__).parent

os.makedirs(root / 'dist-merged', exist_ok=True)


def merge_fmus(version):

    for dirpath, dirnames, filenames in os.walk(root / 'dist-windows' / version):

        for filename in filenames:

            tempdir = mkdtemp()

            for platform in ['windows', 'linux', 'darwin']:

                platform_fmu = root / f'dist-{platform}' / version / filename

                with zipfile.ZipFile(platform_fmu, 'r') as archive:
                    archive.extractall(path=tempdir)

            merged_fmu = os.path.join(root, 'dist-merged', version, filename)

            make_archive(merged_fmu, 'zip', tempdir)

            os.rename(merged_fmu + '.zip', merged_fmu)

            rmtree(tempdir, ignore_errors=True)


for version in ['1.0/cs', '1.0/me', '2.0', '3.0']:
    merge_fmus(version)

results_dir = root / 'dist-merged' / 'results'

os.makedirs(results_dir, exist_ok=True)

# copy reference results
for model in ['BouncingBall', 'Clocks', 'Dahlquist', 'Feedthrough', 'LinearTransform', 'Resource', 'Stair', 'VanDerPol']:
    shutil.copyfile(src=root / model / f'{model}_ref.csv', dst=results_dir / f'{model}_ref.csv')

# copy fmusim
for system in ['windows', 'linux', 'darwin']:
    shutil.copytree(src=root / f'dist-{system}' / f'fmusim-{system}', dst=root / 'dist-merged' / f'fmusim-{system}')

# copy license and readme
for file in ['LICENSE.txt', 'README.md']:
    shutil.copyfile(src=root / file, dst=root / 'dist-merged' / file)
