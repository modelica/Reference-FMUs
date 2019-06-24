import unittest
import subprocess
import os
import shutil


fmus_dir = os.path.join(os.path.dirname(__file__), 'fmus')  # /path/to/fmi-cross-check/fmus
test_fmus_version = '0.0.2'

try:
    from fmpy import simulate_fmu
    from fmpy.util import compile_platform_binary

    fmpy_available = True
except:
    print("FMPy not available. Skipping simulation.")
    fmpy_available = False


fmi3_available = False

try:
    import fmpy

    fmi3_available = 'fmi3' in dir(fmpy)
except:
    pass

if not fmi3_available:
    print("FMI 3.0 support in FMPy not available. Skipping FMI 3.0 simulation.")


test_fmus_dir = os.path.dirname(__file__)

models = ['BouncingBall', 'Dahlquist', 'Resource', 'Stair', 'VanDerPol', 'Feedthrough']

if os.name == 'nt':
    generator = 'Visual Studio 15 2017 Win64'
else:
    generator = 'Unix Makefiles'


def copy_to_cross_check(build_dir, model_names, fmi_version, fmi_types):
    if fmus_dir is None:
        return

    for fmi_type in fmi_types:
        for model in model_names:
            target_dir = os.path.join(fmus_dir, fmi_version, fmi_type, fmpy.platform, 'Test-FMUs', test_fmus_version, model)
            if not os.path.exists(target_dir):
                os.makedirs(target_dir)
            shutil.copy(os.path.join(build_dir, 'dist', model + '.fmu'), target_dir)
            shutil.copy(os.path.join(test_fmus_dir, model, model + '_ref.csv'), target_dir)
            shutil.copy(os.path.join(test_fmus_dir, model, model + '_ref.opt'), target_dir)


class BuildTest(unittest.TestCase):
    """ Build all variants of the Test FMUs and simulate the default experiment """

    @classmethod
    def setUpClass(cls):
        # clean up
        for name in ['fmi1_me', 'fmi1_cs', 'fmi2', 'fmi3']:
            build_dir = os.path.join(test_fmus_dir, name)
            if os.path.isdir(build_dir):
                print("Removing " + build_dir)
                shutil.rmtree(build_dir)

    def validate(self, build_dir, fmi_types=['ModelExchange', 'CoSimulation'], models=models, compile=False):

        from fmpy.util import read_csv, validate_result

        for model in models:

            print(model)

            fmu_filename = os.path.join(build_dir, 'dist', model + '.fmu')

            if model == 'Feedthrough':
                start_values = {'real_fixed_param': 1, 'string_param': "FMI is awesome!"}
                in_csv = os.path.join(test_fmus_dir, model, model + '_in.csv')
                input = read_csv(in_csv)
            else:
                start_values = {}
                input = None

            ref_csv = os.path.join(test_fmus_dir, model, model + '_ref.csv')

            for fmi_type in fmi_types:

                ref = read_csv(ref_csv)

                if compile:
                    compile_platform_binary(fmu_filename)

                result = simulate_fmu(fmu_filename,
                                      fmi_type=fmi_type,
                                      start_values=start_values,
                                      input=input)

                dev = validate_result(result, ref)

                self.assertLess(dev, 0.2, "Failed to validate " + model)

    def test_fmi1_me(self):

        build_dir = os.path.join(test_fmus_dir, 'fmi1_me')

        if not os.path.exists(build_dir):
            os.makedirs(build_dir)

        subprocess.call(['cmake', '-G', generator, '-DFMI_VERSION=1', '-DFMI_TYPE=ME', '..'], cwd=build_dir)
        subprocess.call(['cmake', '--build', '.', '--config', 'Release'], cwd=build_dir)

        model_names = ['BouncingBall', 'Dahlquist', 'Stair', 'VanDerPol']

        if fmpy_available:
            self.validate(build_dir,
                          fmi_types=['ModelExchange'],
                          models=model_names)

        copy_to_cross_check(build_dir=build_dir, model_names=model_names, fmi_version='1.0', fmi_types=['me'])

    def test_fmi1_cs(self):

        build_dir = os.path.join(test_fmus_dir, 'fmi1_cs')

        if not os.path.exists(build_dir):
            os.makedirs(build_dir)

        subprocess.call(['cmake', '-G', generator, '-DFMI_VERSION=1', '-DFMI_TYPE=CS', '..'], cwd=build_dir)
        subprocess.call(['cmake', '--build', '.', '--config', 'Release'], cwd=build_dir)

        model_names = ['BouncingBall', 'Dahlquist', 'Resource', 'Stair', 'VanDerPol']

        if fmpy_available:
            self.validate(build_dir,
                          fmi_types=['CoSimulation'],
                          models=model_names)

        copy_to_cross_check(build_dir=build_dir, model_names=model_names, fmi_version='1.0', fmi_types=['cs'])

    def test_fmi2(self):

        build_dir = os.path.join(test_fmus_dir, 'fmi2')

        if not os.path.exists(build_dir):
            os.makedirs(build_dir)

        subprocess.call(['cmake', '-G', generator, '-DFMI_VERSION=2', '..'], cwd=build_dir)
        subprocess.call(['cmake', '--build', '.', '--config', 'Release'], cwd=build_dir)

        if fmpy_available:
            self.validate(build_dir)
            self.validate(build_dir, compile=True)

        copy_to_cross_check(build_dir=build_dir, model_names=models, fmi_version='2.0', fmi_types=['cs', 'me'])

    def test_fmi3(self):

        print('FMI 3.0')

        build_dir = os.path.join(test_fmus_dir, 'fmi3')

        if not os.path.exists(build_dir):
            os.makedirs(build_dir)

        subprocess.call(['cmake', '-G', generator, '-DFMI_VERSION=3', '..'], cwd=build_dir)
        subprocess.call(['cmake', '--build', '.', '--config', 'Release'], cwd=build_dir)

        if fmi3_available:
            self.validate(build_dir, models=['BouncingBall', 'Dahlquist', 'Feedthrough', 'Resource', 'Stair', 'VanDerPol'])

        copy_to_cross_check(build_dir=build_dir, model_names=models, fmi_version='3.0', fmi_types=['cs', 'me'])

if __name__ == '__main__':
    unittest.main()
