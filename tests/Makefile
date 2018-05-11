all : testCompute testReadFile 


testCompute : testCompute.o
	cc -o testCompute testCompute.o -g -Wall -I$$HOME/local/include -L$$HOME/local/lib -lSDL -lpthread -lcunit 
	
testCompute.o : testCompute.c 
	cc -c testCompute.c -g -Wall -I$$HOME/local/include -lcunit

testReadFile : testReadFile.o
	cc -o testReadFile testReadFile.o -g -Wall -I$$HOME/local/include -L$$HOME/local/lib -lSDL -lpthread -lcunit 
	
testReadFile.o : testReadFile.c 
	cc -c testReadFile.c -g -Wall -I$$HOME/local/include -lcunit

clean: 
	rm testCompute testCompute.o testReadFile testReadFile.o

