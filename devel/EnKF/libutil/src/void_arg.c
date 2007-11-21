#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <void_arg.h>

struct void_arg_struct {
  int      arg_size;
  int      byte_size;
  size_t  *size_list;
  int  	  *offset_list;
  char 	  *argList;
};


void_arg_type * void_arg_alloc(int arg_size , const int * size_list) {
  int i;
  void_arg_type * arg = malloc(sizeof *arg);
  arg->arg_size  = arg_size;
  arg->byte_size = 0;
  arg->size_list   = calloc(arg_size , sizeof *arg->size_list);
  arg->offset_list = calloc(arg_size , sizeof *arg->offset_list);
  for (i=0; i < arg_size; i++) {
    arg->size_list[i] = size_list[i];
    arg->byte_size   += size_list[i];
    if (i == 0)
      arg->offset_list[i] = 0;
    else
      arg->offset_list[i] = arg->offset_list[i-1] + size_list[i-1];
  }
    
  arg->argList = malloc(arg->byte_size * sizeof * arg->argList);
  return arg;
}


void_arg_type * void_arg_alloc2(int size1, int size2) {
  return void_arg_alloc(2 , (const int[2]) {size1 , size2});
}

void_arg_type * void_arg_alloc3(int size1, int size2, int size3) {
  return void_arg_alloc(3 , (const int[3]) {size1 , size2, size3});
}

void_arg_type * void_arg_alloc4(int size1, int size2, int size3, int size4) {
  return void_arg_alloc(4 , (const int[4]) {size1 , size2, size3, size4});
}

void_arg_type * void_arg_alloc5(int size1, int size2, int size3, int size4, int size5) {
  return void_arg_alloc(5 , (const int[5]) {size1 , size2, size3, size4 , size5});
}

void_arg_type * void_arg_alloc6(int size1, int size2, int size3, int size4, int size5, int size6) {
  return void_arg_alloc(6 , (const int[6]) {size1 , size2, size3, size4 , size5 , size6});
}

void_arg_type * void_arg_alloc7(int size1, int size2, int size3, int size4, int size5, int size6, int size7) {
  return void_arg_alloc(7 , (const int[7]) {size1 , size2, size3, size4 , size5 , size6 , size7});
}

void_arg_type * void_arg_alloc8(int size1, int size2, int size3, int size4, int size5, int size6, int size7, int size8) {
  return void_arg_alloc(8 , (const int[8]) {size1 , size2, size3, size4 , size5 , size6 , size7 , size8});
}

void_arg_type * void_arg_alloc9(int size1, int size2, int size3, int size4, int size5, int size6, int size7, int size8 , int size9) {
  return void_arg_alloc(9 , (const int[9]) {size1 , size2, size3, size4 , size5 , size6 , size7 , size8, size9});
}

void_arg_type * void_arg_alloc10(int size1, int size2, int size3, int size4, int size5, int size6, int size7, int size8 , int size9, int size10) {
  return void_arg_alloc(10 , (const int[10]) {size1 , size2, size3, size4 , size5 , size6 , size7 , size8, size9 , size10});
}




void void_arg_free(void_arg_type * arg) {
  free(arg->argList);
  free(arg->size_list);
  free(arg);
}


static void __void_arg_assert_index(const void_arg_type * arg , int iarg) {
  if (iarg < 0 || iarg >= arg->arg_size) {
    fprintf(stderr,"%s: void_arg() object allocated with %d arguments - %d invalid argument number - aborting \n",__func__ , arg->arg_size , iarg);
    abort();
  }
}


void void_arg_pack_ptr(void_arg_type * arg, int iarg , void * input) {
  __void_arg_assert_index(arg , iarg);
  memcpy(&arg->argList[arg->offset_list[iarg]] , input , arg->size_list[iarg]);
}


void void_arg_unpack_ptr(const void_arg_type * arg , int iarg , void * output) {
  __void_arg_assert_index(arg , iarg);
  memcpy(output , &arg->argList[arg->offset_list[iarg]] , (const size_t) arg->size_list[iarg]);
}


void * void_arg_get_target_ptr(const void_arg_type * arg, int iarg) {
  __void_arg_assert_index(arg , iarg);
  return (void *) (*( (size_t *) &arg->argList[arg->offset_list[iarg]]));
}

void * void_arg_get_data_ptr(const void_arg_type * arg, int iarg) {
  __void_arg_assert_index(arg , iarg);
  return &arg->argList[arg->offset_list[iarg]];
}



void void_arg_fscanf(void_arg_type * arg, const char * fmt , FILE * stream) {
  switch(arg->arg_size) {
  case(0):
    break;
  case(1):
    {
      void   *arg;
      arg = void_arg_get_data_ptr(arg , 0);
      
      fscanf(stream , fmt , arg);
      break;
    }
  case(2):
    {
      void   *arg0, *arg1;
      arg0 = void_arg_get_data_ptr(arg , 0);
      arg1 = void_arg_get_data_ptr(arg , 1);

      fscanf(stream , fmt , arg0 , arg1);
      break;
    }
  case(3):
    {
      void   *arg0, *arg1 , *arg2;
      arg0 = void_arg_get_data_ptr(arg , 0);
      arg1 = void_arg_get_data_ptr(arg , 1);
      arg2 = void_arg_get_data_ptr(arg , 2);
      
      fscanf(stream , fmt , arg0 , arg1 , arg2);
      break;
    }
  default:
    fprintf(stderr,"%s: sorry arg_fscanf() not allocated for %d arguments - pathettic ehhh?? \n",__func__ , arg->arg_size);
  }
}
