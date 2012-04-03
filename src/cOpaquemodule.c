/*

Generic privatising class
Stuck to c code for compatibility.

*/

// Note, this must be the first included header
// Already includes stdio, string, errno and stdlib (.h)
#include <Python.h>

// List of names
struct nameList_el 
{
  char * name;
  struct nameList_el * next;
};
typedef struct nameList_el nameList;

// Describes information about a class to encapsulate
typedef struct
{
  PyObject* target;
  nameList* pubGetAttr;
  nameList* pubSetAttr;
  char * name;
  int finalized;
} targetClass;

// A list of target classes
struct targetClassList_el {
  targetClass * elem;
  struct targetClassList_el * next;
};
typedef struct targetClassList_el targetClassList;

// The python-type of the encapObject (defined later)
staticforward PyTypeObject encapType;

// Internal representation of the encapObject
typedef struct 
{
  PyObject_HEAD
  targetClass * targetC;  // To which targetclass this object belongs
  PyObject* objPointer; // Pointer to the encapsulated object
} encapObject;

// Creates a new encapsulated object
static PyObject* encap_init(targetClass * targetC, PyObject* args) 
{
  // Create a new encapObject
  encapObject* encap;
  encap = PyObject_NEW(encapObject, &encapType);
  
  // Parse args to constructor of target class
  PyObject* theObject; 
  theObject = PyObject_CallObject(targetC->target, args); 
  Py_XINCREF(theObject);
  
  // Set pointer in the encapsulated object
  encap->objPointer = theObject;
  encap->targetC = targetC;
  
  return (PyObject*) encap;
}

// Deallocation
static void encap_dealloc(PyObject* self)
{
  // Deallocate the encapsulated object
  encapObject * encap;
  encap = (encapObject*) self;
  //Py_XDECREF(encap->objPointer);
  PyObject_Del(encap->objPointer);
  
  // Deallocate self
  PyObject_Del(self);
}

// Returns direct pointer to the encapsulted object
// Should be REMOVED FROM FINAL VERSION!
static PyObject * encap_getObject(PyObject* self, PyObject *ignored) 
{
  encapObject * encap;
  encap = (encapObject*) self;
 // Py_XINCREF(encap->objPointer);
  return encap->objPointer;
}

