from opaque import opaque,cOpaque

cOpaque.enableDebug()

@opaque(['__call__'],['action','limitations'],True)
class Capability(object):
    def __init__(self,action,limitations=[]):
        self.action=action
        self.limitations=limitations

    def __call__(self, *args, **kargs):
        for l in self.limitations:
            if not l(*args, **kargs): raise Exception('Permission denied to practise this capabily.')
        self.action(*args,**kargs)

    def add_limitation(self,limitation):
        return Capability(self.action,self.limitations+[limitation])

class WriteCapability(Capability):
    def __init__(self,fd):
        super(WriteCapability, self).__init__(fd.write)


class CloseCapability(Capability):
    def __init__(self,fd):
        super(CloseCapability, self).__init__(fd.close)

class CounterLimitation():
    def __init__(self, max):
        self.counter=0
        self.max = max
    def check(self,*args,**kargs):
        self.counter+=1
        return self.counter <= self.max

class Capabilities(): pass

fd = file('/tmp/log.txt','w')

cap=Capabilities()
cap.write=Capability(fd.write,[CounterLimitation(4).check])
cap.close=Capability(fd.close)

fd=cap
