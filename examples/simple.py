import random

class simple():
     def __init__(self,data,pub):
          self.data = data
          self.pub = pub
          self.smalltest = "This is a small test"

'''Uncomment the following lines to protect the password field'''
from opaque import cOpaque
simple=cOpaque.makeOpaque(simple, ['pub'], ['data'] , True )
del cOpaque
simple1=simple(random.randint(0,100000),{1:'iamsuperbart'})
