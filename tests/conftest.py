from pathlib import Path

import pytest


def pytest_addoption(parser):
    parser.addoption('--cmake-generator')
    parser.addoption('--cmake-architecture')


@pytest.fixture(scope='session')
def work_dir():
    yield Path(__file__).parent / 'work'
