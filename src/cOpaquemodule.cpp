#include <Python.h>
#include <set>
#include <map>
using namespace std;
#include "cOpaquemodule.h"

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
* __getattr__ for EncapsulatedObject. Looks in the attribute map if the 
* requested has an associated policy. If not, the attribute is assumed not to
* exist. If there is a mapping to a policy, it calls the policy function. If the
* policy function returns true the __getattr__ of the encapsulated object is
* called with the same argument. If false it returns the message that the 
* attribute is not accessible with the current policy. If the policy function
* yields an error this is reported as a RuntimeError.
**/
static PyObject * EOGetAttr(PyObject * _eobject, char * attr)
{
	EncapsulatedObject * eobject = (EncapsulatedObject *) _eobject;
	
	// Find the attribute's policy
	map<char*,PyObject*> attributes = eobject->target->attributes;
	map<char*,PyObject*>::iterator it = attributes.find(attr);
	if (it == attributes.end())
	{
		(void) PyErr_Format(PyExc_AttributeError, 
                "'\%s' object has no attribute '\%s'",
                eobject->target->name, attr);
		return NULL;
	}
	
	// Call the attribute's policy
	PyObject* policy = it->second;
	PyObject* result = PyObject_CallObject(policy, NULL);
	if (result == NULL)
	{
		(void) PyErr_Format(PyExc_RuntimeError, 
                "Failed in calling policy for attribute '%s' of '\%s' object",
                attr, eobject->target->name);
		return NULL;
	}
	
	// Apply the policy's result
	int isTrue = PyObject_IsTrue(result);
	if (isTrue == 1) // is true, allowed to get attribute
	{
		PyObject * res =  PyObject_GetAttrString(eobject->objPointer,attr);
		Py_XINCREF(res);
		return res;
	}
	if (isTrue == 0)  // is not true, not allowed to get attribute
	{
		(void) PyErr_Format(PyExc_RuntimeError, 
                "The policy of attribute '%s' of '\%s' object disallows access",
                attr, eobject->target->name);
		return NULL;
	}
	// error
	(void) PyErr_Format(PyExc_RuntimeError, 
            "Failure in executing policy for '%s' of '\%s' object",
            attr, eobject->target->name);
	return NULL;
	
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
	encapObjectType->tp_name = const_cast<char*>(name);

	// TODO could set type-instance specific attributes here?

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
                         map<char*,PyObject*> _attributes) 
{
	target = _target;
	name = _name;
	attributes = _attributes;
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
	Py_XINCREF(theObject);

	// Create an encapsulating object
	EncapsulatedObject * encap =
		(EncapsulatedObject *) PyObject_New(EncapsulatedObject, 
											ob->target->encapType);
	encap->target = ob->target;
	Py_XINCREF(ob->target->target);

	// Store the object in the encapsulating one
	encap->objPointer = theObject;

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
	Py_XINCREF(target->target);
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
	
	PyObject *target;
	PyObject * listObj;

	// Checks on the provided arguments

	if (! PyArg_ParseTuple( args, "OO!", &target, &PyList_Type, &listObj)) 
		return NULL;
		
	// TODO: currently rejects:		class A(object): 
	if (!PyClass_Check(target))
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

	int numLines = PyList_Size(listObj);

	if (numLines < 0)
	{
		(void)PyErr_Format(PyExc_RuntimeError, "Argument is not a list");
		return NULL;
	}

	int i;
	map<char*,PyObject*> parsedAttrs;
	for (i = 0; i < numLines; i++)
	{
		PyObject* item;
		
		if (! (item = PyList_GetItem(listObj, i)))
			return NULL;
			
		char * attr;
		PyObject * callable;
		
		if (! PyArg_ParseTuple(item, "sO", &attr, &callable)) 
			return NULL;
			
		if (! PyCallable_Check(callable))
		{
			(void)PyErr_Format(PyExc_RuntimeError, 
				"Not a callable element for attribute %s", attr);
			return NULL;
		}
		
		if (parsedAttrs.find(attr) != parsedAttrs.end())
		{
			(void)PyErr_Format(PyExc_RuntimeError, 
				"Attribute %s is given more than one policy.",attr);
			return NULL;
		}
		
		parsedAttrs.insert(pair<char*,PyObject*>(attr,callable));
	}
	
	// All checks passed, mapping of attributes to policy functions created.
	// Construct builder function and return
	
	TargetClass * targetClass = new TargetClass(target, name, parsedAttrs);
	PyObject * builder = encapBuilder_init(targetClass);
	
	PyObject * res =  PyObject_GetAttrString(builder,"build");
	Py_XINCREF(res);
	
	return res;

}

/**
* Only one method: makeOpaque.
**/
static PyMethodDef cOpaqueMethods[] = 
{
	{"makeOpaque",makeOpaque,METH_VARARGS,"Make a class opaque using the specified list of attribute/policy combinations. Returns a function that passes its arguments to the construction of an encapsulated object for the provided class. The attributes are accessible as according to the provided policies. Calls look as follows :\n class = cOpaque.makeOpaque(class, [(\"attr1\",pol1), (\"attr2\",pol2), ...])\n where each attribute can occur at most once and each pol1, pol2, ... refers to a function with arity 0 that returns True or False."} ,
	{NULL, NULL, 0, NULL} 
} ;

/**
* Initialization.
**/
PyMODINIT_FUNC initcOpaque(void)
{
	(void) Py_InitModule("cOpaque",cOpaqueMethods);
}
