import os
import sys


autofix = False


def lint_file(filename):

    messages = []

    with open(filename, 'r', encoding='utf-8') as file:

        for i, line in enumerate(file):

            c = [ord(c) < 128 for c in line]

            if not all(c):
                marker = ''.join([' ' if x else '^' for x in c])
                message = "Non-ASCII characters found:\n%s\n%s" % (line[:-1], marker)
                messages.append((message, i))

            if len(line) > 1 and line[-2] in {' '}:
                messages.append(("Whitespace at the end of the line", i))

            if '\t' in line:
                messages.append(("Tab character found", i))

            if '\r' in line:
                messages.append(("Carriage return character found", i))

    return messages


def fix_file(filename):

    lines = []

    with open(filename, 'r', encoding='utf-8') as file:

        for line in file:
            line = line.rstrip() + '\n'
            line = line.replace('\t', '    ')
            lines.append(line)

    if lines[-1] != '':
        lines.append('')

    with open(filename, 'w') as file:
        file.writelines(lines)


total_problems = 0

top = os.path.abspath(__file__)
top = os.path.dirname(top)

print("Linting files in %s" % top)

for root, dirs, files in os.walk(top, topdown=True):

    excluded = ['build', 'fmi1_cs', 'fmi1_me', 'fmi2', 'fmi3', 'fmus', 'fmusim', 'ThirdParty', 'venv']

    # skip build, git, and cache directories
    dirs[:] = [d for d in dirs if d not in excluded and not os.path.basename(d).startswith(('.', '_'))]

    for file in files:

        if not file.lower().endswith(('.h', '.c', '.md', '.html', '.csv', '.txt', '.xml')):
            continue

        filename = os.path.join(root, file)

        print(filename)

        messages = lint_file(filename)

        if messages and autofix:
            fix_file(filename)
            messages = lint_file(filename)

        if messages:

            print("%d problems found in %s:" % (len(messages), filename))
            print()

            for message, line in messages:
                print("line %d: %s" % (line + 1, message))
                print()

        total_problems += len(messages)

print("Total problems found: %d" % total_problems)

sys.exit(total_problems)