// TODO: Remove
static PyMethodDef encapType_methods[] = {
  {"getObject",(PyCFunction)encap_getObject, METH_VARARGS,"Get the encapsulated object."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

// If the requested attribute is known to be accessible, call
// __getattr__ on the encapsulted object, otherwise return error
static PyObject * encap_getattr(encapObject *obj, char *name)
{
  // TODO: Remove
  // TODO: Add repr etc.
  if (strcmp(name,"getObject") == 0) {
    PyObject * res = Py_FindMethod(encapType_methods, (PyObject *)obj, name);
    Py_XINCREF(res);
    return res;
  }

  nameList * a = obj->targetC->pubGetAttr;  
  
  while (a) 
  {
    if (a->name && strcmp(name,a->name) == 0) 
    {
      PyObject * res =  PyObject_GetAttrString(obj->objPointer,name);
      Py_XINCREF(res);
      return res;
    }
    a = a->next;
  } 
  (void) PyErr_Format(PyExc_AttributeError, "'\%s' object has no attribute '\%s'",obj->targetC->name,name);
  return NULL;
}

// Similar as getAttr
static int encap_setattr(encapObject *obj, char *name, PyObject *v)
{

//      return PyObject_SetAttrString(obj->objPointer,name,v);
// TODO
  (void)PyErr_Format(PyExc_RuntimeError, "Read-only attribute: \%s", name);
  return -1;
}

// Definition of the type of encapsulated objects
static PyTypeObject encapType = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "Encapsulated", // TODO replace by function
  sizeof(encapObject),
  0,
  encap_dealloc, /*tp_dealloc*/
  0,          /*tp_print*/
  encap_getattr,          /*tp_getattr*/
  encap_setattr,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */                                                        
};

// EncapBuilder: for creating a constructor of encapsulated objects
// for a specific sort
staticforward PyTypeObject encapBuilderType;

// The EncapBuilder object
typedef struct
{
  PyObject_HEAD
  targetClass * target;
} encapBuilderObject;

// Standard memory deallocation
static void encapBuilder_dealloc(PyObject* self)
{
  PyObject_Del(self);
}

// The build function, to replace the original class for ob->target
// Passes along arguments to the constructor
static PyObject * build(PyObject* self, PyObject* args) {
  encapBuilderObject *ob = (encapBuilderObject*)self;
  PyObject * res = encap_init(ob->target, args);
  Py_XINCREF(res);
  return res;
}

// TODO REMOVE
static PyObject * getConstructor(PyObject* self, PyObject*args) 
{
  encapBuilderObject *ob = (encapBuilderObject*)self;
  return ob->target->target; 
}

// Builer has only one method:
static PyMethodDef encapBuilderType_methods[] = 
{
  {"build",(PyCFunction)build,METH_VARARGS,"Build"},
  {"getConstructor",(PyCFunction)getConstructor,METH_VARARGS,"REMOVE THIS"},
  {NULL,NULL,0,NULL}
};

// Find the only method
static PyObject * encapBuilder_getattr(encapBuilderObject *obj, char *name)
{
  return Py_FindMethod(encapBuilderType_methods, (PyObject *)obj, name);
}

// List of the registered classes
targetClassList * knownTargets = NULL;
                                
// Create a new builder for the provided class
static PyObject* encapBuilder_init(PyObject* ignore, PyObject* args) 
{
  PyObject *target;
  
  if (PyArg_ParseTuple(args, "O:registerTargetClass", &target)) 
  {
    targetClassList * k = knownTargets;
    while (k) 
    {
      if (k->elem->target == target) 
      {
        if (k->elem->finalized == 1) 
        {
          encapBuilderObject* encapB;
          encapB = PyObject_NEW(encapBuilderObject, &encapBuilderType);
          Py_XINCREF(encapB);
          Py_XINCREF(target);
          encapB->target = k->elem;
          return (PyObject*) encapB;
        } else 
        {
          (void)PyErr_Format(PyExc_RuntimeError, "Cannot create builder for unfinalized class");
          return NULL;
        }
      }      
      k = k->next;
    }
    (void)PyErr_Format(PyExc_RuntimeError, "Trying to create builder for unknown class");
    return NULL;
  }
  // Incorrect call
  return NULL;
}

// Type of the encap object builder
static PyTypeObject encapBuilderType =
{
  PyObject_HEAD_INIT(NULL)
  0,
  "EncapsulateBuilder",
  sizeof(encapBuilderObject),
  0,
  encapBuilder_dealloc, /*tp_dealloc*/
  0,          /*tp_print*/
  encapBuilder_getattr,          /*tp_getattr*/
  0, //encapBuilder_setattr,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */                                                        
};

// Add a new class to the list of targets
static PyObject * registerTargetClass(PyObject *dummy, PyObject *args)
{
  PyObject *result = NULL;
  PyObject *target;
  
  if (PyArg_ParseTuple(args, "O:registerTargetClass", &target)) 
  {
    // Todo: check if it is a class

    targetClassList * k = knownTargets;
    while (k) 
    {
      if (k->elem->target == target) 
      {
        (void)PyErr_Format(PyExc_RuntimeError, "Trying to register a class two times");
        return NULL;
      }
    }
    
    // Py_XINCREF(target);
    
    targetClass * newTarget = (targetClass *)malloc(sizeof(targetClass));
    newTarget->target = target;
    newTarget->pubGetAttr = NULL;
    newTarget->pubSetAttr = NULL;
    newTarget->finalized = 0;
    
    targetClassList * newEl = (targetClassList *)malloc(sizeof(targetClassList));
    newEl->elem = newTarget;
    newEl->next = knownTargets;
    
    knownTargets = newEl;
    
    Py_INCREF(Py_None);
    result = Py_None;
  }
  return result;
}

// Add an attribute as one of the available attributes for a 
// target class - exportGetAttr(target,"attr")
static PyObject * exportGetAttr(PyObject *dummy, PyObject *args)
{
  PyObject *target;
  char *attr;
  
  if (PyArg_ParseTuple(args, "Os:registerTargetClass", &target,&attr)) 
  {
    targetClassList * k = knownTargets;
    while(k) {
      if (k->elem->target == target) 
      {
        if (k->elem->finalized == 1)
        {
          (void)PyErr_Format(PyExc_RuntimeError, "Finalized class.");
          return NULL;
        }
        
        nameList * newEl = (nameList *)malloc(sizeof(nameList));
        newEl->name = attr;
        newEl->next = k->elem->pubGetAttr;
        k->elem->pubGetAttr = newEl;
        
        Py_INCREF(Py_None);
        return Py_None;
      }
      k = k->next;
    }
  }
  (void)PyErr_Format(PyExc_RuntimeError, "Class not registered");
  return NULL;
}

// Sets boolean to finalized for provided class
static PyObject * finalizeTargetClass(PyObject *dummy, PyObject *args)
{
  PyObject *target;
  char * name;
  
  if (PyArg_ParseTuple(args, "Os:finalizeTargetClass", &target, &name)) 
  {
    targetClassList * k = knownTargets;
    while(k) {
      if (k->elem->target == target) 
      {
        k->elem->finalized = 1;
        k->elem->name = name;
        Py_INCREF(Py_None);
        return Py_None;
      }
      k = k->next;
    }
  }
  return NULL;
}

// Exported methods
static PyMethodDef cOpaqueMethods[] = 
{
  {"registerTargetClass",registerTargetClass,METH_VARARGS,"Register a new class to encapsulate"} ,
  {"builder",encapBuilder_init,METH_VARARGS,"The builder"},
  {"exportGetAttr",exportGetAttr,METH_VARARGS,"Export an attribute to get"},
  {"finalizeTargetClass",finalizeTargetClass,METH_VARARGS,"Ensure that the visibility of this class is not modified"} ,
  {NULL, NULL, 0, NULL} 
} ;

// Initialize module
PyMODINIT_FUNC initcOpaque(void)
{
  encapType.ob_type = &PyType_Type;
  encapBuilderType.ob_type = &PyType_Type;
  (void) Py_InitModule("cOpaque",cOpaqueMethods);
}
