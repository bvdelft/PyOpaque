from capability import Capability
from limitations import CounterLimitation

fd = file('/tmp/log.txt','w')

fileWriteCap = Capability(fd.write, [CounterLimitation(10).check])

# Preventing direct access to the file:

del fd
## Comment out the lines below to use ipython instead of python -i
from opaque import disableDangerousImports
disableDangerousImports()
