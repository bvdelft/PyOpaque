#include <Python.h>
#include <set>
using namespace std;
#include "cOpaquemodule.h"

bool DEBUG = false;

static void debug(const char * text) {
	if (DEBUG)
		printf("%s",text);
}

/*----------------------------------------------------------------------------*/
////// EncapsulatedObject //////////////////////////////////////////////////////

/**
* Deallocation of encapsulating object. Deallocates both the object that is 
* encapsulated as well as the encapsulating object.
**/
static void encap_dealloc(PyObject* self)
{
	PyObject_Del(((EncapsulatedObject*) self)->objPointer);
	PyObject_Del(self);
}

/**
* __getattr__ for EncapsulatedObject. 
**/
static PyObject * EOGetAttr(PyObject * _eobject, char * attr)
{
	EncapsulatedObject * eobject = (EncapsulatedObject *) _eobject;
	

	set<char*> publicAttributes = eobject->target->publicAttributes;

	debug("DEBUG: Checking for public attributes");

	if (publicAttributes.find(attr) != publicAttributes.end()) 
	{ // Public attribute, flow allowed:
		PyObject * res =  PyObject_GetAttrString(eobject->objPointer,attr);
		return res;
	}
	
	set<char*> privateAttributes = eobject->target->privateAttributes;
	
	debug("DEBUG: Checking for private attributes");

	if (privateAttributes.find(attr) != privateAttributes.end()) 
	{ // Private attribute, flow not allowed:
		(void) PyErr_Format(PyExc_RuntimeError, 
                "The policy of attribute '%s' of '\%s' object disallows access",
                attr, eobject->target->name);
		return NULL;
	}
	

	debug("DEBUG: Default public is ");
	if (eobject->target->defaultPublic)
		debug("true.\n");
	else
		debug("false.\n");
	
	
	if (eobject->target->defaultPublic) {
	
		PyObject * res =  PyObject_GetAttrString(eobject->objPointer,attr);
		return res;
	
	} else {
	
		(void) PyErr_Format(PyExc_RuntimeError, 
                "The policy of attribute '%s' of '\%s' object disallows access",
                attr, eobject->target->name);
		return NULL;
		
	}
	
}

/**
* Generates a new encapsulated object type with the provided name, such that the
* correct name appears in message, __name__ etc.
**/
PyTypeObject* makeEncapObjectType(char * name) 
{
	PyTypeObject* encapObjectType = (PyTypeObject*)malloc(sizeof(PyTypeObject));
	memset(encapObjectType, 0, sizeof(PyTypeObject));

	// Copy the standard PyObject header
	PyTypeObject dummy = {PyObject_HEAD_INIT((PyTypeObject*)NULL)};
	memcpy(encapObjectType, &dummy, sizeof(PyObject));

	// Set the type-instance specific name
	char _name[strlen(name)];
	strcpy (_name,name);
	encapObjectType->tp_name = const_cast<char*>(_name);

	// The __getattr__ function (= applying the policies)    
	encapObjectType->tp_getattr = EOGetAttr;

	// Default stuff
	encapObjectType->tp_basicsize = sizeof(EncapsulatedObject);
	// TODO Not really sure what line below does.
	encapObjectType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
								Py_TPFLAGS_CHECKTYPES;
	encapObjectType->tp_doc = NULL;
	encapObjectType->tp_dealloc = encap_dealloc;

	if(PyType_Ready(encapObjectType)<0)
		printf("Err: PyType_Ready: class %s", name);

	return encapObjectType;
}

/*----------------------------------------------------------------------------*/
////// TargetClass /////////////////////////////////////////////////////////////

/**
* Constructor. Generates a new encapsulated object type for this target / name.
**/
TargetClass::TargetClass(PyObject * _target, char * _name,
                         set<char*> _publicAttributes, 
                         set<char*> _privateAttributes,
                         bool _defaultPublic) 
{
	Py_XINCREF(_target);
	target = _target;
	name = _name;
	publicAttributes = _publicAttributes;
	privateAttributes = _privateAttributes;
	defaultPublic = _defaultPublic;
	encapType = makeEncapObjectType(_name);
}

/*----------------------------------------------------------------------------*/
////// EObjectBuilder //////////////////////////////////////////////////////////

/**
* Deallocator for EObjectBuilder, standard.
**/
static void eObjectBuilder_dealloc(PyObject* self)
{
  PyObject_Del(self);
}

