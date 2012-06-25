// TODO
// - check for Py_XINCREF and place appropriate Py_XDECREF
// - check for possible segfaults


/*******************************************************************************

                        PyOpaque version 0.0.2


Apologies for not splitting this code into several files. Python's setuptools 
didn't let me. If anyone does, let me know:
    https://github.com/bvdelft/PyOpaque/issues/10

MIT License - http://www.opensource.org/licenses/MIT
2012 - Luciano Bello, Bart van Delft

******************************************************************************/


#include <Python.h>
#include <set>
using namespace std;


/*------------------------------------------------------------------------------

        Assisting methods
        
            - void debug(const char* format, ...)
            - PyObject * enableDebug(PyObject *dummy, PyObject *args)
            - PyObject * disableDebug(PyObject *dummy, PyObject *args)
            - bool argToCSet(PyObject*        list, 
                      set<char*, strPtrLess>* cset, 
                      const char *            name) 
        
------------------------------------------------------------------------------*/

// If set to -true-, debugging information is printed
// The value can be controlled from python by calling
//     cOpaque.enableDebug()
//     cOpaque.disableDebug()
bool DEBUG = false;


// Print text in debug-friendly format
static void debug(const char* format, ...)
{
    if (DEBUG)
    {
        printf("[PyOpaque DEBUG] ");
        va_list argptr;
        va_start(argptr, format);
        vprintf(format, argptr);
        va_end(argptr);
        printf("\n");
    }
}


// Enable debugging
static PyObject * enableDebug(PyObject *dummy, PyObject *args)
{
    DEBUG = true;
    Py_INCREF(Py_None);
    return Py_None;
}

// Disable debugging
static PyObject * disableDebug(PyObject *dummy, PyObject *args)
{
    DEBUG = false;
    Py_INCREF(Py_None);
    return Py_None;
}


// For performing searches in sets etc.
struct strPtrLess 
{
    bool operator( )(const char* p1,const char* p2) const 
    {
        return strcmp(p1,p2) < 0;
    }
};

// Convert a python object representing a list of strings to a set in C
static bool argToCSet(PyObject*               list, 
                      set<char*, strPtrLess>* cset, 
                      const char *            name) 
{

    int numLines = PyList_Size(list);
    if (numLines < 0)
    {
        (void)PyErr_Format(PyExc_RuntimeError, "Argument is not a list");
        return false;
    }

    for (int i = 0; i < numLines; i++)
    {
        PyObject* item;

        if (! (item = PyList_GetItem(list, i)))
            return NULL; // Error in the provided argument
            
        if (!PyString_Check(item)) {
            (void)PyErr_Format(PyExc_RuntimeError, 
                "Provided list-argument not a string");
            return false;
        }
            
        char * attr = PyString_AsString(item);
        
        debug("Adding to %s: %s", name, attr);
            
        if (cset->find(attr) != cset->end())
        {
            (void)PyErr_Format(PyExc_RuntimeError, 
                "List entry %s appears more than once.",attr);
            return false; // Does not have to be an error, but probably 
            // indicates incorrect use of the library
        }
        
        cset->insert(attr);
    }
    
    return true;
    
}

// define structures
#include "cOpaquemodule.h"

/*------------------------------------------------------------------------------

        Encapsulating the __import__ functionality
        
            - PyObject * encapImport(PyObject * me, PyObject *args)
            - PyObject * doImport(PyObject * me, PyObject *args)
        
------------------------------------------------------------------------------*/




// Pointer to the original import function
PyObject * BUILTIN_IMPORT;

// List of module names that cannot be imported (e.g. sys, gc)
set<char*, strPtrLess> ImportBlacklist;

// When set to -true-, allow for import of modules in the blacklist
bool UNSAFE_IMPORT = false;

// Store the import function and the blacklist
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
    

    debug("Adding blacklist\n");
    
    if (!argToCSet(blacklist, &ImportBlacklist, "blacklist"))
        return NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
        
}


// Call the import function on the module provided as argument. Only executes
// import if not blacklisted, or unsafe imports are allowed.
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
    
    if (BUILTIN_IMPORT == NULL)
    {
        if (UNSAFE_IMPORT)
            return PyImport_ImportModule(name);
        (void) PyErr_Format(PyExc_RuntimeError, 
                "__builtin__.__import__ not yet encapsulated");
        return NULL;
    }
    
    if (!UNSAFE_IMPORT && ImportBlacklist.count(name) > 0) 
    {
        (void) PyErr_Format(PyExc_RuntimeError, 
                "Illegal import (%s)",name);
        return NULL;
    }
    
    return PyObject_CallObject(BUILTIN_IMPORT, args);
    
}

