from pathlib import Path
from subprocess import check_call
from fmpy.util import plot_result, read_csv


dist = Path(r'E:\Development\Reference-FMUs\fmus')

fmusim = r'E:\Development\Reference-FMUs\build\fmusim\Debug\fmusim.exe'  # dist / 'fmusim-windows' / 'fmusim.exe'

filename = dist / '2.0' / 'BouncingBall.fmu'

output_filename = dist / '2.0' / 'BouncingBall_ref.csv'

command = [
    str(fmusim),
    '--interface-type', 'me',
    '--solver', 'cvode',
    '--output-interval', '0.05',
    '--stop-time', '3',
    '--output-file', str(output_filename),
    str(filename)
]

print(' '.join(command))

check_call(command)

result = read_csv(output_filename)

plot_result(result, markers=True, events=True)
