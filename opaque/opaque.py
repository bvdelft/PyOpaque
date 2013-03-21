import cOpaque

class opaque(object):
    def __init__(self, public=[], private=[], default=True, allowExtension=True):
        '''
        Class decorator to convert a class in an opaque class.
        public:  List of attributes/methods that are allow
        private: List of attributes/methods that are deny
        default:  Boolean. If an attribute/method is not in the public or private list, is it allow?
        allowExtension:  The class and instances can be dynamically extended'''
        self.privarg = [ j for j in private ]
        self.pubarg = [ j for j in public ]
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
    classToEncapsulate=opaque(toPublic,toPrivate,default,allowExtension)(classToEncapsulate)
    return classToEncapsulate
       
def disableDangerousImports(ListOfModules=[]):
	try:
		import __builtin__
	except RuntimeError:
		raise RuntimeError('It is not possible to run disableDangerousImports twice.')
	del __builtin__.file
	cOpaque.encapImport(__builtin__.__import__, ["gc", "sys", "__builtin__"]+ListOfModules)
	__builtin__.__import__ = cOpaque.doImport
	del __builtin__

def disableDangerousCalls(ListOfModules=[]):
	blacklist={
		'gc':['get_objects'],
		'sys':['modules']
	}
	import __builtin__
	for (module,attributes) in blacklist.iteritems():
		for attr in attributes: __builtin__.delattr(__import__(module),attr)
