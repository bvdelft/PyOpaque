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
	set<char*> publicAttributes;
	set<char*> privateAttributes;
	bool defaultPublic;
	PyTypeObject * encapType;
	TargetClass(PyObject *, char *, set<char*>, set<char*>, bool);
};

typedef struct 
{
	PyObject_HEAD
	TargetClass* target;
} EObjectBuilder;






