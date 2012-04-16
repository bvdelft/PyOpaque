from distutils.core import setup, Extension, Command
from distutils.util import get_platform
from distutils.dir_util import copy_tree
import os
import sys

class examplesCommand(Command):
    description = "install the library in the examples directory"
    user_options = []

    def initialize_options(self):
        self.build_platlib = None
        self.build_base = 'build'
        self.path_examples = 'examples'

    def finalize_options(self):
        self.plat_name = get_platform()
        plat_specifier = ".%s-%s" % (self.plat_name, sys.version[0:3])

        if self.build_platlib is None:
            self.build_platlib = os.path.join(self.build_base,
                                              'lib' + plat_specifier)
        print self.build_platlib
        copy_tree(self.build_platlib,self.path_examples)


    def run(self):
        for cmd_name in self.get_sub_commands():
            self.run_command(cmd_name)

    sub_commands = [('build', lambda x: True)]

setup (name = 'PyOpaque',
       version = '0.0.2',
       description = 'Encapsulate python objects to make them opaque',
       author='Bart van Delf, Luciano Bello',
       author_email='<vandeba -at- chalmers.se>, <bello -at- chalmers.se>',
       url='https://github.com/bvdelft/PyOpaque',
       license='MIT License',
       ext_modules = [Extension('opaque.cOpaque', sources = ['opaque/cOpaquemodule.cpp']) ],
       packages = ['opaque'],
       cmdclass={'install_examples': examplesCommand})

