from limitations import SizeLimitation
from exampleCapability import fileWriteCap

fileWriteCap("Write 1\n")

delegatedWriteCap = fileWriteCap.add_limitation(SizeLimitation(8).check)

delegatedWriteCap("Write 2\n")
delegatedWriteCap("Write 3: Trying to write a long line\n")
