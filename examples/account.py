import cOpaque

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

cOpaque.registerTargetClass(account)

cOpaque.exportGetAttr(account,"balance")
cOpaque.exportGetAttr(account,"withdraw")
cOpaque.exportGetAttr(account,"deposit")
cOpaque.exportGetAttr(account,"owner")
#cOpaque.exportGetAttr(account,"history")

cOpaque.finalizeTargetClass(account,"account")

account = cOpaque.builder(account).build

del cOpaque
