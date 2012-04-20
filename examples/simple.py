import random
from opaque import opaque

@opaque(['pub'], ['data'] , True)
class simple():
     def __init__(self,data,pub):
          self.data = data
          self.pub = pub
          self.smalltest = "This is a small test"

# simple = opaque(['pub'], ['data'] , True)(simple)

simple1=simple(random.randint(0,100000),{1:'iamsuperbart'})
