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
OBJECTS   = hash_node.o hash_sll.o hash.o
INC_FILES = hash.h hash_node.h hash_sll.h
LIB       = libhash.a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.f90
	$(FC) -c $(FFLAGS) $< -o $@

hash_node.o : hash_node.c hash_node.h
hash_sll.o  : hash_sll.c hash_sll.h hash_node.o
hash.o      : hash.c hash.h hash_sll.o


