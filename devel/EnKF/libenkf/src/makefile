CC = gcc
FC = ifort
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./ -I../../libutil/include -I../../libhash/include -I../../libecl/include -I../../librms/include -I../../libsched/include 
COMPILE_LIB_PATH  = -L../../libhash/lib -L../../libecl/lib -L../../librms/lib -L../../libutil/lib -L../../libsched/lib
COMPILE_LIBS      = -lrms -lpthread -lsched -lecl -lutil -lhash -lm

INSTALL_INC_PATH = ../include
INSTALL_LIB_PATH = ../lib
INSTALL_MOD_PATH = ../mod
#################################################################
CFLAGS  = -m32 -O3 -std=gnu99 -g -Wall  $(COMPILE_INCLUDE) 
ARFLAGS = -csr
#################################################################
OBJECTS           = logmode.o multz_interC.o enkf_types.o enkf_obs.o obs_data.o obs_node.o well_obs.o meas_op.o meas_data.o well_config.o well.o enkf_config_node.o field_config.o field.o ecl_static_kw_config.o ecl_static_kw.o equil_config.o equil.o enkf_state.o multflt.o enkf_util.o multflt_config.o enkf_config.o multz.o multz_config.o enkf_node.o
INC_FILES_INSTALL = logmode.h enkf_obs.h obs_data.h obs_node.h well_obs.h meas_op.h meas_data.h well_config.h well.h enkf_config_node.h field_config.h field.h config.h ecl_static_kw_config.h ecl_static_kw.h equil_config.h equil.h enkf_state.h multflt.h enkf_util.h multflt_config.h enkf_config.h multz.h multz_config.h enkf_node.h
LIB               = libenkf.a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install -d $(INSTALL_MOD_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc -std=gnu99 -Wall -g -m32 -I./ $(COMPILE_INCLUDE) enkf_test.c -o enkf.x -L./ $(COMPILE_LIB_PATH) -lenkf $(COMPILE_LIBS)
	gcc -std=gnu99 -Wall -g -m32 -I./ $(COMPILE_INCLUDE) field_test.c -o field.x -L./ $(COMPILE_LIB_PATH) -lenkf $(COMPILE_LIBS)

clean:
	rm -f *.o *~

new:	clean INSTALL


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@


logmode.o            : logmode.c logmode.h enkf_types.h
multz_interC.o       : multz_interC.c multz.o multz_config.o
enkf_types.o         : enkf_types.c enkf_types.h
enkf_obs.o           : enkf_obs.c enkf_obs.h enkf_config.o obs_node.o
obs_data.o           : obs_data.c obs_data.h
obs_node.o           : obs_node.c obs_node.h enkf_types.h
meas_data.o          : meas_data.c meas_data.h enkf_util.o
well_obs.o           : well_obs.h well_obs.c meas_op.o
meas_op.o            : meas_op.c meas_op.h
well_config.o        : enkf_config.c enkf_config.h enkf_types.h config.h enkf_config_node.o well_config.c well_config.h well_config_private.h
well.o               : well.c well.h well_config.o 
enkf_config_node.o   : enkf_config.c enkf_config.h enkf_types.h config.h
field_config.o       : field_config.c field_config.h enkf_types.h
field.o              : field.c field.h field_config.o enkf_types.h config.h
equil_config.o : enkf_util.h equil_config.c equil_config.h config.h
equil.o        : equil.c equil_config.o equil.h config.h enkf_types.h
enkf_state.o   : enkf_util.h enkf_state.c enkf_state.h enkf_node.o config.h field.o field_config.o multz.o multflt.o equil.o multz_config.o multflt_config.o equil_config.o well.o well_config.o
multflt_config.o : multflt_config.c multflt_config.h config.h
multflt.o      : enkf_util.h multflt.c multflt.h multflt_config.o enkf_util.o 
ecl_config.o   : enkf_util.h ecl_config.c ecl_config.h config.h enkf_config_node.o
multz_config.o : enkf_util.h multz_config.c multz_config.h config.h logmode.o
multz.o        : enkf_util.h multz.c multz.h multz_config.o
enkf_node.o    : enkf_util.h enkf_node.h enkf_node.c config.h




