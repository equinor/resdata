CC = gcc
FC = ifort
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./
COMPILE_LIB_PATH  = 
COMPILE_LIBS      = 

INSTALL_INC_PATH = ../include
INSTALL_LIB_PATH = ../lib
INSTALL_MOD_PATH = ../include
#################################################################
CFLAGS  = -O2 -m32 -std=gnu99 -g -Wall $(COMPILE_INCLUDE) 
ARFLAGS = -csr
#################################################################
OBJECTS   = set.o hash_node.o hash_sll.o hash.o list_node.o list.o node_data.o
INC_FILES = set.h hash.h hash_node.h hash_sll.h list_node.h list.h node_data.h
LIB       = libhash.a




INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc -m32 -Wall -g -I./ hash_test.c -o hash.x -L./ -L../../libutil/lib -lhash -lutil

new:	clean INSTALL

clean:
	rm -f *.o *~ 

$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.f90
	$(FC) -c $(FFLAGS) $< -o $@


set.o       : set.c set.h hash.o
node_data.o : node_data.c node_data.h
list_node.o : list_node.c list_node.h
list.o      : list.c list.h list_node.o node_data.o
hash_node.o : hash_node.c hash_node.h
hash_sll.o  : hash_sll.c hash_sll.h hash_node.o
hash.o      : hash.c hash.h hash_sll.o node_data.o


