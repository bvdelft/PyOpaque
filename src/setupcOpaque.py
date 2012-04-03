from distutils.core import setup, Extension

module1 = Extension('privmodule',
                    sources = ['privmodule.c'])          
setup (name = 'Private Module',
       version = '1.0',
       description = 'Making stuff private',
       ext_modules = [module1])