/*------------------------------------------------------------------------------

        Encapsulating callable attributes
        
            - void encapsulatedAttribute_dealloc(PyObject* self)
            - PyObject * EAGetAttr(PyObject* self, char * attr) 
            - PyObject * EACall(PyObject * self, 
                                PyObject *args, 
                                PyObject *other)
            - PyTypeObject* makeEAType(char * name) 
            - PyObject* encapAttribute_init(PyObject* att, char * name) 
            
------------------------------------------------------------------------------*/


// Deallocation
static void encapsulatedAttribute_dealloc(PyObject* self)
{
    // PyObject_Del only to be called for objects allocated using
    // PyObject_New or PyObject_Newvar.
    Py_XDECREF(((EncapsulatedAttribute*) self)->attPointer);
    PyObject_Del(self);
}

// Do not return any attributes on a callable attribute (especially __self__ 
// would completely circumvent the protection)
static PyObject * EAGetAttr(PyObject* self, char * attr) 
{
    (void) PyErr_Format(PyExc_RuntimeError, 
        "This is an encapsulated callable attribute - can only be called.");
    return NULL;
}

// The only allowed operation is calling the attribute:
static PyObject * EACall(PyObject * self, PyObject *args, PyObject *other)
{
    return PyObject_CallObject(((EncapsulatedAttribute*)self)->attPointer, 
        args);
}

// Returns the type of the encapsulated attribute.
PyTypeObject* makeEAType(char * name) 
{

    debug("Generating encapsulated attribute %s", name);
    
    PyTypeObject * encapsulatedAttribute = 
        (PyTypeObject *)malloc(sizeof(PyTypeObject));

    memset(encapsulatedAttribute, 0, sizeof(PyTypeObject));
    PyTypeObject dummy = {PyObject_HEAD_INIT((PyTypeObject*)NULL)};
    memcpy(encapsulatedAttribute, &dummy, sizeof(PyObject));

    encapsulatedAttribute->tp_name = const_cast<char*>(name);
    encapsulatedAttribute->tp_basicsize = sizeof(EncapsulatedAttribute);

    encapsulatedAttribute->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                                Py_TPFLAGS_CHECKTYPES;
    encapsulatedAttribute->tp_doc = NULL;

    encapsulatedAttribute->tp_dealloc = encapsulatedAttribute_dealloc;

    encapsulatedAttribute->tp_getattr = EAGetAttr;
    encapsulatedAttribute->tp_methods = NULL;
    encapsulatedAttribute->tp_call = EACall;

    if(PyType_Ready(encapsulatedAttribute)<0)
        printf("Err: PyType_Ready: encapsulatedAttribute");

    return encapsulatedAttribute;

}

// Create a new EncapsulatedAttribute for the specified attribute
static PyObject* encapAttribute_init(PyObject* att, char * name) 
{

    if (att == NULL) 
        return att;    
        
    // Encapsulate iff it is callable and has attribute __self__ (i.e. 
    // preventing that we encapsulate callable objects)
    if (PyObject_HasAttrString(att,"__self__") && PyCallable_Check(att)) {
        EncapsulatedAttribute* encapAttr;
        encapAttr = PyObject_NEW(EncapsulatedAttribute, 
                                      makeEAType(name));
        encapAttr->attPointer = att;
        Py_XINCREF(att);
        return (PyObject*) encapAttr;
    }
    
    return att;
}

/*------------------------------------------------------------------------------

        Encapsulating objects
        
            - PyObject * EOGetAttr(PyObject * _eobject, char * attr)
            - PyObject * EOCall(PyObject * self, 
                                PyObject * args, 
                                PyObject * other)
            
------------------------------------------------------------------------------*/


