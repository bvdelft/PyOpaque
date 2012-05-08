from exampleCapability import fileWriteCap
from limitations import SizeLimitation

# We are allowed to write 10 times in total

# Allowed: writing to the file:
fileWriteCap("Write 1\n")
fileWriteCap("Write 2\n")
fileWriteCap("Write 3\n")

# Delegating:
delegatedWriteCap = fileWriteCap.add_limitation(SizeLimitation(10).check)

delegatedWriteCap("Write 4\n")

try:
	delegatedWriteCap("Trying to write a long line\n")
except Exception:
	print "Error caught: trying to write a line that's too long"

# Note: previous is not counted as a write

fileWriteCap("Write 5: Trying to write a long line\n")
fileWriteCap("Write 6\n")

delegatedWriteCap("Write 7\n")
delegatedWriteCap("Write 8\n")

fileWriteCap("Write 9\n")
fileWriteCap("Write 10\n")

try:
	fileWriteCap("Write 11?\n")
except Exception:
	print "Error caught: trying to write too many lines"
	
from exampleCapability import fileWriteCap
try:
	fileWriteCap("Trying to cheat\n")
except Exception:
	print "Error caught: No cheating"
