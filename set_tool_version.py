import subprocess
import sys
import os


def set_tool_version(filename):
    """ Set the Git tag or hash in the generationTool attribute if the repo is clean """

    cwd = os.path.dirname(__file__)

    changed_files = subprocess.check_output(['git', 'status', '--porcelain', '--untracked=no'], cwd=cwd).decode('ascii').strip()

    if changed_files:
        return

    version = subprocess.check_output(['git', 'tag', '--contains'], cwd=cwd).decode('ascii').strip()

    if not version:
        version = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], cwd=cwd).decode('ascii').strip()

    if not version:
        return

    with open(filename, 'r') as f:
        lines = f.read()

    lines = lines.replace('"Reference FMUs (development build)"', f'"Reference FMUs ({version})"')

    with open(filename, 'w') as f:
        f.write(lines)


if __name__ == '__main__':

    try:
        set_tool_version(sys.argv[1])
    except Exception as e:
        print(e)
