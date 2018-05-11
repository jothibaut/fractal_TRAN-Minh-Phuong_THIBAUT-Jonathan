main : main.o lib
	cc -pthread -g -o main main.o libfractal/libfractal.a -lSDL 

Tests : 
	$(MAKE) -C ./tests/
	cd ./tests/ && export LD_LIBRARY_PATH=$$HOME/local/lib:$$LD_LIBRARY_PATH && ./testReadFile &&./testCompute
lib : 
	$(MAKE) -C ./libfractal/

main.o : main.c ./libfractal/fractal.h
	cc -c -g -Ilibfractal/ main.c

clean :
	rm main main.o ./libfractal/fractal.o ./libfractal/tools.o ./libfractal/libfractal.a

cleanTests :
	rm ./tests/testCompute ./tests/testCompute.o ./tests/testReadFile ./tests/testReadFile.o
