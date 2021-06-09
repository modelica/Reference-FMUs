#%reset
import fmpy
import shutil
from collections import defaultdict

# provide timing & scheduling information as communication points of trace
# in format (time in ns, target, event)
commPoints =  [
(0, 'Task4_10MS', 'start'),
(0, 'Task1_5MS', 'start'),
(2000000, 'Task1_5MS', 'end'),
(2000000, 'Task2_10MS', 'start'),
(3000000, 'Task2_10MS', 'end'),
(3000000, 'Task3_20MS', 'start'),
(4000000, 'Task4_10MS', 'end'),
(4000000, 'Task5_20MS', 'start'),
(5000000, 'Task1_5MS', 'start'),
(7000000, 'Task1_5MS', 'end'),
(10000000, 'Task1_5MS', 'start'),
(10000000, 'Task4_10MS', 'start'),
(12000000, 'Task1_5MS', 'end'),
(12000000, 'Task2_10MS', 'start'),
(13000000, 'Task2_10MS', 'end'),
(14000000, 'Task4_10MS', 'end'),
(15000000, 'Task1_5MS', 'start'),
(17000000, 'Task5_20MS', 'end'),
(17000000, 'Task1_5MS', 'end'),
(19000000, 'Task3_20MS', 'end'),
(20000000, 'Task4_10MS', 'start'),
(20000000, 'Task1_5MS', 'start'),
(22000000, 'Task1_5MS', 'end'),
(22000000, 'Task2_10MS', 'start'),
(23000000, 'Task2_10MS', 'end'),
(23000000, 'Task3_20MS', 'start'),
(24000000, 'Task4_10MS', 'end'),
(24000000, 'Task5_20MS', 'start'),
(25000000, 'Task1_5MS', 'start'),
(27000000, 'Task1_5MS', 'end'),
(30000000, 'Task1_5MS', 'start'),
(30000000, 'Task4_10MS', 'start'),
(32000000, 'Task1_5MS', 'end'),
(32000000, 'Task2_10MS', 'start'),
(33000000, 'Task2_10MS', 'end'),
(34000000, 'Task4_10MS', 'end'),
(35000000, 'Task1_5MS', 'start'),
(37000000, 'Task5_20MS', 'end'),
(37000000, 'Task1_5MS', 'end'),
(39000000, 'Task3_20MS', 'end'),
(40000000, 'Task4_10MS', 'start'),
(40000000, 'Task1_5MS', 'start'),
(42000000, 'Task1_5MS', 'end'),
(42000000, 'Task2_10MS', 'start'),
(43000000, 'Task2_10MS', 'end'),
(43000000, 'Task3_20MS', 'start'),
(44000000, 'Task4_10MS', 'end'),
(44000000, 'Task5_20MS', 'start'),
(45000000, 'Task1_5MS', 'start'),
(47000000, 'Task1_5MS', 'end')]

# provide information on FMU and how to access its interface
fmuFilename = 'ControlledClocksCS.fmu'
fmuClocks = defaultdict(dict)
fmuClocks['Task1_5MS']['start'] = 'startTask1'
fmuClocks['Task1_5MS']['end'] = 'endTask1'
fmuClocks['Task2_10MS']['start'] = 'startTask2'
fmuClocks['Task2_10MS']['end'] = 'endTask2'
fmuClocks['Task3_20MS']['start'] = 'startTask3'
fmuClocks['Task3_20MS']['end'] = 'endTask3'
fmuClocks['Task4_10MS']['start'] = 'startTask4'
fmuClocks['Task4_10MS']['end'] = 'endTask4'
fmuClocks['Task5_20MS']['start'] = 'startTask5'
fmuClocks['Task5_20MS']['end'] = 'endTask5'
fmuOutputs = ['output1', 'output2']

# initialize simulation
unzipdir = fmpy.extract(fmuFilename)
modelDescription = fmpy.read_model_description(unzipdir)
vr = dict((v.name, v.valueReference) for v in modelDescription.modelVariables)

fmu = fmpy.instantiate_fmu(unzipdir, modelDescription)
fmu.enterInitializationMode() 
fmu.exitInitializationMode()

results = list()
t = 0
 
# run simulation    
for (commPointTime, commPointTarget, commPointEvent) in commPoints:
    
    dt = commPointTime/(10**9) - t
    if dt > 0:
        fmu.doStep(t, dt, fmpy.fmi3.fmi3False)  # advance time, empty impl...
        t = t + dt

    fmu.enterEventMode()
    fmu.setClock([vr[fmuClocks[commPointTarget][commPointEvent]]], [fmpy.fmi3.fmi3ClockActive])
    fmu.updateDiscreteStates()
    fmu.enterStepMode()

    results.append((commPointTime/(10**9), commPointTarget, commPointEvent, [fmu.getInt32([vr[i]])[0] for i in fmuOutputs]))

# view simulation results
print(*results, sep='\n')

# cleanup
fmu.terminate()
fmu.freeInstance()
shutil.rmtree(unzipdir, ignore_errors=True)
