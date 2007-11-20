MFLAG = m32
CC = gcc
FC = ifort
AR = ar
#################################################################
COMPILE_INCLUDE  = -I./ -I../../libutil/include -I../../libhash/include -I../../libecl/include -I../../librms/include -I../../libsched/include 
INSTALL_INC_PATH = ../include
INSTALL_LIB_PATH = ../lib
#################################################################
CFLAGS  = -$(MFLAG) -O3 -std=gnu99 -g -Wall  $(COMPILE_INCLUDE) 
ARFLAGS = -csr
#################################################################
OBJECTS           = field_interC.o thist_interC.o thist.o scalar.o scalar_config.o layer_config.o trans_func.o logmode.o multflt_interC.o multz_interC.o enkf_types.o enkf_obs.o obs_data.o obs_node.o well_obs.o field_obs.o meas_op.o meas_data.o well_config.o well.o enkf_config_node.o field_config.o field.o ecl_static_kw_config.o ecl_static_kw.o equil_config.o equil.o enkf_state.o multflt.o enkf_util.o multflt_config.o enkf_config.o multz.o multz_config.o enkf_node.o
INC_FILES_INSTALL = thist.h scalar.h scalar_config.h layer_config.h trans_func.h logmode.h enkf_obs.h obs_data.h obs_node.h well_obs.h meas_op.h meas_data.h well_config.h well.h enkf_config_node.h field_obs.h field_config.h field.h enkf_macros.h ecl_static_kw_config.h ecl_static_kw.h equil_config.h equil.h enkf_state.h multflt.h enkf_util.h multflt_config.h enkf_config.h multz.h multz_config.h enkf_node.h

LIB_LINK          = -lenkf_$(MFLAG) -lsched_$(MFLAG) -lrms_$(MFLAG) -lecl_$(MFLAG) -lhash_$(MFLAG) -lutil_$(MFLAG) -lm -lpthread
LIB_PATH          = -L../../libhash/lib -L../../libutil/lib -L../../libecl/lib -L../../librms/lib -L../../libsched/lib -L$(INSTALL_LIB_PATH)
LIB               = libenkf_$(MFLAG).a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc $(CFLAGS) enkf_test.c -o enkf.x   -L./ $(LIB_PATH) $(LIB_LINK) 
	gcc $(CFLAGS) thist_test.c -o thist.x -L./ $(LIB_PATH) $(LIB_LINK)
	gcc $(CFLAGS) field_test.c -o field.x -L./ $(LIB_PATH) $(LIB_LINK)
	gcc $(CFLAGS) swcr_init.c -o /d/proj/bg/enkf/bin/swcr_init.x  -L./ $(LIB_PATH) $(LIB_LINK)


clean:
	rm -f *.o *~
	rm -f $(INSTALL_INC_PATH)/*.h
	rm -f libenkf_*
	rm -f $(INSTALL_LIB_PATH)/$(LIB)


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

thist_interC.o       : thist.o
thist.o              : thist.c thist.h
layer_config.o       : layer_config.c layer_config.h field_config.o
logmode.o            : logmode.c logmode.h enkf_types.h
multflt_interC.o     : multflt_interC.c multflt.o multflt_config.o scalar.o
multz_interC.o       : multz_interC.c multz.o multz_config.o scalar.o
scalar.o               : scalar.h scalar.c enkf_util.o scalar_config.o
enkf_types.o         : enkf_types.c enkf_types.h
enkf_obs.o           : enkf_obs.c enkf_obs.h enkf_config.o obs_node.o well_obs.o field_obs.o
obs_data.o           : obs_data.c obs_data.h
obs_node.o           : obs_node.c obs_node.h enkf_types.h
meas_data.o          : meas_data.c meas_data.h enkf_util.o
well_obs.o           : well_obs.h well_obs.c meas_op.o
meas_op.o            : meas_op.c meas_op.h
well_config.o        : enkf_config.c enkf_config.h enkf_types.h enkf_macros.h enkf_config_node.o well_config.c well_config.h well_config_private.h
well.o               : well.c well.h well_config.o 
enkf_config_node.o   : enkf_config.c enkf_config.h enkf_types.h enkf_macros.h
field_config.o       : field_config.c field_config.h enkf_types.h
field.o              : field.c field.h field_config.o enkf_types.h enkf_macros.h
equil_config.o : enkf_util.h equil_config.c equil_config.h enkf_macros.h
equil.o        : equil.c equil_config.o equil.h enkf_macros.h enkf_types.h
enkf_state.o   : enkf_util.h enkf_state.c enkf_state.h enkf_node.o enkf_macros.h field.o field_config.o multz.o multflt.o equil.o multz_config.o multflt_config.o equil_config.o well.o well_config.o
multflt_config.o : multflt_config.c multflt_config.h enkf_macros.h
multflt.o      : enkf_util.h multflt.c multflt.h multflt_config.o enkf_util.o logmode.o
ecl_config.o   : enkf_util.h ecl_config.c ecl_config.h enkf_macros.h enkf_config_node.o
multz_config.o : enkf_util.h multz_config.c multz_config.h enkf_macros.h logmode.o
multz.o        : enkf_util.h multz.c multz.h multz_config.o logmode.o
enkf_node.o    : enkf_util.h enkf_node.h enkf_node.c enkf_macros.h
field_interC.o : field_interC.c field.o field_config.o
field_obs.o    : field_obs.c field_obs.h field.o field_config.o


