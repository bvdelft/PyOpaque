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

A=opaquer(A,'secret')
B=opaque([ 'secret' ], [ 'public' ] , True )(B)

class C(B):
	pass

def assertRuntimeError(self,o,secret):
     with self.assertRaises(RuntimeError) as cm:
        getattr(o, secret)

unittest.TestCase.assertRaiseRuntimeError=assertRuntimeError

class TestSimpleAccess(unittest.TestCase):
    def test_opaquer(self):
        a=A(2)
        self.assertEqual(a.secret,2)
        self.assertRaiseRuntimeError(a,'public')

    def test_opaquerAttrObject(self):
        c=A(2)
        a=A(c)
        self.assertEqual(a.secret.secret,2)
        self.assertRaiseRuntimeError(a.secret,'public')
        self.assertRaiseRuntimeError(a,'public')

    def test_opaquerAttrFunc(self):
        a=A(lambda: True)
        self.assertTrue(a.secret())
        self.assertRaiseRuntimeError(a,'public')
        self.assertRaiseRuntimeError(a,'thisFieldDoesNotExist')

class TestDeny_insecure_attributes(unittest.TestCase):
    def test_ifdefaultisFalse(self):
        b=B(2)
        self.assertRaiseRuntimeError(b,'__dict__')
    def test_ifdefaultisTrue(self):
        a=A(2)
        self.assertRaiseRuntimeError(a,'__dict__')

class TestSubtypeObject(unittest.TestCase):
    def test_opaquer(self):
        b=B(2)
        self.assertEqual(b.secret,2)
        self.assertRaiseRuntimeError(b,'public')

class TestExtendingClassSupport(unittest.TestCase):
    def test_extendAndAccessClassAttributes(self):
        C.newsecret=3
        self.assertEqual(C.newsecret,3)

    def test_extend(self):
        C.newsecret=3
        c=C(2)
        self.assertEqual(c.newsecret,3)
        self.assertEqual(c.secret,2)
        self.assertRaiseRuntimeError(c,'public')

class TestSubtypeObjectInstance(unittest.TestCase):
    def test_opaquer(self):
        c=C(2)
        self.assertEqual(c.secret,2)
        self.assertRaiseRuntimeError(c,'public')

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

if __name__ == '__main__':
    unittest.main()
