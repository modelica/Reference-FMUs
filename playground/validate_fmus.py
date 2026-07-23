import os
import subprocess

# validate the merged FMUs
for root, _dirs, files in os.walk("."):
    for file in files:
        if file.endswith(".fmu"):
            fmu_path = os.path.join(root, file)
            out = subprocess.check_output(["fmusim", "validate", fmu_path])
            