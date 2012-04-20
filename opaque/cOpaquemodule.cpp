#include <Python.h>
#include <set>
using namespace std;

struct strPtrLess {
   bool operator( )(const char* p1,const char* p2) const {
      return strcmp(p1,p2) < 0;
   }
};

#include "cOpaquemodule.h"

bool DEBUG = false;

static void debug(const char * text) {
	if (DEBUG)
		printf("%s",text);
}

/*----------------------------------------------------------------------------*/
////// Encapsulating __builtin__.__import__ ////////////////////////////////////

PyObject * BUILTIN_IMPORT;

set<char*, strPtrLess> ImportBlacklist;

PyObject * encapImport(PyObject * me, PyObject *args)
{
    if (BUILTIN_IMPORT != NULL)
    {
        (void) PyErr_Format(PyExc_RuntimeError, 
                "cPyOpaque already encapsulated __builtin__.__import__");
		return NULL;
    }

    PyObject * blacklist;

	if (! PyArg_ParseTuple( args, "OO!", &BUILTIN_IMPORT, &PyList_Type, 
	  &blacklist))
		return NULL;
	

	debug("DEBUG: Adding blacklist\n");
	
	int numLines = PyList_Size(blacklist);

	if (numLines < 0)
	{
		(void)PyErr_Format(PyExc_RuntimeError, "2nd argument is not a list");
		return NULL;
	}

	int i;
	for (i = 0; i < numLines; i++)
	{
		PyObject* item;

		if (! (item = PyList_GetItem(blacklist, i)))
			return NULL;
			
		if (!PyString_Check(item)) {
			(void)PyErr_Format(PyExc_RuntimeError, 
				"Provided attribute not a string");
			return NULL;
		}
			
		char * attr = PyString_AsString(item);
		
		debug("DEBUG: Blacklisting: ");
		debug(attr);
		debug("\n");
			
		if (ImportBlacklist.find(attr) != ImportBlacklist.end())
		{
			(void)PyErr_Format(PyExc_RuntimeError, 
				"Blacklist %s appears more than once.",attr);
			return NULL;
		}
		
		ImportBlacklist.insert(attr);
	}
	
	Py_INCREF(Py_None);
    return Py_None;
		
}

PyObject * doImport(PyObject * me, PyObject *args)
{
    char * name;
    PyObject * globals;
    PyObject * locals;
    PyObject * fromlist;
    int level;
    
    if (! PyArg_ParseTuple(args, "s|OOOi", &name, &globals, &locals, &fromlist, 
        &level))     
        return NULL;
    
    if (ImportBlacklist.count(name) > 0) 
	{
        (void) PyErr_Format(PyExc_RuntimeError, 
                "Illegal import (%s)",name);
		return NULL;
    }
    
    if (BUILTIN_IMPORT == NULL)
    {
        (void) PyErr_Format(PyExc_RuntimeError, 
                "__builtin__.__import__ not yet encapsulated");
		return NULL;
    }
    
    return PyObject_CallObject(BUILTIN_IMPORT, args);
    
}


/*----------------------------------------------------------------------------*/
////// EncapsulatedAttribute ///////////////////////////////////////////////////

/**
* Deallocator for encapsulatedAttribute
**/
static void encapsulatedAttribute_dealloc(PyObject* self)
{
	PyObject_Del(((EncapsulatedAttribute*) self)->attPointer);
	PyObject_Del(self);
}


static PyObject * EAGetAttr(PyObject* self, char * attr) {
//	return PyObject_GetAttrString(((EncapsulatedAttribute*)self)->attPointer,attr);
	return NULL;
}

static PyObject * EACall(PyObject * self, PyObject *args, PyObject *other)
{
    return PyObject_CallObject(((EncapsulatedAttribute*)self)->attPointer, args);
}


static PyMethodDef EAMethods[] = 
{
  {NULL, NULL, 0, NULL} 
} ;

