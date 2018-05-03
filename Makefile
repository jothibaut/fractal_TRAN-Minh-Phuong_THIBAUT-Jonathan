main : main.o lib
	cc -pthread -o main main.o libfractal/libfractal.a -lSDL 
lib : 
	$(MAKE) -C ./libfractal/
main.o : main.c ./libfractal/fractal.h
	cc -c -Ilibfractal/ main.c

clean :
	rm main main.o ./libfractal/fractal.o ./libfractal/tools.o ./libfractal/libfractal.a
