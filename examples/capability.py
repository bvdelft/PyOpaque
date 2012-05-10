from opaque import opaque

@opaque(['__call__'],['action','limitations'],True)
class Capability(object):
    def __init__(self,action,limitations=[]):
        self.action=action
        self.limitations=limitations

    def __call__(self, *args, **kargs):
        for l in self.limitations:
            if not l(*args, **kargs): 
            	raise Exception('Permission denied to exercise this capability.')
        self.action(*args,**kargs)

    def add_limitation(self,limitation):
    	# It is important that the new limitation is added at the front, so
    	# that it is checked first (e.g. when the new limitation returns False,
    	# the other limitations are not called and do not change their state
    	# incorrectly.)
        return Capability(self.action, [limitation] + self.limitations)

