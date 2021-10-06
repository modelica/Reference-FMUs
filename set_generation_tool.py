"""
Set the generation tool to each modelDescription template
"""

import os.path
from subprocess import check_output

import lxml.etree as ET
from tidylib import tidy_document

model_names = (
    'BouncingBall',
    'Dahlquist',
    'LinearTransform',
    'Feedthrough',
    'Resource',
    'Stair',
    'VanDerPol',
)

xml_files = (
    'FMI1CS.xml',
    'FMI1ME.xml',
    'FMI2.xml',
    'FMI3.xml',
)

root_dir = os.path.dirname(__file__)

parser = ET.XMLParser(encoding='utf-8')

git_vs = check_output(['git', 'describe', '--tags']).decode('ascii').strip()

generation_tool = 'Reference-FMUs {0}'.format(git_vs)

for model_name in model_names:
    for xml_file in xml_files:
        xml_path = os.path.join(root_dir, model_name, xml_file)
        if not os.path.isfile(xml_path):
            continue
        xml_tree = ET.parse(xml_path, parser=parser)
        xml_tree.getroot().attrib['generationTool'] = generation_tool
        xml_doc = tidy_document(
            ET.tostring(xml_tree),
            options={'indent-attributes': 'yes', 'input-xml': 'yes', 'newline': 'LF'})[0]
        with open(xml_path, 'wb') as fd:
            fd.write(b'<?xml version="1.0" encoding="UTF-8"?>\n')
            fd.write(xml_doc)
