#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <util.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <stringlist.h>
#include <menu.h>
#include <subst.h>
#include <arg_pack.h>
#include <vector.h>
#include <double_vector.h>
#include <matrix.h>
#include <matrix_lapack.h>
#include <matrix_blas.h>
#include <parser.h>
#include <block_fs.h>




void write_random_file( block_fs_type * fs , const char * prefix , int max_file , int min_size , int max_size , int * randint) {
  int byte_size = (rand() % (max_size - min_size)) + min_size;
  int int_size  = byte_size / 4;
  
  for (int i = 0; i < int_size; i++) 
    randint[i] = rand();
  
  char * filename = util_alloc_sprintf("%s.%d" , prefix , (rand() % max_file));
  block_fs_fwrite_file( fs , filename , randint , byte_size );
  
  {
    char * backup_file = util_alloc_filename("/tmp" , filename , NULL);
    FILE * stream = util_fopen(backup_file , "w");
    util_fwrite( randint , 1 , byte_size , stream , __func__);
    fclose( stream );
    free( backup_file );
  }

  for (int i= 0; i < 5; i++) {
    filename = util_realloc_sprintf(filename , "%s.%d" , prefix , (rand() % max_file));
    if (block_fs_has_file( fs , filename )) {
      char * backup_file = util_alloc_filename("/tmp" , filename , NULL);
      block_fs_unlink_file( fs , filename );
      unlink( backup_file );
      free( backup_file );
    }
  }
  free( filename );
}



void check_file(block_fs_type * block_fs , const char * backup_file , const char * filename , void * block_buffer , void * plain_buffer) {
  int byte_size = util_file_size( backup_file );
  if (block_fs_has_file( block_fs , filename )) {
    printf("Filename:%s Size: %d - %d ",filename , byte_size , block_fs_get_filesize( block_fs , filename ));
    {
      FILE * stream = util_fopen( backup_file , "r");
      util_fread(plain_buffer , 1 , byte_size ,stream , __func__);
      fclose( stream );
    }
    
    block_fs_fread_file( block_fs , filename , block_buffer );
    
    if ( memcmp( block_buffer , plain_buffer , byte_size ) != 0) {
      printf("%-15s : ERROR \n",filename);
      exit(1);
    }
    
    printf("OK \n\n");
  } else printf("File:%s does not exist in block_fs \n",filename);
}




void check_all(block_fs_type * block_fs , int max_file , const char * prefix , void * buffer1 , void * buffer2) {
  int i;
  for (i=0; i < max_file; i++) {
    char * block_file = util_alloc_sprintf("%s.%d",prefix , i);
    char * fs_file    = util_alloc_sprintf("/tmp/%s.%d",prefix, i);
    
    if (util_file_exists( fs_file )) 
      check_file( block_fs , fs_file , block_file , buffer1 , buffer2 );
    
    free( block_file );
    free( fs_file);
  }
}


void random_test(int outer_loop , int inner_loop) {
  int max_file            = 1000;
  int min_size            = 10;
  int max_size            = 1067;
  const int block_size    = 1;
  const char * mount_file = "/tmp/block_fs.mnt";
  char       * prefix;
  int   * buffer          = util_malloc( sizeof * buffer * 2 * (max_size / 4 + 1) , __func__);
  int   * buffer2         = util_malloc( sizeof * buffer * 2 * (max_size / 4 + 1) , __func__);

  {
    FILE * stream = util_fopen("/dev/random" , "r");
    int seed = util_fread_int( stream );
    srand(seed);
    fclose( stream );
  }
  
  prefix = util_alloc_sprintf("FileTest_%03d" , rand() % 1000);

  for (int j=0; j < outer_loop; j++) {
    block_fs_type * fs;
    fs = block_fs_mount( mount_file , block_size , 128 , 0.25 , 100 , false , false);  /* Realloc on each round - just drop the existing fs instance on the floor(). Testing abiility 
                                                                                    to recover from crashes. */
    {
      char * index_file = util_alloc_sprintf("initial_index.%d" , j);
      FILE * stream = util_fopen(index_file , "w");
      fclose( stream );
      free( index_file );
    } 
    printf("j:%d \n",j);
    {
      /** Small files */
      for (int i =0; i < inner_loop; i++) {
        write_random_file( fs , prefix , max_file , min_size , 2*min_size , buffer );
      }
      
      /** All sizes */
      for (int i =0; i < inner_loop; i++) {
        write_random_file( fs , prefix , max_file , min_size , max_size , buffer );
      }
      
      /** Large files */
      for (int i =0; i < inner_loop; i++) {
        write_random_file( fs , prefix , max_file , max_size / 2 , max_size , buffer );
      }
    }
    
    printf("-----------------------------------------------------------------\n");
    {
      char * index_file = util_alloc_sprintf("final_index.%d" , j);
      FILE * stream = util_fopen(index_file , "w");
      fclose( stream );
      free( index_file );
    }
    
    //block_fs_close( fs , true);
  }
  {
    block_fs_type * fs = block_fs_mount( mount_file , block_size , 1024 , 0.25 , 100 , false , false);
    
    check_all(fs , max_file , prefix , buffer , buffer2);
    block_fs_defrag( fs );
    block_fs_close( fs , true);
  }
  free( buffer );
  free( buffer2 );
  free( prefix );
}



