class A():
	def __init__(self,q):
		self.field = q
		self.data = q
		
def polT():
	return True

def polF():
	return False

# Fails dramatically:
# A = cOpaque.makeOpaque(A, [ ("field",polT) ] )
# instead include nest.py
