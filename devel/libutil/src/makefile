include global_config
SVN_VERSION 	   = $(shell svnversion ./)
COMPILE_TIME_STAMP = $(shell date)

#################################################################
COMPILE_INCLUDE   = -I./  
COMPILE_LIB_PATH  = 
INSTALL_INC_PATH = $(LIBUTIL_HOME)/include
INSTALL_LIB_PATH = $(LIBUTIL_HOME)/lib
#################################################################
VECTOR_HEADERS    = int_vector.h double_vector.h float_vector.h bool_vector.h time_t_vector.h long_vector.h
VECTOR_OBJECTS    = int_vector.o double_vector.o float_vector.o bool_vector.o time_t_vector.o long_vector.o
VECTOR_SOURCE     = int_vector.c double_vector.c float_vector.c bool_vector.c time_t_vector.c long_vector.c
SDP_ROOT          = $(shell get_sdp_root.py)
OBJECTS           = $(VECTOR_OBJECTS)  set.o hash_node.o hash_sll.o hash.o list_node.o list.o node_data.o node_ctype.o util.o thread_pool.o msg.o arg_pack.o str_buffer.o path_fmt.o menu.o subst.o vector.o parser.o stringlist.o matrix.o matrix_lapack.o matrix_blas.o conf_new.o buffer.o block_fs.o log.o template.o timer.o
INC_FILES_INSTALL = set.h hash.h hash_node.h hash_sll.h list_node.h list.h node_data.h node_ctype.h util.h thread_pool.h msg.h arg_pack.h str_buffer.h path_fmt.h  stringlist.h menu.h subst.h vector.h parser.h $(VECTOR_HEADERS) matrix.h matrix_lapack.h matrix_blas.h conf_new.h buffer.h block_fs.h log.h template.h timer.h
LIB               = libutil.a
LIB_LINK          = -lutil -lz -lpthread -llapack -lblas -lm
LINK_LIB_PATH     = -L./ 


LOCAL: LIB
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)


SDP_INSTALL: LIB
	install $(LIB) $(SDP_ROOT)/lib/$(LIB)
	install $(INC_FILES_INSTALL) $(SDP_ROOT)/include


clean:
	rm -f *.o *~ 
	rm -f $(VECTOR_HEADERS) $(VECTOR_OBJECTS)
	rm -f libutil_*
	rm -f $(INSTALL_LIB_PATH)/$(LIB)
	rm -f $(INSTALL_INC_PATH)/*.h


rebuild: clean new LOCAL

new: 
	../../Scripts/cdep.py all_include.h 0

LIB:	$(LIB)

$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

$(VECTOR_HEADERS): vector_template.h
	sed -e 's/<TYPE>/int/g'    vector_template.h > int_vector.h
	sed -e 's/<TYPE>/float/g'  vector_template.h > float_vector.h
	sed -e 's/<TYPE>/double/g' vector_template.h > double_vector.h
	sed -e 's/<TYPE>/bool/g'   vector_template.h > bool_vector.h
	sed -e 's/<TYPE>/time_t/g' vector_template.h > time_t_vector.h
	sed -e 's/<TYPE>/long/g'   vector_template.h > long_vector.h

$(VECTOR_SOURCE): vector_template.c $(VECTOR_HEADERS) util.o
	sed -e 's/<TYPE>/int/g'    vector_template.c > int_vector.c
	sed -e 's/<TYPE>/float/g'  vector_template.c > float_vector.c
	sed -e 's/<TYPE>/double/g' vector_template.c > double_vector.c
	sed -e 's/<TYPE>/bool/g'   vector_template.c > bool_vector.c
	sed -e 's/<TYPE>/time_t/g' vector_template.c > time_t_vector.c
	sed -e 's/<TYPE>/long/g'   vector_template.c > long_vector.c

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) -D'SVN_VERSION="$(SVN_VERSION)"' -D'COMPILE_TIME_STAMP="$(COMPILE_TIME_STAMP)"' $< -o $@

include dependencies