/**
* Simulates the constructor for an encapsulated object. Creates an object
* instance of the target to which this build method belongs using the provided
* arguments. Wraps this object in an EncapsulatedObject and returns the result.
**/
static PyObject * build(PyObject* self, PyObject* args) {
	EObjectBuilder *ob = (EObjectBuilder*)self;

	// Create the object to be encapsulated
	PyObject* theObject; 
	theObject = PyObject_CallObject(ob->target->target, args); 
	if (theObject == NULL)
		return NULL;
	

	// Create an encapsulating object
	EncapsulatedObject * encap =
		(EncapsulatedObject *) PyObject_New(EncapsulatedObject, 
										ob->target->encapType);
	
	encap->target = ob->target;
	// Py_XINCREF(ob->target->target);

	// Store the object in the encapsulating one
	encap->objPointer = theObject;
	Py_XINCREF(theObject);

	// Return the result
	PyObject * res = (PyObject*) encap;
	Py_XINCREF(res);
	return res;
}

/**
* The only method for EObjectBuilder is the constructor-mimicing function build.
**/
static PyMethodDef EOBMethods[] = 
{
  {"build",build,METH_VARARGS,"Build method"} ,
  {NULL, NULL, 0, NULL} 
} ;

/**
* Constructs the EObjectBuilderType. Easier to make this function than to use 
* the struct-approach.
**/
PyTypeObject* makeEObjectBuilderType() 
{
	PyTypeObject * eObjectBuilderType = 
		(PyTypeObject *)malloc(sizeof(PyTypeObject));

	memset(eObjectBuilderType, 0, sizeof(PyTypeObject));
	PyTypeObject dummy = {PyObject_HEAD_INIT((PyTypeObject*)NULL)};
	memcpy(eObjectBuilderType, &dummy, sizeof(PyObject));

	eObjectBuilderType->tp_name = const_cast<char*>("EObjectBuilder");
	eObjectBuilderType->tp_basicsize = sizeof(EObjectBuilder);

	// TODO Not really sure what line below does.
	eObjectBuilderType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
								Py_TPFLAGS_CHECKTYPES;
	eObjectBuilderType->tp_doc = NULL;

	eObjectBuilderType->tp_dealloc = eObjectBuilder_dealloc;

	eObjectBuilderType->tp_methods = EOBMethods;

	if(PyType_Ready(eObjectBuilderType)<0)
		printf("Err: PyType_Ready: eObjectBuilderType");

	return eObjectBuilderType;

}

/**
* There can be only one (EObjectBuilderType).
**/
static PyTypeObject* EObjectBuilderType = makeEObjectBuilderType();

/**
* Creates a new EObjectBuilder for the specified target class.
**/                             
static PyObject* encapBuilder_init(TargetClass* target) 
{
	EObjectBuilder* encapB;
	encapB = PyObject_NEW(EObjectBuilder, EObjectBuilderType);
	encapB->target = target;
	// Py_XINCREF(target->target);
	return (PyObject*) encapB;
}

/*----------------------------------------------------------------------------*/
////// cOpaque /////////////////////////////////////////////////////////////////

/**
* Auxilary function. Returns the name of an object as module.name. Returns NULL
* if unable to construct the name.
**/
static char * getObjectFullName(PyObject * obj) {
	
	if (! PyObject_HasAttrString(obj, "__name__"))
		return NULL;
	PyObject * nameattr = PyObject_GetAttrString(obj, "__name__");
	if (! PyString_Check(nameattr)) 
		return NULL;
	char * name = PyString_AsString(nameattr);
	
	if (! PyObject_HasAttrString(obj, "__module__"))
		return NULL;
	PyObject * moduleattr = PyObject_GetAttrString(obj, "__module__");
	if (! PyString_Check(moduleattr)) 
		return NULL;
	char * module = PyString_AsString(moduleattr);
	
	return strcat(strcat(module,"."),name);
}

/**
* List of classes that have been made opaque - a class can only be made opaque
* once, otherwise we run into some problems.
**/
static set<PyObject*> knownTargets;

