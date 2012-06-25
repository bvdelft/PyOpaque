#!/usr/bin/python
# -*- coding: utf-8 -*-
import unittest
from opaque import opaque

class A():
    def __init__(self,q):
        self.secret = q
        self.public = q

class B(object):
    def __init__(self,q):
        self.secret = q
        self.public = q

def opaquer(aClassObject,aField):
    return opaque([ aField ], [] , False )(aClassObject)

A=opaquer(A,'public')
B=opaque([ 'public' ], [ 'secret' ] , True )(B)


class C(B):
	pass

class D(object):
	def __init__(self,q):
		self.secret=q
		self.public=q
	def __call__(self):
		return self.secret

D=opaque([ 'public' ], [ 'secret' ] , True )(D)

class E(object):
	def __init__(self,q):
		self.secret=q
		self.public=q
	def __call__(self):
		return self.secret

def assertRuntimeError(self,o,secret):
     with self.assertRaises(RuntimeError) as cm:
        getattr(o, secret)

unittest.TestCase.assertRaiseRuntimeError=assertRuntimeError

class TestSimpleAccess(unittest.TestCase):
    def test_opaquer(self):
        a=A(2)
        self.assertEqual(a.public,2)
        self.assertRaiseRuntimeError(a,'secret')

    def test_opaquerAttrObject(self):
        c=A(2)
        a=A(c)
        self.assertEqual(a.public.public,2)
        self.assertRaiseRuntimeError(a.public,'secret')
        self.assertRaiseRuntimeError(a,'secret')

    def test_opaquerAttrFunc(self):
        a=A(lambda: True)
        self.assertTrue(a.public())
        self.assertRaiseRuntimeError(a,'secret')
        self.assertRaiseRuntimeError(a,'thisFieldDoesNotExist')

class TestCallableIssues(unittest.TestCase):
    def test_instanceCallable(self):
        a=A(1)
        d=D(1)
        e=E(1)
        self.assertFalse(callable(a))
        self.assertTrue(callable(d))
        self.assertTrue(callable(e))

    def test_nestedInstance1(self):
        d=D(A)
        e=E(A)
        self.assertTrue(callable(d.public))
        self.assertTrue(callable(e.public))
        
    def test_nestedInstance2(self):
        e=E(3)
        d=D(e)
        self.assertTrue(callable(d.public))
        self.assertEqual(d.public(), 3)
        self.assertEqual(d.public.public, 3)
    
    def test_nestedInstance3(self):
        d=D(3)
        e=E(d)        
        self.assertTrue(callable(e.public))
        self.assertEqual(e.public(), 3)
        self.assertEqual(e.public.public, 3)
        # Should not give segfault :)
        self.assertFalse(hasattr(e.public,'thisFieldDoesNotExist'))
    
    def test_nestedInstance4(self):
        d=D(3)
        e=E(d)    
        # .__self__ points back to e.public, which is already an opaque object.
        # Hence .__self__ can be accessed (not dangerous), but not the field
        # secret.
        self.assertRaiseRuntimeError(e.public.__call__.__self__,'secret')
        
class TestDeny_insecure_attributes(unittest.TestCase):
    def test_ifdefaultisFalse_withextension(self):
        b=B(2)
        self.assertTrue(hasattr(b,'__dict__')) # because has allow_extension
        with self.assertRaises(RuntimeError) as cm:
	   b.__class__.__bases__[0].__getattr__(b,'secret')
    def test_ifdefaultisTrue_withextension(self):
        a=A(2)
        self.assertTrue(hasattr(a,'__dict__')) # because has allow_extension
        with self.assertRaises(RuntimeError) as cm:
	   a.__class__.__bases__[0].__getattr__(a,'secret')

class TestSubtypeObject(unittest.TestCase):
    def test_opaquer(self):
        c=C(2)
        self.assertEqual(c.public,2)
        self.assertRaiseRuntimeError(c,'secret')

class TestExtendingClassSupport(unittest.TestCase):
    def test_extendAndAccessClassAttributes(self):
        C.newsecret=3
        self.assertEqual(C.newsecret,3)

    def test_extend(self):
        C.newsecret=3
        c=C(2)
        self.assertEqual(c.newsecret,3)
        self.assertEqual(c.public,2)
        self.assertRaiseRuntimeError(c,'secret')

class TestSubtypeObjectInstance(unittest.TestCase):
    def test_opaquer(self):
        c=C(2)
        self.assertEqual(c.public,2)
        self.assertRaiseRuntimeError(c,'secret')

from opaque import applyPolicy
import io

class account():
     def __init__(self,owner="Bart",balance=0):
          self.owner=owner
          self.balance=balance
          self.history=[]

     def withdraw(self,amount=0):
          self.history.append(('withdraw',amount))
          self.balance-=amount

     def deposit(self,amount=0):
          self.history.append(('deposit',amount))
          self.balance+=amount

class TestConfig1(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        sample_config = """
[account]
default-public
balance =private
history =private
owner   = public
withdraw=public
deposit =public
"""
        self.account1=applyPolicy(account,io.BytesIO(sample_config))
        self.instance=self.account1()

    def testDeny(self):
        self.assertRaiseRuntimeError(self.instance,'balance')
        self.assertRaiseRuntimeError(self.instance,'history')

    def testAllow(self):
        self.assertEqual(self.instance.owner,'Bart')
        self.assertTrue(callable(self.instance.withdraw))
        self.assertTrue(callable(self.instance.deposit))

    def testExtendClass(self):
       self.account1.extraClass='extra'

if __name__ == '__main__':
    unittest.main()
