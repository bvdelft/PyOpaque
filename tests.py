#!/usr/bin/python
# -*- coding: utf-8 -*-
import unittest
import os
# os.system('cd opaque; make')
from opaque import opaque

class A():
    def __init__(self,q):
        self.field = q
        self.data = q
        
class B(object):
    def __init__(self,q):
        self.field = q
        self.data = q
       
def opaquer(aClassObject,aField):
    return opaque([ aField ], [] , False )(aClassObject)

A=opaquer(A,'field')
B=opaquer(B,'field')

class C(A):
	pass

def assertRuntimeError(self,o,field):
     with self.assertRaises(RuntimeError) as cm:
        getattr(o, field)

unittest.TestCase.assertRaiseRuntimeError=assertRuntimeError

class TestSimpleAccess(unittest.TestCase):
    def test_opaquer(self):
        a=A(2)
        self.assertEqual(a.field,2)
        self.assertRaiseRuntimeError(a,'data')

    def test_opaquerAttrObject(self):
        c=A(2)
        a=A(c)
        self.assertEqual(a.field.field,2)
        self.assertRaiseRuntimeError(a.field,'data')
        self.assertRaiseRuntimeError(a,'data')

    def test_opaquerAttrFunc(self):
        a=A(lambda: True)
        self.assertTrue(a.field())
        self.assertRaiseRuntimeError(a,'data')
        self.assertRaiseRuntimeError(a,'thisFieldDoesNotExist')
        
class TestSubtypeObject(unittest.TestCase):
    def test_opaquer(self):
        b=B(2)
        self.assertEqual(b.field,2)
        self.assertRaiseRuntimeError(b,'data')

class TestSubtypeObjectInstance(unittest.TestCase):
    def test_opaquer(self):
        c=C(2)
        self.assertEqual(c.field,2)
        self.assertRaiseRuntimeError(c,'data')


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
