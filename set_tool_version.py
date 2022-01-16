import os.path
import subprocess
import sys


def set_tool_version(filename, git_executable='git'):
    """ Set the Git tag or hash in the generationTool attribute if the repo is clean """

    cwd = os.path.dirname(__file__)
    
    changed_files = subprocess.check_output([git_executable, 'status', '--porcelain', '--untracked=no'], cwd=cwd).decode('ascii').strip()

    if changed_files:
        return

    version = subprocess.check_output([git_executable, 'tag', '--contains'], cwd=cwd).decode('ascii').strip()

    if not version:
        version = subprocess.check_output([git_executable, 'rev-parse', '--short', 'HEAD'], cwd=cwd).decode('ascii').strip()

    if not version:
        return

    with open(filename, 'r') as f:
        lines = f.read()

    lines = lines.replace('"Reference FMUs (development build)"', f'"Reference FMUs ({version})"')

    with open(filename, 'w') as f:
        f.write(lines)


if __name__ == '__main__':

    try:
        if len(sys.argv) > 2:
            set_tool_version(sys.argv[1], sys.argv[2])
        elif len(sys.argv) > 1:
            set_tool_version(sys.argv[1])
        else:
            raise RuntimeError('Not enough arguments')
    except Exception as e:
        print(e)
