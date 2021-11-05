head = lsh.h 
srcs = lsh.c 
objs = lsh.o 
opts = -g -c 

all: lsh 

lsh: $(objs)
	gcc $(objs) -l pthread -o lsh

lsh.o: $(srcs) $(head)
	gcc $(opts) $(srcs) 

clean:
	rm lsh *.o