// Return (possibly encapsulated) attributes only if the policy allows so.
static PyObject * EOGetAttr(PyObject * _eobject, char * attr)
{
    debug("Requested access to attribute %s", attr);

    EncapsulatedObject * eobject = (EncapsulatedObject *) _eobject;

    set<char*,strPtrLess> publicAttributes = eobject->target->publicAttributes;

    debug("Checking if attribute is in public list");

    // Public attribute, flow allowed:
    if (publicAttributes.count(attr) > 0) 
    { 
        PyObject * a =  PyObject_GetAttrString(eobject->objPointer,attr);
        PyObject * res = encapAttribute_init(a, attr);
        Py_XINCREF(res);
        return res;
    }
    
    debug("No");
    debug("Checking if attribute is in private list");
    
    set<char*,strPtrLess> privateAttributes = 
        eobject->target->privateAttributes;

    // Private attribute, flow not allowed:
    if (privateAttributes.count(attr) > 0) 
    {
        (void) PyErr_Format(PyExc_RuntimeError, 
                "The policy of attribute '%s' of '\%s' object disallows access",
                attr, eobject->target->name);
        return NULL;
    }
    

    debug("No");
    debug("Default public is %s", 
        eobject->target->defaultPublic ? "public" : "private");
    
    
    if (eobject->target->defaultPublic) {
    
        PyObject * a =  PyObject_GetAttrString(eobject->objPointer,attr);
        PyObject * res = encapAttribute_init(a, attr);
        Py_XINCREF(res);
        return res;
    
    } else {
    
        (void) PyErr_Format(PyExc_RuntimeError, 
                "The policy of attribute '%s' of '\%s' object disallows access",
                attr, eobject->target->name);
        return NULL;
        
    }
    
}

// If the wrapped object itself is callable, forward call.
static PyObject * EOCall(PyObject * self, PyObject *args, PyObject *other)
{
    PyObject * cmethod = EOGetAttr(self, (char *) "__call__");
    if (cmethod == NULL)
        return NULL;
        
    return PyObject_CallObject(cmethod, args);
}

/*------------------------------------------------------------------------------

        Constructor for the wrapper around the object to opaque.
            
------------------------------------------------------------------------------*/

/// Constructor. Stores public/private/default policy for this object.
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
}

/*------------------------------------------------------------------------------

        Opaque Objects
            
            - void eEncapsulatedType_dealloc(PyObject* self)
            - PyObject * newEOB(PyTypeObject * type, 
                                PyObject * args, 
                                PyObject * kargs) 
            
------------------------------------------------------------------------------*/

// Deallocates encapsulated objects
static void eEncapsulatedType_dealloc(PyObject* self)
{
    Py_XDECREF(((EncapsulatedObject*) self)->objPointer);
    PyObject_Del(self);
}

// Create a new instance of an encapsulating object
PyObject * newEOB(PyTypeObject * type, PyObject * args, PyObject *kargs) 
{
    
    debug("Generating new opaque object");
    
    EncapsulatedType* etype = (EncapsulatedType*)(type);
    
    // The new instance might be a child of the actual object to opaque, go up
    // until we find it.
    while (etype != NULL && etype->target == NULL) {
        
        debug("No opaque object found in hierarchy");
        etype = (EncapsulatedType*) 
                    PyObject_GetAttrString((PyObject*)etype, "__base__");
    }
    
    if (etype == NULL)
        return NULL;
    
    debug("Creating object of class %s ", etype->target->name);
    // Create the object to be encapsulated
    PyObject* theObject; 
    theObject = PyObject_CallObject(etype->target->target, args); 
    if (theObject == NULL)
        return NULL;
    
    debug("Creating encapsulating (opaque) object");    
    // Create an encapsulating object
    EncapsulatedObject * encap = (EncapsulatedObject *) type->tp_alloc(type, 0);
    
    encap->target = etype->target;
    
    debug("Linking opaque object to real object");
    // Store the object in the encapsulating one
    encap->objPointer = theObject;
    Py_XINCREF(theObject);

    
    debug("Returning opaque object");
    // Return the result
    PyObject * res = (PyObject*) encap;
    Py_XINCREF(res);
    
    return res;
}

// Simple wrapper to take advantage of simpler EOGetAttr signature in rest of
// the code.
PyObject * EOGetAttrS(PyObject * obj, PyObject *args) {
    char * attr;
    if (! PyArg_ParseTuple( args, "s", &attr)) 
        return NULL;
    return EOGetAttr(obj,attr);
}

static PyMethodDef EOMethods[] = 
{
    {"__getattr__",EOGetAttrS,METH_VARARGS,NULL} ,
    {NULL, NULL, 0, NULL} 
} ;

static PyObject * EORepr(PyObject * obj)
{
    return PyObject_Repr(((EncapsulatedObject*)obj)->objPointer);
}

static PyObject * EOStr(PyObject * obj)
{
    return PyObject_Str(((EncapsulatedObject*)obj)->objPointer);
}


