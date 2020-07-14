# config file for HelloObject

import m5
from m5.objects import *

root = Root(full_system = False)
# give time_to_wait default value
root.hello = HelloObject(time_to_wait = '2us')
# or:
# root.hello = HelloObject()
# root.hello.time_to_wait = '2us'

m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))
