all: cOpaquemodule.so

cOpaquemodule.so: cOpaquemodule.cpp cOpaquemodule.h setupcOpaque.py
	python setupcOpaque.py build
	cp build/lib*/cOpaquemodule.so .
	rm -rf build
	
clean:
	rm -rf build
	rm cOpaquemodule.so