// TODO change tp_name
PyTypeObject* makeEncapsulatedAttribute() 
{
	PyTypeObject * encapsulatedAttribute = 
		(PyTypeObject *)malloc(sizeof(PyTypeObject));

	memset(encapsulatedAttribute, 0, sizeof(PyTypeObject));
	PyTypeObject dummy = {PyObject_HEAD_INIT((PyTypeObject*)NULL)};
	memcpy(encapsulatedAttribute, &dummy, sizeof(PyObject));

	encapsulatedAttribute->tp_name = const_cast<char*>("EncapsulatedAttribute");
	encapsulatedAttribute->tp_basicsize = sizeof(EncapsulatedAttribute);

	// TODO Not really sure what line below does.
	encapsulatedAttribute->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
								Py_TPFLAGS_CHECKTYPES;
	encapsulatedAttribute->tp_doc = NULL;

	encapsulatedAttribute->tp_dealloc = encapsulatedAttribute_dealloc;

	encapsulatedAttribute->tp_getattr = EAGetAttr;
	encapsulatedAttribute->tp_methods = EAMethods;
	encapsulatedAttribute->tp_call = EACall;

	if(PyType_Ready(encapsulatedAttribute)<0)
		printf("Err: PyType_Ready: encapsulatedAttribute");

	return encapsulatedAttribute;

}

/**
* There can be only one (EncapsulatedAttributeType).
**/
static PyTypeObject* EncapsulatedAttributeType = makeEncapsulatedAttribute();