/**
* Make a class opaque using the specified list of attribute/policy combinations.
* Returns a function that passes its arguments to the construction of an 
* encapsulated object for the provided class. The attributes are accessible as
* according to the provided policies. Calls look as follows :
* 	cOpaque.makeOpaque(class, [("attr1",pol1), ("attr2",pol2), ...])
* where each attribute can occur at most once and each pol1, pol2, ... refers to
* a function with arity 0 that returns True or False.
**/
static PyObject * makeOpaque(PyObject *dummy, PyObject *args)
{
	
	PyObject * target;
	PyObject * publicAttrs;
	PyObject * privateAttrs;
	PyObject * defaultPolicy;

	// Checks on the provided arguments

	if (! PyArg_ParseTuple( args, "OO!O!O", &target, &PyList_Type, &publicAttrs,
                                    &PyList_Type, &privateAttrs, &defaultPolicy)) 
		return NULL;
	
	////
	// The target class
	////
	debug("DEBUG: Checking target class\n");
	
	if (!PyClass_Check(target)) // TODO: currently rejects:	class A(object):
	{
		(void)PyErr_Format(PyExc_RuntimeError, 
			"First argument should be a class");
		return NULL;
	}

	char * name = getObjectFullName(target);
	if (name == NULL)
	{
		(void)PyErr_Format(PyExc_RuntimeError, 
			"Could not derive name from class");
		return NULL;
	}
	
	if (knownTargets.find(target) != knownTargets.end())
	{
		(void)PyErr_Format(PyExc_RuntimeError, 
			"Opaque instance for %s can only be created once.", name);
		return NULL;
	}
		
	knownTargets.insert(target);
	Py_XINCREF(target);
	
	
	////
	// Public attributes
	////
	debug("DEBUG: Checking public attributes\n");
	
	int numLines = PyList_Size(publicAttrs);

	if (numLines < 0)
	{
		(void)PyErr_Format(PyExc_RuntimeError, "2nd argument is not a list");
		return NULL;
	}

	int i;
	set<char*> publicAttributes;
	for (i = 0; i < numLines; i++)
	{
		PyObject* item;

		if (! (item = PyList_GetItem(publicAttrs, i)))
			return NULL;
			
		if (!PyString_Check(item)) {
			(void)PyErr_Format(PyExc_RuntimeError, 
				"Provided attribute not a string");
			return NULL;
		}
			
		char * attr = PyString_AsString(item);
		
		debug("DEBUG: Public attribute: ");
		debug(attr);
		debug("\n");
			
		if (publicAttributes.find(attr) != publicAttributes.end())
		{
			(void)PyErr_Format(PyExc_RuntimeError, 
				"Attribute %s appears more than once.",attr);
			return NULL;
		}
		
		publicAttributes.insert(attr);
	}
	
	////
	// Private attributes
	////
	debug("DEBUG: Checking private attributes\n");
	
	numLines = PyList_Size(privateAttrs);

	if (numLines < 0)
	{
		(void)PyErr_Format(PyExc_RuntimeError, "3th argument is not a list");
		return NULL;
	}

	set<char*> privateAttributes;
	for (i = 0; i < numLines; i++)
	{
		PyObject* item;
		
		if (! (item = PyList_GetItem(privateAttrs, i)))
			return NULL;
			
		if (!PyString_Check(item)) {
			(void)PyErr_Format(PyExc_RuntimeError, 
				"Provided attribute not a string");
			return NULL;
		}
			
		char * attr = PyString_AsString(item);
		
		debug("DEBUG: Private attribute: ");
		debug(attr);
		debug("\n");
			
		if (publicAttributes.find(attr) != publicAttributes.end() ||
		    privateAttributes.find(attr) != privateAttributes.end())
		{
			(void)PyErr_Format(PyExc_RuntimeError, 
				"Attribute %s appears more than once.",attr);
			return NULL;
		}
		
		privateAttributes.insert(attr);
	}
	
	////
	// Default policy
	////
	bool defPol = PyObject_IsTrue(defaultPolicy) == 1;
	
	debug("DEBUG: Checking setting default policy to");
	if (defPol)
		debug(" true.\n");
	else
		debug(" false.\n");
	
	
	debug("DEBUG: Creating target class\n");	
	TargetClass * targetClass = new TargetClass(target, name, publicAttributes, 
	                   privateAttributes, defPol);
	PyObject * builder = encapBuilder_init(targetClass);
	
	Py_XINCREF(builder);
	PyObject * res =  PyObject_GetAttrString(builder,"build");
	Py_XINCREF(res);
	
	return res;

}

static PyObject * enableDebug(PyObject *dummy, PyObject *args)
{
	DEBUG = true;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * disableDebug(PyObject *dummy, PyObject *args)
{
	DEBUG = false;
	Py_INCREF(Py_None);
	return Py_None;
}


/**
* Only one method: makeOpaque.
**/
static PyMethodDef cOpaqueMethods[] = 
{
	{"makeOpaque",makeOpaque,METH_VARARGS,"Make a class opaque using the specified list of attribute/policy combinations. Returns a function that passes its arguments to the construction of an encapsulated object for the provided class. The attributes are accessible as according to the provided policies. Calls look as follows :\n class = cOpaque.makeOpaque(class, [(\"attr1\",pol1), (\"attr2\",pol2), ...])\n where each attribute can occur at most once and each pol1, pol2, ... refers to a function with arity 0 that returns True or False."} ,
	{"enableDebug",enableDebug,METH_VARARGS,"Enable the debugger"} ,
	{"disableDebug",disableDebug,METH_VARARGS,"Disable the debugger"} ,
	{NULL, NULL, 0, NULL} 
} ;

/**
* Initialization.
**/
PyMODINIT_FUNC initcOpaque(void)
{
	(void) Py_InitModule("cOpaque",cOpaqueMethods);
}
