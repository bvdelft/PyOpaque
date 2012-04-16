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
	set<char*, strPtrLess> publicAttributes;
	set<char*, strPtrLess> privateAttributes;
	bool defaultPublic;
	PyTypeObject * encapType;
	TargetClass(PyObject *, char *, set<char*, strPtrLess>, set<char*, strPtrLess>, bool);
};

typedef struct 
{
	PyObject_HEAD
	TargetClass* target;
} EObjectBuilder;






