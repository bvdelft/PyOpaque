class account():
     def __init__(self,owner="bart",balance=0):
          self.owner=owner
          self.balance=balance
          self.history=[]

     def withdraw(self,amount=0):
          self.history.append(('withdraw',amount))
          self.balance-=amount

     def deposit(self,amount=0):
          self.history.append(('deposit',amount))
          self.balance+=amount

from opaque import applyPolicy
account=applyPolicy(account,'account.cfg')
