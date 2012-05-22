import random
from opaque import cOpaque

#@opaque(['pub'], ['data'] , False)
class simple():
     def __init__(self,data,pub):
          self.data = data
          self.pub = pub
          self.smalltest = "This is a small test"

# simple = opaque(['pub'], ['data'] , True)(simple)

cOpaque.enableDebug()
simple = cOpaque.makeOpaque(simple,['pub'], ['data'] , True)

simple1=simple(random.randint(0,100000),{1:'iamsuperbart'})

class M(simple):
	pass

m=M(45,12)
