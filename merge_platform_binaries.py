import shutil
import zipfile
from pathlib import Path
from tempfile import mkdtemp
from shutil import rmtree, make_archive
import os


root = Path(__file__).parent

os.makedirs(root / 'fmus-merged', exist_ok=True)


def merge_fmus(version):

    for dirpath, dirnames, filenames in os.walk(root / 'fmus-linux' / version):

        for filename in filenames:

            tempdir = mkdtemp()

            for platform in ['linux', 'windows']:

                platform_fmu = root / f'fmus-{platform}' / version / filename

                with zipfile.ZipFile(platform_fmu, 'r') as archive:
                    archive.extractall(path=tempdir)

            merged_fmu = os.path.join(root, 'fmus-merged', version, filename)

            make_archive(merged_fmu, 'zip', tempdir)

            os.rename(merged_fmu + '.zip', merged_fmu)

            rmtree(tempdir, ignore_errors=True)


for version in ['1.0/cs', '1.0/me', '2.0', '3.0']:
    merge_fmus(version)

results_dir = root / 'fmus-merged' / 'results'

os.makedirs(results_dir, exist_ok=True)

# copy reference results
for model in ['BouncingBall', 'Clocks', 'Dahlquist', 'Feedthrough', 'LinearTransform', 'Resource', 'Stair', 'VanDerPol']:
    shutil.copyfile(src=root / model / f'{model}_ref.csv', dst=results_dir / f'{model}_ref.csv')

# copy license and readme
for file in ['LICENSE.txt', 'README.md']:
    shutil.copyfile(src=root / file, dst=root / 'fmus-merged' / file)

# make_archive(base_name=root / 'fmus-merged', format='zip', root_dir=root / 'merged')

# clean up
rmtree(os.path.join(root, 'merged'), ignore_errors=True)
