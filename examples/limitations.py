from opaque import opaque

@opaque(['check'],[],False)
class SizeLimitation():
    def __init__(self, max):
        self.max = max
    def check(self,*args,**kargs):
        return len(args[0]) <= self.max
        
@opaque(['check'],[],False)
class CounterLimitation():
    def __init__(self, max):
        self.counter = 0
        self.max = max
    def check(self,*args,**kargs):
        self.counter += 1
        return self.counter <= self.max
