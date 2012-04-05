import cOpaque
from simple import A

## note: should not allow access to these policies, otherwise can be changed?
##... but changing results in failure to call the policy

def polT():
	return True

def polF():
	return False

A = cOpaque.makeOpaque(A, [ ("field",polF), ("data",polT) ] )

# Do not delete policies, yields segmentation fault!
#del polF
#del polT