/**
* Creates a new EncapsulatedAttribute for the specified attribute
**/                             
static PyObject* encapAttribute_init(PyObject* att) 
{
	
	if (PyCallable_Check(att)) {
		EncapsulatedAttribute* encapAttr;
		encapAttr = PyObject_NEW(EncapsulatedAttribute, 
		                              EncapsulatedAttributeType);
		encapAttr->attPointer = att;
		Py_XINCREF(att);
		return (PyObject*) encapAttr;
	}
	return att;
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
    
	// TODO, important: should never allow access to
	// - __class__

	EncapsulatedObject * eobject = (EncapsulatedObject *) _eobject;
	

	set<char*, strPtrLess> publicAttributes = eobject->target->publicAttributes;

	debug(attr);

	debug("DEBUG: Checking for public attributes\n");

	if (publicAttributes.count(attr) > 0) 
	{ // Public attribute, flow allowed:
		PyObject * a =  PyObject_GetAttrString(eobject->objPointer,attr);
		PyObject * res = encapAttribute_init(a);
		Py_XINCREF(res);
		return res;
	}
	
	set<char*, strPtrLess> privateAttributes = eobject->target->privateAttributes;
	
	debug("DEBUG: Checking for private attributes\n");

	if (privateAttributes.count(attr) > 0) 
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
	
		PyObject * a =  PyObject_GetAttrString(eobject->objPointer,attr);
		PyObject * res = encapAttribute_init(a);
		Py_XINCREF(res);
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
	debug("Name: ");
	debug(name);
	debug("\n");
	encapObjectType->tp_name = const_cast<char*>(name);

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
                         set<char*, strPtrLess> _publicAttributes, 
                         set<char*, strPtrLess> _privateAttributes,
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
* The only method for EObjectBuilder is the constructor-mimicing function build.
**/
static PyMethodDef EOBMethods[] = 
{
  {NULL, NULL, 0, NULL} 
} ;

PyObject * newEOB(PyTypeObject * self, PyObject * args, PyObject *kargs) {

  
    EncapsulatedType *etype = (EncapsulatedType*)self;

	// Create the object to be encapsulated
	PyObject* theObject; 
	theObject = PyObject_CallObject(etype->target->target, args); 
	if (theObject == NULL)
		return NULL;
	

	// Create an encapsulating object
	EncapsulatedObject * encap =
		(EncapsulatedObject *) PyObject_New(EncapsulatedObject, 
										etype);
	
	encap->target = etype->target;
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
* Constructs the EObjectBuilderType. Easier to make this function than to use 
* the struct-approach.
**/
EncapsulatedType* makeEObjectBuilderType(char * name) 
{
	EncapsulatedType * eObjectBuilderType = 
		(EncapsulatedType *)malloc(sizeof(EncapsulatedType));

	memset(eObjectBuilderType, 0, sizeof(EncapsulatedType));
	PyTypeObject dummy = {PyObject_HEAD_INIT((PyTypeObject*)NULL)};
	memcpy(eObjectBuilderType, &dummy, sizeof(PyObject));

    // Set the type-instance specific name
	debug("Name: ");
	debug(name);
	debug("\n");

	eObjectBuilderType->tp_name = const_cast<char*>(name);
	eObjectBuilderType->tp_getattr = EOGetAttr;
	
	eObjectBuilderType->tp_basicsize = sizeof(EObjectBuilder);

	// TODO Not really sure what line below does.
	eObjectBuilderType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
								Py_TPFLAGS_CHECKTYPES;
	eObjectBuilderType->tp_doc = NULL;

	eObjectBuilderType->tp_dealloc = eObjectBuilder_dealloc;

	eObjectBuilderType->tp_methods = EOBMethods;
	eObjectBuilderType->tp_new = newEOB;

	if(PyType_Ready(eObjectBuilderType)<0)
		printf("> Error: PyType_Ready: %s", name);

	return eObjectBuilderType;

}

/**
* Creates a new EObjectBuilder for the specified target class.
**/                             
static PyObject * encapBuilder_init(PyObject * module, char * name, TargetClass* target) 
{
    EncapsulatedType* EObjectBuilderType = makeEObjectBuilderType(name);
	EObjectBuilderType->target = target;
	Py_XINCREF(module);
	Py_XINCREF(EObjectBuilderType);
    PyModule_AddObject(module, name,(PyObject*) EObjectBuilderType);
    Py_XINCREF((PyObject*) EObjectBuilderType);
    return (PyObject*) EObjectBuilderType;
}

/*----------------------------------------------------------------------------*/
////// cOpaque /////////////////////////////////////////////////////////////////


static char * getObjectName(PyObject * obj) {
	
	if (! PyObject_HasAttrString(obj, "__name__"))
		return NULL;
	PyObject * nameattr = PyObject_GetAttrString(obj, "__name__");
	if (! PyString_Check(nameattr)) 
		return NULL;
	char * name = PyString_AsString(nameattr);
	
	return name;
}


static PyObject * getObjectModule(PyObject * obj) {
	
	if (! PyObject_HasAttrString(obj, "__module__"))
		return NULL;
	PyObject * moduleattr = PyObject_GetAttrString(obj, "__module__");

	PyObject * modules = PyObject_GetAttrString(PyImport_ImportModule("sys"), "modules");
	PyObject * args = PyTuple_New(1);

	Py_XINCREF(moduleattr);
	PyTuple_SetItem(args, 0, moduleattr);

	
	return PyObject_Call(PyObject_GetAttrString(modules, "get"), args, NULL);
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

	char * name = getObjectName(target);
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
	set<char*, strPtrLess> publicAttributes;
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

	set<char*, strPtrLess> privateAttributes;
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
	                   
	                   
	                   
	return encapBuilder_init(getObjectModule(target), name, targetClass);
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
	{"encapImport",encapImport,METH_VARARGS,"Encapsulate __builtin__.__import__"},
	{"doImport",doImport,METH_VARARGS,"Call the encapsulated __builtin__.__import__"},
	{NULL, NULL, 0, NULL} 
} ;

/**
* Initialization.
**/
PyMODINIT_FUNC initcOpaque(void)
{
   (void) Py_InitModule("cOpaque",cOpaqueMethods);
}
