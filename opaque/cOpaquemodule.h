struct TargetClass;

typedef struct 
{
	PyObject_HEAD
	PyObject* objPointer;
	TargetClass* target;
} EncapsulatedObject;

typedef struct 
{
	PyObject_HEAD
	PyObject* attPointer;
} EncapsulatedAttribute;

struct TargetClass 
{
	PyObject* target;
	char * name;
	set<char*, strPtrLess> publicAttributes;
	set<char*, strPtrLess> privateAttributes;
	bool defaultPublic;
	TargetClass(PyObject *, char *, set<char*, strPtrLess>, set<char*, strPtrLess>, bool);
};

class EncapsulatedType : public PyTypeObject {
  public:
    TargetClass* target;
};

