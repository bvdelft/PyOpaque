import cOpaque

class opaque(object):
    def __init__(self, pubargs=[], privargs=[], default=True, allowExtension=True):
        self.privarg = [ j for j in privargs ]
        self.pubarg = [ j for j in pubargs ]
        self.default = default
        self.allowExtension = allowExtension

    def __call__(self, klass):
        self.private_attr=filter(lambda x: x[0]=='_',dir(klass)) #some of them could be dangerous, like __dict__
        opaquedKlass = cOpaque.makeOpaque(klass, self.pubarg, self.privarg+self.private_attr , self.default )
        if self.allowExtension:
             return type('wrappedOpaque',(opaquedKlass,),{})
        else:
             return opaquedKlass

def applyPolicy(classToEncapsulate,cfgFileName='opaque.cfg'):
    import ConfigParser
    config = ConfigParser.RawConfigParser(allow_no_value=True)

    if type(cfgFileName)==str: config.read(cfgFileName)
    else: config.readfp(cfgFileName)

    toPrivate=[]
    toPublic =[]
    allowExtension=True
    for (attribute,property) in config.items(classToEncapsulate.__name__):
        if property == None:
            if attribute == 'default-private': default=False
            if attribute == 'default-public' : default=True
            if attribute == 'disallow-extension' : allowExtension=False
        if property == 'private': toPrivate.append(attribute)
        if property == 'public' :  toPublic.append(attribute)
    classToEncapsulate=opaque(toPublic,toPrivate,default )(classToEncapsulate)
    return classToEncapsulate
        
def disableDangerousImports():
	import __builtin__
	del __builtin__.file
	cOpaque.encapImport(__builtin__.__import__, ["gc", "sys", "__builtin__"])
	__builtin__.__import__ = cOpaque.doImport
	del __builtin__
