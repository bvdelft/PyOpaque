from opaque import cOpaque
import __builtin__
cOpaque.encapImport(__builtin__.__import__, ["gc", "sys", "__builtin__"])
__builtin__.__import__ = cOpaque.doImport
