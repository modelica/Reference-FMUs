import os
import shutil
from pathlib import Path
import pytest
from fmpy import extract
from fmpy.util import download_file


@pytest.fixture(scope='session')
def executable():
    path = Path(__file__).parent.parent

    if os.name == 'nt':
        path = path / 'win64' / 'install' / 'fmusim.exe'

    yield path


@pytest.fixture(scope='session')
def work():
    path = Path(__file__).parent / 'work'
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path)
    yield path


@pytest.fixture(scope='session')
def dist():

    path = Path(__file__).parent / 'resources' / 'Reference-FMUs-0.0.18'

    if not path.exists():
        os.makedirs(path)
        archive = download_file('https://github.com/modelica/Reference-FMUs/releases/download/v0.0.18/Reference-FMUs-0.0.18.zip')
        extract(archive, unzipdir=path)
        os.remove(archive)

    yield path


@pytest.fixture(scope='session')
def resources():
    yield Path(__file__).parent / 'resources'
