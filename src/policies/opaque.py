def applyPolicy(cfgFileName,classToEncapsulate):
    #probably this is not needed. We need to enhace the directory structure
    import sys
    sys.path.insert(0, '../../src/')
    import cOpaque
    #---------------------------
    import ConfigParser

    config = ConfigParser.RawConfigParser()
    config.read(cfgFileName)
    cOpaque.registerTargetClass(classToEncapsulate)
    for attribute in config.options(classToEncapsulate.__name__):
        cOpaque.exportGetAttr(classToEncapsulate,attribute)
    cOpaque.finalizeTargetClass(classToEncapsulate,classToEncapsulate.__name__)
    return cOpaque.builder(classToEncapsulate).build
