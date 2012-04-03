all: cOpaquemodule.so

cOpaquemodule.so:
	python setupcOpaque.py build
	cp build/lib*/cOpaquemodule.so .
	rm -rf build
	
clean:
	rm -rf build
	rm cOpaquemodule.so
