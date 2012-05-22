from capability import Capability
from limitations import CounterLimitation

fd = file('/tmp/log.txt','w')

fileWriteCap = Capability(fd.write, [CounterLimitation(5).check])

# Preventing direct access to the file:

del fd

from opaque import disableDangerousImports
disableDangerousImports()
