import cOpaque

def applyPolicy(classToEncapsulate,cfgFileName='opaque.cfg'):
    import ConfigParser
    #cOpaque.enableDebug()
    config = ConfigParser.RawConfigParser(allow_no_value=True)

    if type(cfgFileName)==str: config.read(cfgFileName)
    else: config.readfp(cfgFileName)

    toPrivate=[]
    toPublic =[]
    for (attribute,property) in config.items(classToEncapsulate.__name__):
        if property == None:
            if attribute == 'default-private': default=False
            if attribute == 'default-public' : default=True
        if property == 'private': toPrivate.append(attribute)
        if property == 'public' :  toPublic.append(attribute)
    classToEncapsulate=cOpaque.makeOpaque(classToEncapsulate,toPublic,toPrivate,default )
    return classToEncapsulate
