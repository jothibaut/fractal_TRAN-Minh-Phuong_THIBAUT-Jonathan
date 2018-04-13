main : main.o 
	$(MAKE) -C ./libfractal/
lib : 
	$(MAKE) -C ./libfractal/
main.o : main.c ./libfractal/fractal.h
	cc -c -Ilibfractal/ main.c

clean :
	rm main.o ./libfractal/fractal.o ./libfractal/tools.o ./libfractal/libfractal.a
