TARGET     = libecl
CC         = gcc4
CFLAGS     = -g -fPIC -Wall -std=gnu99 
HEADERS    = #../ecl_sum.h ../ecl_util.h
SRCS       = #../ecl_sum.c ../ecl_util.c
INCLUDES   = -I../../include -I../../../libutil/include
LIBS = -L../../../libutil/lib -L../../lib -lecl -lutil -lz -lm -lpthread
INTERFACE  = libecl.i
SWIG       = swig
 
LDSHARED   = ld
CSHARED    = -shared
 
OBJS       = $(SRCS:.c=.o)
 
IWRAP      = $(INTERFACE:.i=_wrap.i)
ISRCS      = $(IWRAP:.i=.c)
IOBJS      = $(IWRAP:.i=.o)
 
PYTHON_INCLUDE= -I/usr/include/python2.3 -I/usr/lib/python2.3
#PYTHON_INCLUDE= -I/usr/include/python2.2 -I/usr/lib/python2.2
PYTHON_SO     = .so
 
all: $(SRCS)
	$(SWIG) -python $(INTERFACE)
	$(CC) -c $(CCSHARED) $(CFLAGS) $(ISRCS) $(HEADERS) $(SRCS) $(INCLUDES) $(PYTHON_INCLUDE)
	$(LDSHARED) $(CSHARED) $(OBJS) $(IOBJS) $(LIBS) -o _$(TARGET)$(PYTHON_SO)
 
clean:
	rm -f *_wrap* *~ .~* *.pyc
	rm -f *.o *.so *.so

