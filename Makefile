main : main.o lib
	cc -pthread -g -o main main.o libfractal/libfractal.a -lSDL 
lib : 
	$(MAKE) -C ./libfractal/
main.o : main.c ./libfractal/fractal.h
	cc -c -g -Ilibfractal/ main.c

clean :
	rm main main.o ./libfractal/fractal.o ./libfractal/tools.o ./libfractal/libfractal.a
