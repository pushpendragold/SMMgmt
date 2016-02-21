all: Create Read Clean
Create:
	g++ -g -c mainCreate.cpp
	g++ -g mainCreate.o ../lib/MemoryMgmt.o -o Create
Read:
	g++ -g -c mainRead.cpp
	g++ -g mainRead.o ../lib/MemoryMgmt.o -o Read
Clean:
	g++ -g -c mainClean.cpp
	g++ -g mainClean.o ../lib/MemoryMgmt.o -o Clean
clean:
	rm -f core* *.o Create Clean Read
