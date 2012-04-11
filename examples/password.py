class password():
     def __init__(self,username,password):
          self.username = username
          self.password = password
          self.triesLeft = 3

     def checkPassword(self,guessPassword):
     	if (self.triesLeft == 0):
     	  print "No more tries left!"
     	  return
		if (self.guessPassword == password) :
         	self.triesLeft = 3
         	print "very good!"
        else:
            self.triesLeft -= 1


import opaque
account=opaque.applyPolicy('opaque.cfg',account)
