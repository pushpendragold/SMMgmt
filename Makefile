all : 
	make -C lib
	make -C example
clean:
	make -C lib clean
	make -C example clean
