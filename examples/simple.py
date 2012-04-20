import random
import types
from opaque import cOpaque

class opaque(object):
    def __init__(self, arg1, arg2, arg3):  
        self.pubarg = arg1
        self.privarg = arg2
        self.default = arg3

    def __call__(self, klass):
        return cOpaque.makeOpaque(klass, self.pubarg, self.privarg , self.default )
            
@opaque(['pub'], ['data'] , True)
class simple():
     def __init__(self,data,pub):
          self.data = data
          self.pub = pub
          self.smalltest = "This is a small test"

# simple = opaque(['pub'], ['data'] , True)(simple)

simple1=simple(random.randint(0,100000),{1:'iamsuperbart'})
