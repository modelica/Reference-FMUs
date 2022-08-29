""" Simulate the FMUs, generate the reference results and plots and convert the readme.md files to HTML """
import shutil
from pathlib import Path
from subprocess import check_call
import jinja2
from fmpy.util import plot_result, read_csv
import os
import markdown2


root_dir = Path(__file__).parent
dist_dir = root_dir / 'build' / 'dist'
temp_dir = root_dir / 'build' / 'temp'

info = [
    ('BouncingBall',    {}, 0.01),
    ('Dahlquist',       {}, 0.2),
    ('LinearTransform', {}, 1),
    ('Feedthrough',     {'real_fixed_param': 1}, 0.2),
    ('Resource',        {}, 0.2),
    ('Stair',           {}, 10),
    ('VanDerPol',       {}, 0.1),
]

loader = jinja2.FileSystemLoader(searchpath=root_dir)
environment = jinja2.Environment(loader=loader, trim_blocks=True)
template = environment.get_template(f'template.html')

for model_name, start_values, output_interval in info:

    print(model_name)

    ref_csv = root_dir / model_name / f'{model_name}_ref.csv'
    ref_svg = root_dir / model_name / f'{model_name}_ref.svg'

    if os.name == 'nt':
        exe = temp_dir / f'{model_name}_me.exe'
    else:
        exe = temp_dir / f'{model_name}_me'

    out_csv = temp_dir / f'{model_name}_me_out.csv'

    check_call([exe], cwd=temp_dir)

    shutil.copyfile(src=out_csv, dst=ref_csv)

    ref = read_csv(ref_csv)
    plot_result(ref, events=True, filename=ref_svg)

    md_file = root_dir / model_name / 'readme.md'
    html_file = root_dir / model_name / 'readme.html'

    content = markdown2.markdown_path(md_file, extras=['tables', 'fenced-code-blocks'])

    html = template.render(model_name=model_name, content=content)

    with open(html_file, 'w') as f:
        f.write(html)
