#include <util.h>
#include <time.h>
#include <buffer.h>


static void fix_file(const char * filename, const char * target_path , buffer_type * buffer) {
  int  iens;
  char ** tmp;
  int num;
  util_split_string( filename , "." , &num , &tmp);
  util_sscanf_int( tmp[2] , &iens );
  
  {
    char * new_file      = util_alloc_sprintf("%s%c%s_%04d" , target_path , UTIL_PATH_SEP_CHAR , tmp[0] , iens);
    FILE * target_stream = util_fopen( new_file , "w");

    buffer_fread_realloc( buffer , filename );
    {
      char * ptr      =  buffer_get_data( buffer );
      int    elements = (buffer_get_size(buffer) - 12)/8;
      double *   data =  (double *) &ptr[12];
      for (int i=0; i < elements; i++)
        fprintf(target_stream , "%g\n",data[i]);
    }

    fclose( target_stream );
  }
  util_free_stringlist( tmp , num );
}


int main( int argc , char ** argv) {
  int iarg;
  buffer_type * buffer = buffer_alloc(199);
  
  for (iarg = 1; iarg < argc; iarg++) 
    fix_file( argv[iarg] , "init_files" , buffer);
    
}
