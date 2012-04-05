from distutils.core import setup, Extension

module1 = Extension('cOpaquemodule',
                    sources = ['cOpaquemodule.cpp'])          
setup (name = 'cOpaque Module',
       version = '0.0.2',
       description = 'Encapsulate python objects to make them opaque',
       ext_modules = [module1])
