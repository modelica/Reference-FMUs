import zipfile
from tempfile import mkdtemp
from shutil import rmtree
import shutil
import os

repo_dir = os.path.dirname(__file__)

test_fmus_version = '0.0.2'


def merge(fmi_version, fmi_types):

    for fmi_type in fmi_types:

        platforms_dir = os.path.join(repo_dir, 'fmus', fmi_version, fmi_type)

        _, platforms, _ = next(os.walk(platforms_dir))

        version_dir = os.path.join(repo_dir, 'fmus', fmi_version, fmi_type, platforms[0], 'Test-FMUs', test_fmus_version)

        _, model_names, _ = next(os.walk(version_dir))

        for model_name in model_names:

            tempdir = mkdtemp()

            for platform in platforms:

                platform_fmu = os.path.join(repo_dir, 'fmus', fmi_version, fmi_type, platform, 'Test-FMUs', test_fmus_version, model_name, model_name + '.fmu')

                if not os.path.isfile(platform_fmu):
                    continue

                with zipfile.ZipFile(platform_fmu, 'r') as archive:
                    archive.extractall(path=tempdir)

            if len(fmi_types) > 1:
                merged_fmu = os.path.join(repo_dir, 'merged', fmi_version, fmi_type, model_name + '.fmu')
            else:
                merged_fmu = os.path.join(repo_dir, 'merged', fmi_version, model_name + '.fmu')

            os.makedirs(os.path.dirname(merged_fmu), exist_ok=True)

            shutil.make_archive(merged_fmu, 'zip', tempdir)

            os.rename(merged_fmu + '.zip', merged_fmu)

            rmtree(tempdir, ignore_errors=True)


merge('1.0', ['cs', 'me'])
merge('2.0', ['cs'])
merge('3.0', ['cs'])

shutil.make_archive(os.path.join(repo_dir, 'merged_fmus'), 'zip', os.path.join(repo_dir, 'merged'))
