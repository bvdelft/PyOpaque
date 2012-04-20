import md5

class password():
     def __init__(self,password):
          self.password =  md5.new(password).digest()
          self.triesLeft = 3
          self.username = "sample"

     def checkPassword(self,guessPassword):
     	if (self.triesLeft == 0):
     	  print "No more tries left!"
          return
        if self.password == md5.new(guessPassword).digest():
         	self.triesLeft = 3
         	print 'very good!'
        else:
            print 'wrong!'
            self.triesLeft -= 1

'''Uncomment the following lines to protect the password field'''
from opaque import opaque
password=opaque(['checkPassword','username'], ['password','triesLeft'] , True )(password)
pass1=password('thisisasecret')
