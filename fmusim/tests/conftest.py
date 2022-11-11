import os
import shutil
from pathlib import Path
import pytest


@pytest.fixture(scope='session')
def executable():
    yield Path(r'E:\Development\Reference-FMUs\sim\Debug\fmusim.exe')


@pytest.fixture(scope='session')
def work():
    path = Path(r'E:\Development\Reference-FMUs\fmusim\tests\work')
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path)
    yield path


@pytest.fixture(scope='session')
def dist():
    yield Path(r'C:\Users\tsr2\Downloads\Reference-FMUs-0.0.18')


@pytest.fixture(scope='session')
def resources():
    yield Path(r'E:\Development\Reference-FMUs\fmusim\tests\resources')


@pytest.fixture(scope='session')
def environment():
    path_var = os.pathsep.join([
        os.environ.get('PATH', os.defpath),
        r'E:\Development\Reference-FMUs\mambaforge\Library\bin'
    ])
    yield dict(os.environ, PATH=path_var)


