from pathlib import Path

import pytest


def pytest_addoption(parser):
    parser.addoption('--cmake-generator')
    parser.addoption('--cmake-architecture')
    parser.addoption('--arch', default='x86_64')


@pytest.fixture(scope='session')
def work_dir():
    yield Path(__file__).parent / 'work'


@pytest.fixture(scope='session')
def arch(request):
    yield request.config.getoption('--arch')