void speed_test(bool write , int N) {
  const int block_size    = 4;
  const char * basename   = "WWCT";
  const char * mount_file = "/d/proj/bg/enkf/tmp/Kast3/summary.mnt";

  char spinner[4] = {'/' , '-' , '\\' , '|'};
  buffer_type * buffer = buffer_alloc(16);
  int int_value       = 0;
  double double_value = 100;
  block_fs_type * fs;
  int i;


  fs = block_fs_mount( mount_file , block_size , 1024 , 1.0 , 100 , false , false);
  
  if (write) {
    for (i=0; i < N; i++) {
      char * block_file = util_alloc_sprintf("%s.%d" , basename , i);
      char * plain_file = util_alloc_sprintf("/d/proj/bg/enkf/tmp/Kast3/%s.%d" , basename , i);
      
      buffer_clear( buffer );
      buffer_fwrite_time_t( buffer , time( NULL ));
      buffer_fwrite_int( buffer , int_value );
      buffer_fwrite_double( buffer , double_value );

      
      
      buffer_store( buffer , plain_file );
      block_fs_fwrite_buffer( fs , block_file , buffer );
      
      if (i % 1000 == 0) {
        printf("\b%c" , spinner[(i / 1000) % 4]); 
        fflush( stdout );
      }
      
      free( block_file );
      free( plain_file );
    }
  }

  fs = block_fs_mount( mount_file , block_size , 1024 , 1.0 , 100 , false , false);
  
  {
    clock_t start_time;
    clock_t end_time;
    
    printf("-----------------------------------------------------------------\n");
    printf("==>  ");
    start_time = clock();
    for (i=0; i < N; i++) {
      char * block_file = util_alloc_sprintf("%s.%d" , basename , i);
      block_fs_fread_realloc_buffer( fs , block_file , buffer );
      if (i % 1000 == 0) {
        printf("\b%c" , spinner[(i / 1000) % 4]); 
        fflush( stdout );
      }
      
      free( block_file );
    }
    end_time = clock();
    printf("\nBlock_fs: %d \n",end_time - start_time);
  
    printf("\n-----------------------------------------------------------------\n");
    printf("==>  ");
    start_time = clock();
    for (i=0; i < N; i++) {
      char * plain_file = util_alloc_sprintf("/d/proj/bg/enkf/tmp/Kast3/%s.%d" , basename , i);
      buffer_fread_realloc( buffer , plain_file );
      if (i % 1000 == 0) {
        printf("\b%c" , spinner[(i / 1000) % 4]); 
        fflush( stdout );
      }
      free( plain_file );
    }
    end_time = clock();
    printf("Plain_fs: %d \n",end_time - start_time);
    
    printf("-----------------------------------------------------------------\n");
  }
  block_fs_close( fs , true );
  buffer_free( buffer );
}



void large_test(int external_loops , int internal_loops) {
  int external_counter;
  block_fs_type * block_fs = block_fs_mount("/tmp/large.mnt" , 1 , 0 , 1.0 , 100 , false  , false);
  
  int buffer_size = 65538 * 16;
  void * buffer   = util_malloc( buffer_size , __func__);
  
  for (external_counter = 0; external_counter < external_loops; external_counter++) {

    for (int internal_counter = 0; internal_counter < internal_loops; internal_counter++) {
      char * key = util_alloc_sprintf("%s.%d.%d" , "Large" , external_counter , internal_counter);
      block_fs_fwrite_file( block_fs , key , buffer , buffer_size );
      free( key );
    }
    
    
    for (int internal_counter = 0; internal_counter < internal_loops; internal_counter++) {
      char * key = util_alloc_sprintf("%s.%d.%d" , "Large" , external_counter , internal_counter);
      block_fs_fread_file( block_fs , key , buffer );
      free( key );
    }
    
    printf("external_index:%d file_size:%ld \n" , external_counter , util_file_size("/tmp/large.data_0"));
  }
  block_fs_close( block_fs , true );
}






int main(int argc , char ** argv) {
  signal(SIGFPE , util_abort_signal);    /* Segmentation violation, i.e. overwriting memory ... */
  //large_test(100   , 100);
  random_test(25 , 100);
  //speed_test(true , 10000);
}


