""" Simulate the FMUs, generate the reference results and plots and convert the readme.md files to HTML """
import shutil
from subprocess import check_call

from fmpy.util import plot_result, read_csv
import os
import markdown2


header1 = """
<html>
<head>
"""

header2 = """
<style>
body {
    font-family: Helvetica, Arial, Sans-Serif;
    margin: 2em;
}

div.container {
    max-width: 750px;
    margin: auto;
}


h1, h2 {
    border-bottom: 1px solid #eaecef;
    padding-bottom: .3em;
}

pre {
    background-color: #f6f8fa;
    border-radius: 3px;
    font-size: 85%;
    line-height: 1.45;
    overflow: auto;
    padding: 16px;
}

table {
    border-collapse: collapse;
    border-spacing: 0;
}

th, td {
    font-size: 0.9em;
    border: 1px solid #dfe2e5;
    padding: 6px 13px;
}
</style>
</head>
<body>
<div class="container">
"""

footer = """
</div>
</body>
</html>
"""

root_dir = os.path.dirname(__file__)
dist_dir = os.path.join(root_dir, 'build', 'dist')
temp_dir = os.path.join(root_dir, 'build', 'temp')

src_dir = os.path.dirname(__file__)

info = [
    ('BouncingBall',    {}, 0.01),
    ('Dahlquist',       {}, 0.2),
    ('LinearTransform', {}, 1),
    ('Feedthrough',     {'real_fixed_param': 1}, 0.2),
    ('Resource',        {}, 0.2),
    ('Stair',           {}, 10),
    ('VanDerPol',       {}, 0.1),
]

for model_name, start_values, output_interval in info:

    print(model_name)

    fmu = os.path.join(dist_dir, model_name + '.fmu')

    ref_csv = os.path.join(src_dir, model_name, model_name + '_ref.csv')
    ref_svg = os.path.join(src_dir, model_name, model_name + '_ref.svg')
    in_csv  = os.path.join(src_dir, model_name, model_name + '_in.csv')

    if os.path.isfile(in_csv):
        input = read_csv(in_csv)
    else:
        input = None

    exe = os.path.join(temp_dir, model_name + '_me.exe')
    out_csv = os.path.join(temp_dir, model_name + '_me_out.csv')

    check_call([exe], cwd=temp_dir)

    shutil.copyfile(src=out_csv, dst=ref_csv)

    ref = read_csv(ref_csv)
    plot_result(ref, events=True, filename=ref_svg)

    md_file = os.path.join(src_dir, model_name, 'readme.md')
    html_file = os.path.join(src_dir, model_name, 'readme.html')

    md = markdown2.markdown_path(md_file, extras=['tables', 'fenced-code-blocks'])

    with open(html_file, 'w') as html:
        html.write(header1)
        html.write("<title>%s</title>" % model_name)
        html.write(header2)
        html.write(md)
        html.write(footer)
