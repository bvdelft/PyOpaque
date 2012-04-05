struct TargetClass;

typedef struct 
{
	PyObject_HEAD
	PyObject* objPointer;
	TargetClass* target;
} EncapsulatedObject;

struct TargetClass 
{
	PyObject* target;
	char * name;
	map<char*,PyObject*> attributes;
	PyTypeObject * encapType;
	TargetClass(PyObject *, char *, map<char*,PyObject*>);
};

typedef struct 
{
	PyObject_HEAD
	TargetClass* target;
} EObjectBuilder;






