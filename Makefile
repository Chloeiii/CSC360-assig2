.phony all:

all: MFS.c
	 gcc MFS.c -lpthread -o MFS 

.PHONY clean:
clean:
	-rm -rf *.o *.exe
