import m5
from m5.objects import *

root = Root(full_system = False);
root.verilate = VerilatorObject(cycles = 50,
        latency = '1ns' ,
        stages = 1,
        startTime = 0,
        memData = "test",
        name = "VerilatorObject")

m5.instantiate()

print("Beginning Sim")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}' .format(m5.curTick(),
    exit_event.getCause()))
