from fmpy import *


filename = 'build/dist/LinearTransform.fmu'
# filename = 'build/dist/Dahlquist.fmu'

md = read_model_description(filename)

result = simulate_fmu(filename, output=['m', 'y', 'A'])

plot_result(result)
