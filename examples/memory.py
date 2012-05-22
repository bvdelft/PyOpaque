def search(arg,mem) :
	for o in mem :
		try :
			if arg in o.__repr__() :
				if isinstance(o, dict) :
					return o
		except :
			continue
