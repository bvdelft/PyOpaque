import priv

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

priv.registerTargetClass(account)

priv.exportGetAttr(account,"balance")
priv.exportGetAttr(account,"withdraw")
priv.exportGetAttr(account,"deposit")
priv.exportGetAttr(account,"owner")
#priv.exportGetAttr(account,"history")

priv.finalizeTargetClass(account,"account")

account = priv.builder(account).build

del priv
