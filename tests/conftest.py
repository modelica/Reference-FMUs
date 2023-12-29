from pathlib import Path

import pytest

import fmpy


def pytest_addoption(parser):
    parser.addoption('--platform', default=None)


@pytest.fixture(scope='session')
def work_dir():
    yield Path(__file__).parent / 'work'


@pytest.fixture(scope='session')
def platform(request):
    platform = request.config.getoption('--platform')
    if platform is None:
        platform = fmpy.platform_tuple
    yield platform


@pytest.fixture(scope='session')
def arch(platform):
    architecture, _ = platform.split('-')
    yield architecture

