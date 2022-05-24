import zipfile
from tempfile import mkdtemp
from shutil import rmtree, copyfile, make_archive
import os

repo_dir = os.path.dirname(__file__)

test_fmus_version = '0.0.16'


def merge(fmi_version, fmi_types):

    for fmi_type in fmi_types:

        platforms_dir = os.path.join(repo_dir, 'fmus', fmi_version, fmi_type)

        _, platforms, _ = next(os.walk(platforms_dir))

        version_dir = os.path.join(repo_dir, 'fmus', fmi_version, fmi_type, platforms[0], 'Reference-FMUs', test_fmus_version)

        _, model_names, _ = next(os.walk(version_dir))

        for model_name in model_names:

            tempdir = mkdtemp()

            for platform in platforms:

                platform_fmu = os.path.join(repo_dir, 'fmus', fmi_version, fmi_type, platform, 'Reference-FMUs', test_fmus_version, model_name, model_name + '.fmu')

                if not os.path.isfile(platform_fmu):
                    continue

                with zipfile.ZipFile(platform_fmu, 'r') as archive:
                    archive.extractall(path=tempdir)

            if fmi_version == '1.0':
                merged_fmu = os.path.join(repo_dir, 'merged', fmi_version, fmi_type, model_name + '.fmu')
            else:
                merged_fmu = os.path.join(repo_dir, 'merged', fmi_version, model_name + '.fmu')

            os.makedirs(os.path.dirname(merged_fmu), exist_ok=True)

            make_archive(merged_fmu, 'zip', tempdir)

            os.rename(merged_fmu + '.zip', merged_fmu)

            rmtree(tempdir, ignore_errors=True)


# clean up
rmtree(os.path.join(repo_dir, 'merged'), ignore_errors=True)

# merge platform binaries
merge('1.0', ['cs', 'me'])
merge('2.0', ['cs'])
merge('3.0', ['cs', 'se'])

results_dir = os.path.join(repo_dir, 'merged', 'results')
os.makedirs(results_dir, exist_ok=True)

# copy reference results
for model in ['BouncingBall', 'Clocks', 'Dahlquist', 'Feedthrough', 'LinearTransform', 'Resource', 'Stair', 'VanDerPol']:
    copyfile(src=os.path.join(repo_dir, model, model + '_ref.csv'), dst=os.path.join(results_dir, model + '_ref.csv'))

# copy license and readme
for file in ['LICENSE.txt', 'README.md']:
    copyfile(src=os.path.join(repo_dir, file), dst=os.path.join(repo_dir, 'merged', file))

make_archive(os.path.join(repo_dir, 'merged_fmus'), 'zip', os.path.join(repo_dir, 'merged'))
