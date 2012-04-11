# -*- coding: utf-8 -*-
import unittest
import os
os.system('cd src; make')
import sys
sys.path.insert(0, 'src/')
import cOpaque

class A():
    def __init__(self,q):
        self.field = q
        self.data = q

def opaquer(aClassObject,aField):
    return cOpaque.makeOpaque(aClassObject, [ aField ], [] , False )

A=opaquer(A,'field')

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

from policies.opaque import applyPolicy
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
        self.assertRaiseRuntimeError(self.instance,'history')
        self.assertRaiseRuntimeError(self.instance,'balance')

    def testAllow(self):
        self.assertEqual(self.instance.owner,'Bart')
        self.assertTrue(callable(self.instance.withdraw))
        self.assertTrue(callable(self.instance.deposit))

if __name__ == '__main__':
    unittest.main()