/**
* Constructs the EncapsulatedType. 
**/
EncapsulatedType* makeEncapsulatedType(char * name, bool calb) 
{
    EncapsulatedType * encapsulatedType = 
        (EncapsulatedType *)malloc(sizeof(EncapsulatedType));

    memset(encapsulatedType, 0, sizeof(EncapsulatedType));
    PyTypeObject dummy = {PyObject_HEAD_INIT((PyTypeObject*)NULL)};
    memcpy(encapsulatedType, &dummy, sizeof(PyObject));

    // Set the type-instance specific name
    debug("Name: ");
    debug(name);
    debug("\n");

    encapsulatedType->tp_name = const_cast<char*>(name);
    encapsulatedType->tp_getattr = EOGetAttr;
    
    encapsulatedType->tp_basicsize = sizeof(EncapsulatedType);

    encapsulatedType->tp_flags = Py_TPFLAGS_DEFAULT    | 
                                 Py_TPFLAGS_BASETYPE   |
                                 Py_TPFLAGS_CHECKTYPES ;
    encapsulatedType->tp_doc = NULL;
    encapsulatedType->tp_repr = EORepr;
    encapsulatedType->tp_str = EOStr;
    
    if(calb) {
        debug(name); debug(" is callable!!\n");
        encapsulatedType->tp_call = EOCall;
    }

    encapsulatedType->tp_dealloc = eEncapsulatedType_dealloc;

    encapsulatedType->tp_methods = EOMethods;
    encapsulatedType->tp_new = newEOB;

    if(PyType_Ready(encapsulatedType)<0)
        printf("> Error: PyType_Ready: %s", name);
    else
        debug("PyType Ready\n");

    return encapsulatedType;

}

/**
* Creates a new EncapsulatedType for the specified target class.
**/                             
static PyObject * encapType_init(PyObject * module, char * name, bool calb, TargetClass* target) 
{
    EncapsulatedType* myEncapsulatedType = makeEncapsulatedType(name, calb);
    myEncapsulatedType->target = target;
    Py_XINCREF(module);
    Py_XINCREF(myEncapsulatedType);
    PyModule_AddObject(module, name,(PyObject*) myEncapsulatedType);
    Py_XINCREF((PyObject*) myEncapsulatedType);
    return (PyObject*) myEncapsulatedType;
}

/*----------------------------------------------------------------------------*/
////// cOpaque /////////////////////////////////////////////////////////////////

// Helper function, returns name
static char * getObjectName(PyObject * obj) {
    
    if (! PyObject_HasAttrString(obj, "__name__"))
        return NULL;
    PyObject * nameattr = PyObject_GetAttrString(obj, "__name__");
    if (! PyString_Check(nameattr)) 
        return NULL;
    char * name = PyString_AsString(nameattr);
    
    return name;
}

static bool isCallable(PyObject * obj) {
    
    return PyObject_HasAttrString(obj, "__call__"); // ||
           //PyCallable_Check(obj);
           
}

// Helper function, returns pointer to module
static PyObject * getObjectModule(PyObject * obj) {

    debug("Getting object module\n");
    
    if (! PyObject_HasAttrString(obj, "__module__"))
        return NULL;
    PyObject * moduleattr = PyObject_GetAttrString(obj, "__module__");
    UNSAFE_IMPORT = true;
    PyObject * modules = PyObject_GetAttrString(PyImport_ImportModule("sys"), "modules");
    UNSAFE_IMPORT = false;
    PyObject * args = PyTuple_New(1);
    Py_XINCREF(moduleattr);
    PyTuple_SetItem(args, 0, moduleattr);

    debug("Returning object module\n");
    
    return PyObject_Call(PyObject_GetAttrString(modules, "get"), args, NULL);
}

/**
* List of classes that have been made opaque - a class can only be made opaque
* once, otherwise we run into some problems. -- still required?
**/
static set<PyObject*> knownTargets;

/**
* Creates a new type for the provided class, with list of public and private 
* attributes.
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
    
    if (!PyClass_Check(target) && !PyType_Check(target))
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
    

    set<char*, strPtrLess> publicAttributes;
    
    if (!argToCSet(publicAttrs, &publicAttributes,"public"))
        return NULL;

    if (PyCallable_Check(target))
        publicAttributes.insert((char *)"__call__");
    
    ////
    // Private attributes
    ////
    debug("DEBUG: Checking private attributes\n");
    
    set<char*, strPtrLess> privateAttributes;
    
    if (!argToCSet(privateAttrs, &privateAttributes,"private"))
        return NULL;
    
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
                       
    debug("DEBUG: Created\n");              
                       
    return encapType_init(getObjectModule(target), name, isCallable(target), targetClass);
}



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
