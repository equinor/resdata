#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <ecl_util.h>
#include <dirent.h>
#include <util.h>
#include <hash.h>
#include <stringlist.h>



/**
   This little function writes the pathetic little file read by
   ECLIPSE on startup. This is actually read from stdin, so on startup
   of ECLIPSE stdin must be redirected to this file.
*/
   

static void ecl_util_init_stdin__(const char * stdin_file , const char * ecl_base , int max_cpu_sec , int max_wall_sec) {
  FILE * stream = util_fopen(stdin_file , "w");
  fprintf(stream , "%s\n" , ecl_base);
  fprintf(stream , "%s\n" , ecl_base);
  fprintf(stream , "%d\n" , max_cpu_sec);
  fprintf(stream , "%d\n\n" , max_wall_sec);
  fclose(stream);
}

void ecl_util_init_stdin(const char * stdin_file, const char * ecl_base) {
  ecl_util_init_stdin__(stdin_file , ecl_base , 10000000 , 99999999);  /* Default values applied at Hydro Sandsli. */ 
}



char * ecl_util_alloc_base_guess(const char * path) {
  char *base = NULL;
  int   data_count = 0;
  struct dirent *dentry;
  DIR *dirH;
  dirH = opendir( path ); 
  if (dirH == NULL)
    util_abort("%s: failed to open directory: %s - aborting.\n",__func__ , path);
  
  while ( (dentry = readdir(dirH)) != NULL) {
    const char * entry = dentry->d_name;
    char *this_base , *ext;
    
    if (entry[0] == '.') continue; 
    util_alloc_file_components(entry , NULL , &this_base , &ext);
    if (ext != NULL) {
      
      if ((strcmp(ext,"DATA") == 0) || (strcmp(ext , "data") == 0)) {
	if (data_count == 0) 
	  base = util_alloc_string_copy(this_base);
	else if (data_count == 1) {
	  free(base);
	  base = NULL;
	}
	data_count++;
      }
      
      free(ext);
    }
    if (this_base != NULL) free(this_base);
    
  }
  closedir(dirH);
  
  if (data_count > 1)
    fprintf(stderr,"%s: found several files with extension DATA in:%s  can not guess ECLIPSE base - returning NULL\n",__func__ , path);
  else if (data_count == 0)
    fprintf(stderr,"%s: could not find any files ending with data / DATA in:%s - can not guess ECLIPSE base - returning NULL \n",__func__ , path);
  return base;
}



int ecl_util_filename_report_nr(const char *filename) {
  int report_nr = -1;
  char *ext = strrchr(filename , '.');
  if (ext == NULL) 
    util_abort("%s: can not determine timestep from filename:%s - aborting \n",__func__ , filename);
  
  if (ext[1] == 'X' || ext[1] == 'F' || ext[1] == 'S' || ext[1] == 'A') 
    report_nr = atoi(&ext[2]);
  else 
    util_abort("%s: Filename:%s not recognized - valid extensions: Annnn / Xnnnn / Fnnnn / Snnnn - aborting \n",__func__ , filename);
  
  return report_nr;
}


/*
bool ecl_util_numeric_extension(const char * extension) {
  
  const char digit_ascii_min = 48;
  const char digit_ascii_max = 57;
  bool valid = true;
  int pos = 1;
  while (valid && pos <= 5) {
    char c = extension[pos];
    valid = (valid & (c >= digit_ascii_min && c <= digit_ascii_max));
    if (!valid)
      break;
  }
  
  return valid;
}
*/



/** 
  This function takes an eclipse filename as input - looks at the
  extension, and uses that to determine the type of file. In addition
  to the fundamental type, it is also determined whether the file is
  formatted or not, and in the case of summary/restart files, which
  report number this corresponds to.

  The function itself returns void, all the results are by reference.
*/


void ecl_util_get_file_type(const char * filename, ecl_file_type * _file_type , bool *_fmt_file, int * _report_nr) {
  const bool ecl_other_ok = true;
  ecl_file_type file_type = ecl_other_file;
  bool fmt_file = true;
  int report_nr = -1;
  
  char *ext = strrchr(filename , '.');
  if (ext != NULL) {
    ext++;
    if (strcmp(ext , "UNRST") == 0) {
      file_type = ecl_unified_restart_file;
      fmt_file = false;
    } else if (strcmp(ext , "FUNRST") == 0) {
      file_type = ecl_unified_restart_file;
      fmt_file = true;
    } else if (strcmp(ext , "UNSMRY") == 0) {
      file_type = ecl_unified_summary_file;
      fmt_file  = false;
    } else if (strcmp(ext , "FUNSMRY") == 0) {
      file_type = ecl_unified_summary_file;
      fmt_file  = true;
    } else if (strcmp(ext , "SMSPEC") == 0) {
      file_type = ecl_summary_header_file;
      fmt_file  = false;
    } else if (strcmp(ext , "FSMSPEC") == 0) {
      file_type = ecl_summary_header_file;
      fmt_file  = true;
    } else if (strcmp(ext , "GRID") == 0) {
      file_type = ecl_grid_file;
      fmt_file  = false;
    } else if (strcmp(ext , "FGRID") == 0) {
      file_type = ecl_grid_file;
      fmt_file  = true;
    } else if (strcmp(ext , "EGRID") == 0) {
      file_type = ecl_egrid_file;
      fmt_file  = false;
    } else if (strcmp(ext , "FEGRID") == 0) {
      file_type = ecl_egrid_file;
      fmt_file  = true;
    } else if (strcmp(ext , "INIT") == 0) {
      file_type = ecl_init_file;
      fmt_file  = false;
    } else if (strcmp(ext , "FINIT") == 0) {
      file_type = ecl_init_file;
      fmt_file  = true;
    } else if (strcmp(ext , "FRFT") == 0) {
      file_type = ecl_rft_file;
      fmt_file  = true;
    } else if (strcmp(ext , "RFT") == 0) {
      file_type = ecl_rft_file;
      fmt_file  = false;
    } else if (strcmp(ext , "DATA") == 0) {
      file_type = ecl_data_file;
      fmt_file  = true;  /* Not really relevant ... */
    } else {
      switch (ext[0]) {
      case('X'):
	file_type = ecl_restart_file;
	fmt_file  = false;
	break;
      case('F'):
	file_type = ecl_restart_file;
	fmt_file  = true;
	break;
      case('S'):
	file_type = ecl_summary_file;
	fmt_file  = false;
	break;
      case('A'):
	file_type = ecl_summary_file;
	fmt_file  = true;
	break;
      default:
	file_type = ecl_other_file;
	/*
	  fprintf(stderr,"*** Warning: *** %s failed to find type of file:%s \n",__func__ , filename);
	*/
      }
      if (file_type != ecl_other_file) 
	if (!util_sscanf_int(&ext[1] , &report_nr))
	  file_type = ecl_other_file;
    }
  }

  if (_file_type != NULL)
    *_file_type = file_type;

  if (_fmt_file != NULL)
    *_fmt_file  = fmt_file;

  if (_report_nr != NULL)  
    *_report_nr = report_nr;
  
  if ( (file_type == ecl_other_file) && !ecl_other_ok) 
    util_abort("%s: Can not determine type of:%s from filename - aborting \n",__func__ , filename);
  
}



/**
   This function takes a path, along with a filetype as input and
   allocates a new string with the filename. If path == NULL, the
   filename is allocated without a leading path component.
*/

static char * ecl_util_alloc_filename_static(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr, bool must_exist) {
  char * filename;
  char * ext;
  switch (file_type) {
  case(ecl_restart_file):
    if (fmt_file)
      ext = util_alloc_sprintf("F%04d" , report_nr);
    else
      ext = util_alloc_sprintf("X%04d" , report_nr);
    break;

  case(ecl_unified_restart_file):
    if (fmt_file)
      ext = util_alloc_string_copy("FUNRST");
    else
      ext = util_alloc_string_copy("UNRST");
    break;

  case(ecl_summary_file):
    if (fmt_file)
      ext = util_alloc_sprintf("A%04d" , report_nr);
    else
      ext = util_alloc_sprintf("S%04d" , report_nr);
    break;
    
  case(ecl_unified_summary_file):
    if (fmt_file)
      ext = util_alloc_string_copy("FUNSMRY");
    else
      ext = util_alloc_string_copy("UNSMRY");
    break;

  case(ecl_summary_header_file):
    if (fmt_file) 
      ext = util_alloc_string_copy("FSMSPEC");
    else
      ext = util_alloc_string_copy("SMSPEC");
    break;

  case(ecl_grid_file):
    if (fmt_file) 
      ext = util_alloc_string_copy("FGRID");
    else
      ext = util_alloc_string_copy("GRID");
    break;
    
  case(ecl_egrid_file):
    if (fmt_file) 
      ext = util_alloc_string_copy("FEGRID");
    else
      ext = util_alloc_string_copy("EGRID");
    break;

  case(ecl_init_file):
    if (fmt_file) 
      ext = util_alloc_string_copy("FINIT");
    else
      ext = util_alloc_string_copy("INIT");
    break;

  case(ecl_rft_file):
    if (fmt_file) 
      ext = util_alloc_string_copy("FRFT");
    else
      ext = util_alloc_string_copy("RFT");
    break;

  case(ecl_data_file):
    ext = util_alloc_string_copy("DATA");
    break;
    
  default:
    util_abort("%s: Invalid input file_type to ecl_util_alloc_filename - aborting \n",__func__);
    /* Dummy to shut up compiler */
    ext        = NULL;
  }

  filename = util_alloc_filename(path , base , ext);
  free(ext);

  if (must_exist) {
    const int max_usleep_time = 10000000;  /* 10 seconds   */ 
    const int usleep_time     =    10000;  /* 1/100 second */
    int   total_usleep_time   =        0;


    /*
      If we require the file to exist we do several attempts, waiting
      up to ten seconds. The reason for this funny approach is that we
      have quite a lot of problems with file-system synchronization (I
      think ...). Where a file which clearly "is there" fails to show
      up.
    */
    

    while (1) {
      if (util_file_exists(filename)) 
	break;
      else {
	/*
	  If aborting is not permissible you must first allocate the name
	  in the normal way, and then check whether it existst in the calling
	  unit. Tough luck ...
	*/
	if (total_usleep_time >= max_usleep_time) 
	  util_abort("%s: file:%s does not exist - aborting \n",__func__ , filename);
	
	total_usleep_time += usleep_time;
	usleep(usleep_time);
      }
    }
  }
  
  return filename;
}


char * ecl_util_alloc_filename(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr) {
  return ecl_util_alloc_filename_static(path , base , file_type ,fmt_file , report_nr , false);
}


static char ** ecl_util_alloc_filelist_static(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr1 , int report_nr2, bool must_exist) {
  if (report_nr2 < report_nr1) 
    util_abort("%s: Invalid input report_nr1:%d > report_nr:%d - aborting \n",__func__ , report_nr1 , report_nr2);

  {
    char ** file_list = (char **) util_malloc((report_nr2 - report_nr1 + 1) * sizeof * file_list , __func__);
    int report_nr;
    for (report_nr = report_nr1; report_nr <= report_nr2; report_nr++)
      file_list[report_nr - report_nr1] = ecl_util_alloc_filename_static(path , base , file_type , fmt_file , report_nr , must_exist);
    return file_list;
  }
}

char ** ecl_util_alloc_filelist(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr1 , int report_nr2) {
  return ecl_util_alloc_filelist_static(path , base , file_type , fmt_file , report_nr1 , report_nr2 , false);
}

char ** ecl_util_alloc_exfilelist(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr1 , int report_nr2) {
  return ecl_util_alloc_filelist_static(path , base , file_type , fmt_file , report_nr1 , report_nr2 , true);
}


char * ecl_util_alloc_exfilename(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr) {
  return ecl_util_alloc_filename_static(path , base , file_type ,fmt_file , report_nr , true);
}


static int ecl_util_fname_cmp(const void *f1, const void *f2) {
  int t1 = ecl_util_filename_report_nr( *((const char **) f1) );
  int t2 = ecl_util_filename_report_nr( *((const char **) f2) );
  if (t1 < t2)
    return -1;
  else if (t1 > t2)
    return 1;
  else
    return 0;
}


static bool ecl_util_filetype_p(const char * filename , int type_mask , bool _fmt_file) {
  ecl_file_type file_type;
  int report_nr;
  bool fmt_file;
  ecl_util_get_file_type(filename , &file_type , &fmt_file , &report_nr);

  if (fmt_file == _fmt_file) {
    if ((type_mask & file_type) != 0)
      return true;
    else
      return false;
  } else
    return false;
}



char ** ecl_util_alloc_scandir_filelist(const char *_path , const char *base, ecl_file_type file_type , bool fmt_file , int *_files) {
  char *path; 
  
  if (_path != NULL)
    path = (char *) _path;
  else 
    path = util_alloc_cwd();
  
  {
    struct dirent *dentry;
    char **fileList;
    int files;
    DIR * dirH = opendir(path);
    
    if (dirH == NULL) 
      util_abort("\n%s: opening directory:%s failed - aborting \n",__func__ , path);

    files = 0;
    while ((dentry = readdir(dirH)) != NULL) {
      if (ecl_util_filetype_p(dentry->d_name , file_type , fmt_file))
	files++;
    }
    rewinddir(dirH);
    
    if (files == 0) 
      fileList = NULL;
    else {
      fileList = (char **) util_malloc(files * sizeof *fileList , __func__ );
      files = 0;
      while ((dentry = readdir (dirH)) != NULL) {
	if (ecl_util_filetype_p(dentry->d_name , file_type , fmt_file)) {
	  fileList[files] = util_alloc_full_path(path , dentry->d_name);
	  files++;
	}
      }
    }
    closedir(dirH);

    *_files = files;
    if (files > 0)
      qsort(fileList , files , sizeof *fileList , &ecl_util_fname_cmp);

    if (_path == NULL)
      free(path);
    return fileList;
  }
}


char ** ecl_util_alloc_simple_filelist(const char *path , const char *base, ecl_file_type file_type , bool fmt_file , int report_nr1 , int report_nr2) {
  char ** fileList = (char **) util_malloc((report_nr2 - report_nr1 + 1) * sizeof * fileList , __func__);
  int report_nr;
  for (report_nr = report_nr1; report_nr <= report_nr2; report_nr++) 
    fileList[report_nr - report_nr1] = ecl_util_alloc_filename_static(path , base , file_type , fmt_file , report_nr , false);
  
  return fileList;
}



bool ecl_util_unified(ecl_file_type file_type) {
  bool unified = true;

  switch (file_type) {
  case(ecl_other_file):
    unified = true;
    break;
  case(ecl_restart_file):
    unified = false;
    break;
  case(ecl_unified_restart_file):
    unified = true;
    break;
  case(ecl_summary_file):
    unified = false;
    break;
  case(ecl_unified_summary_file):
    unified = true;
    break;
  case(ecl_summary_header_file):
    unified = false;
    break;
  case(ecl_grid_file):
    unified = true;
    break;
  case(ecl_egrid_file):
    unified = false;
    break;
  case(ecl_init_file):
    unified = false;
    break;
  case(ecl_rft_file):
    unified = true;
    break;
  default:
    util_abort("%s: internal error - file_type:%d invalid input - aborting \n",__func__ , file_type);
  }

  return unified;
}

/*****************************************************************/

int ecl_util_get_sizeof_ctype(ecl_type_enum ecl_type) {
  int sizeof_ctype = -1;
  switch (ecl_type) {
  case(ecl_char_type):
    sizeof_ctype = (ecl_str_len + 1) * sizeof(char);
    break;
  case(ecl_float_type):
    sizeof_ctype = sizeof(float);
    break;
  case(ecl_double_type):
    sizeof_ctype = sizeof(double);
    break;
  case(ecl_int_type):
    sizeof_ctype = sizeof(int);
    break;
  case(ecl_bool_type):
    sizeof_ctype = sizeof(int);
    break;
  case(ecl_mess_type):
    sizeof_ctype = sizeof(char);
    break;
  default:
    util_abort("Internal error in %s - internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_type);
  }
  return sizeof_ctype;
}


/**
 This function copies size elements from _src_data to target_data. If
 src_type == target_type the copy is a simple memcpy, otherwise the
 appropriate numerical conversion is applied.
*/

void ecl_util_memcpy_typed_data(void *_target_data , const void * _src_data , ecl_type_enum target_type , ecl_type_enum src_type, int size) {
  int i;

  if (target_type == src_type) 
    memcpy(_target_data , _src_data , size * ecl_util_get_sizeof_ctype(src_type));
  else {
    switch (target_type) {
    case(ecl_double_type):
      {
	double * target_data = (double *) _target_data;
	switch(src_type) {
	case(ecl_float_type):
	  util_float_to_double(target_data , (const float *) _src_data , size);
	  break;
	case(ecl_int_type):
	  for (i = 0; i < size; i++) 
	    target_data[i] = ((int *) _src_data)[i];
	  break;
	default:
	  util_abort("%s: double type can only load from int/float/double - aborting \n",__func__);
	}
	break;
      }
    case(ecl_float_type):
      {
	float * target_data = (float *) _target_data;
	switch(src_type) {
	case(ecl_float_type):
	  util_double_to_float(target_data , (const double *) _src_data , size);
	  break;
	case(ecl_int_type):
	  for (i = 0; i < size; i++) 
	    target_data[i] = ((int *) _src_data)[i];
	  break;
	default:
	  util_abort("%s: float type can only load from int/float/double - aborting \n",__func__);
	}
	break;
      }
    default:
      util_abort("%s con not convert %d -> %d \n",__func__ , src_type , target_type);
    }
  }
}


ecl_type_enum ecl_util_guess_type(const char * key){ 
  hash_type * type_hash = hash_alloc( );
  ecl_type_enum type = ecl_float_type;  /* Keep compiler silent / happy .*/

  hash_insert_int(type_hash , "PERMX"  , ecl_float_type);
  hash_insert_int(type_hash , "PERMZ"  , ecl_float_type);
  hash_insert_int(type_hash , "PERMY"  , ecl_float_type);
  hash_insert_int(type_hash , "PORO"   , ecl_float_type);
  hash_insert_int(type_hash , "COORD"  , ecl_float_type);
  hash_insert_int(type_hash , "ZCORN"  , ecl_float_type);
  hash_insert_int(type_hash , "ACTNUM" , ecl_int_type);
  
  if (hash_has_key(type_hash , key)) 
    type = (ecl_type_enum) hash_get_int(type_hash , key);
  else 
    util_abort("could not guess type of keyword %s - update the table in %s/%s - aborting \n",key , __FILE__ , __func__);

  

  hash_free(type_hash);
  return type;
}


/**
   This routine allocates summary header and data files from a
   directory; path and base are input. For the header file there are
   two possible files:

     1. X.FSMSPEC
     2. X.SMSPEEC

   For the data there are four different possibilities:

     1. X.A0001, X.A0002, X.A0003, ... 
     2. X.FUNSMRY
     3. X.S0001, X.S0002, X.S0003, ... 
     4. X.UNSMRY
  
   In principle a directory can contain all different (altough that is
   probably not typical). The algorithm is a a two step algorithm:

     1. Determine wether to use X.FSMSPEC or X.SMSPEC based on which
        is the newest. This also implies a decision of wether to use
        formatted, or unformatted filed.
 
     2. Use formatted or unformatted files according to 1. above, and
        then choose either a list of files or unified files according
        to which is the newest.
   
   This algorithm should work in most practical cases, but it is
   surely possible to fool it.
*/


void ecl_util_alloc_summary_files(const char * path , const char * _base , char ** _header_file , char *** _data_files , int * _num_data_files , bool * _fmt_file , bool * _unified) {
  bool    fmt_file    	 = true; 
  bool    unified     	 = true;
  char  * header_file 	 = NULL;
  char ** data_files  	 = NULL;
  int     num_data_files = 0;
  char  * base;

  if (_base == NULL)
    base = ecl_util_alloc_base_guess(path);
  else
    base = (char *) _base;
  
  {
    char * fsmspec_file = ecl_util_alloc_filename(path , base , ecl_summary_header_file , true  , -1);
    char *  smspec_file = ecl_util_alloc_filename(path , base , ecl_summary_header_file , false , -1);
    if (util_file_exists(fsmspec_file) && util_file_exists(smspec_file)) {
      if (util_file_difftime(fsmspec_file , smspec_file) < 0) {
	header_file = fsmspec_file;
	free(smspec_file);
	fmt_file = true;
      } else {
	header_file = smspec_file;
	free(fsmspec_file);
	fmt_file = false;
      }
    } else if (util_file_exists(fsmspec_file)) {
      header_file = fsmspec_file;
      free(smspec_file);
      fmt_file = true;
    } else if (util_file_exists(smspec_file)) {
      header_file = smspec_file;
      free(fsmspec_file);
      fmt_file = false;
    } else {
      if (path == NULL)
	util_abort("%s: could not find either %s or %s - can not load summary data from %s.DATA \n",__func__ , fsmspec_file , smspec_file , base);
      else
	util_abort("%s: could not find either %s or %s - can not load summary data from %s/%s.DATA \n",__func__ , fsmspec_file , smspec_file , path , base);
    }
  }
  {
    int files;
    char  * unif_data_file = ecl_util_alloc_filename(path , base , ecl_unified_summary_file , fmt_file , -1);
    char ** file_list      = ecl_util_alloc_scandir_filelist(path , base , ecl_summary_file , fmt_file , &files); 
    bool    unif_exists    = util_file_exists(unif_data_file);            
    
    
    
    if ((files > 0) && unif_exists) {
      bool unified_newest = true;
      int file_nr = 0;
      while (unified_newest && (file_nr < files)) {
	if (util_file_difftime(file_list[file_nr] , unif_data_file) < 0) 
	  unified_newest = false;
	file_nr++;
      }
      
      if (unified_newest) {
	util_free_stringlist( file_list , files );
	data_files     = (char **) util_malloc( sizeof * data_files , __func__);
	data_files[0]  = unif_data_file;
	unified        = true;
	num_data_files = 1;
      } else {
	free(unif_data_file);
	unified    = false;
	data_files = file_list;
	num_data_files = files;
      }
    } else if (files > 0) {
      free(unif_data_file);
      unified    = false;
      data_files = file_list;
      num_data_files = files;
    } else if (unif_exists) {
      util_free_stringlist( file_list , files );
      data_files     = (char **) util_malloc( sizeof * data_files , __func__);
      data_files[0]  = unif_data_file;
      unified        = true;
      num_data_files = 1;
    } else 
      util_abort("%s: could not find summary data in %s - aborting.\n",__func__ , path );
  }
  
  if (_base == NULL)
    free(base);

  *_fmt_file  	   = fmt_file;
  *_unified   	   = unified;
  *_num_data_files = num_data_files;
  *_header_file    = header_file;
  *_data_files     = data_files;
}



void ecl_util_alloc_restart_files(const char * path , const char * _base , char *** _restart_files , int * num_restart_files , bool * _fmt_file , bool * _unified) {
  char * base = NULL;
  if (_base == NULL)
    base = ecl_util_alloc_base_guess(path);
  else
    base = (char *) _base;
  {
    int num_F_files;
    int num_X_files;

    char *  unrst_file  = ecl_util_alloc_filename(path , base , ecl_unified_restart_file , false , -1);  
    char *  funrst_file = ecl_util_alloc_filename(path , base , ecl_unified_restart_file , true  , -1);
    char *  unif_file   = NULL; 

    char ** F_files     = ecl_util_alloc_scandir_filelist(path , base , ecl_restart_file , true  , &num_F_files); 
    char ** X_files     = ecl_util_alloc_scandir_filelist(path , base , ecl_restart_file , false , &num_X_files); 
    char *  FX_file      = NULL;
    char *  final_file; 

    /*
      Ok now we have formatted/unformatted unified and not
      unified: Time to check what exists in the filesystem, and which
      is the newest.
    */
    unif_file = util_newest_file(unrst_file , funrst_file);
    
    if (num_F_files > 0 || num_X_files > 0) {    
      if (num_F_files > 0 && num_X_files > 0) {
	/* 
	   We have both a list of .Fnnnn and a list of .Xnnnn files; if
	   the length of lists is not equal we take the longest,
	   otherwise we compare the dates of the last files in the
	   list. 
	*/
	if (num_F_files == num_X_files) {
	  FX_file = util_newest_file( F_files[num_F_files - 1] , X_files[num_X_files - 1]);
	} else if (num_F_files > num_X_files)
	  FX_file = F_files[num_F_files - 1];
	else
	  FX_file = X_files[num_X_files - 1];
      } else if (num_F_files > 0)
	FX_file = F_files[num_F_files - 1];
      else
	FX_file = X_files[num_X_files - 1];

      if (unif_file != NULL)
	final_file = util_newest_file(unif_file , FX_file);
      else
	final_file = FX_file;
    } else
      final_file = unif_file;
      
    
    if (final_file == NULL) 
      util_abort("%s: could not find any restart data in %s/%s \n",__func__ , path , base);
    

    /* 
       Determine type of final_file. Thois block is where the return
       values are actually set.
    */
    {
      char ** restart_files;
      bool fmt_file , unified;
      ecl_file_type file_type;
      
      ecl_util_get_file_type( final_file , &file_type , &fmt_file , NULL);
      if (file_type == ecl_unified_restart_file) {
	*num_restart_files = 1;
	restart_files = util_malloc(sizeof * restart_files, __func__);
	restart_files[0] = util_alloc_string_copy( final_file );
	unified = true;
      } else {
	restart_files = ecl_util_alloc_scandir_filelist( path , base , ecl_restart_file , fmt_file , num_restart_files);
	unified = false;
      }
      *_restart_files = restart_files;

      if (_fmt_file != NULL) *_fmt_file = fmt_file;
      if (_unified  != NULL) *_unified  = unified; 
    }

    util_free_stringlist(F_files , num_F_files);
    util_free_stringlist(X_files , num_X_files);
    free(unrst_file);
    free(funrst_file);
  }

  if (_base == NULL)
    free(base);
}



/**
  This little function will take an ecl_type_enum variable as input,
  and return a constant string description of the type. Observe that
  these strings can *NOT* be used when writing eclipse files; then
  internal functionality in ecl_kw.c should be used.
*/


const char * ecl_util_type_name(ecl_type_enum ecl_type) {
  switch (ecl_type) {
  case(ecl_char_type):
    return "ecl_char_type";
    break;
  case(ecl_float_type):
    return "ecl_float_type";
    break;
  case(ecl_double_type):
    return "ecl_double_type";
    break;
  case(ecl_int_type):
    return "ecl_int_type";
    break;
  case(ecl_bool_type):
    return "ecl_bool_type";
    break;
  case(ecl_mess_type):
    return "ecl_mess_type";
    break;
  default:
    util_abort("%s: unrecognized ecl_type value:%d - aborting \n",__func__ , ecl_type);
  }
  return NULL;  /* This should never happen */
}


/**
This little function escapes eclipse keyword names so that they can be
safely used as filenames, i.e for instance the substitution:

   1/FVFGAS -> 1-FVFGAS

The escape process is done 'in-place' memory-wise.
*/
void ecl_util_escape_kw(char * kw) {
  uint index;
  for (index = 0; index < strlen(kw); index++) {
    switch (kw[index]) {
    case('/'):
      kw[index] = '-';
      break;
    case('\\'):
      kw[index] = '-';
      break;
    }
  }
}



/*
  I have *intentionally* dived straight at the problem of extracting
  the start_date; otherwise one might quite quickly end up with a
  half-baked DATA-file parser. I think that path leads straight to an
  asylum. But of course - not many points are awarded for pointing out
  that this parsing is extremly ugly.

    ECLIPSE100 has default date: 1. of january 1983.
    ECLIPSE300 has default date: 1. of january 1990.

  They don't have much style those fuckers at Schlum ...
*/



time_t ecl_util_get_start_date(const char * data_file) { 
  time_t start_date  = -1;
  FILE * stream      = util_fopen(data_file , "r");
  char * line        = NULL;
  bool   at_eof      = false;
  bool   start_found = false;
  int    line_start;
  
  do {
    char * start_pos;
    line_start = ftell(stream);
    line = util_fscanf_realloc_line(stream , &at_eof , line);
    util_strupr(line);
    start_pos = strstr(line , "START");
    if (start_pos != NULL) {
      /* 
	 OK - we have found START - must go back and check that it is
	 not in a section which is commented out.
      */
      char * comment_start = strstr(line , "--");
      start_found = true;
      if (comment_start != NULL)
	if (comment_start < start_pos)
	  start_found = false; /* Sorry - it was in a comment */
    }
  } while (!start_found && !at_eof);
  free(line);
  if (!start_found) 
    util_abort("%s: sorry - could not find START in DATA file %s \n",__func__ , data_file);
  
  {
    int c;
    int buffer_length = 0;
    char * buffer;


    fseek(stream , line_start , SEEK_SET);
    /* This will be fooled by a commented out termination '/' */
    do {
      c = fgetc(stream);
      buffer_length++;
    } while (c != '/');
    buffer = (char *) util_malloc(buffer_length + 1 , __func__);
    buffer[buffer_length] = '\0';

    {
      int comment_mode = 0;
      int pos      = 0;
      int file_pos = 0;

      fseek(stream , line_start , SEEK_SET);
      do {
	c = fgetc(stream);
	file_pos++;
	if (c == '-')
	  comment_mode++;
	else 
	  if (comment_mode == 0) {
	    if (!(c == '\r' || c == '\n')) {
	      buffer[pos] = c;
	      pos++;
	    }
	  } else {
	    /* Just looking for newline */
	    if (c == '\r' || c == '\n')
	      comment_mode = 0;
	  }
      } while (file_pos < buffer_length);
    } 
    

    
    /* Searching for the first numeric character */
    {
      int pos = 0;
      while (!isdigit(buffer[pos]) && pos <= buffer_length) 
	pos++;

      if (!isdigit(buffer[pos])) 
	util_abort("%s: sorry - failed to detect start date from DATA file \n",__func__);
      
      {
	int day, year, month_nr;
	char * month_str = (char *) util_malloc(32 , __func__);
	{
	  int scanf_count = sscanf(&buffer[pos] , "%d %s %d" , &day , month_str , &year);
	  if (scanf_count != 3)
	    util_abort("%s: failed to parse DAY MONTH YEAR from: %s \n",__func__ , &buffer[pos]);
	}
	month_str = util_realloc_dequoted_string( month_str );
	
	month_nr   = util_get_month_nr(month_str);
	start_date = util_make_date(day , month_nr , year );
	free(month_str);
      }
    }
    free(buffer);
  }
  
  fclose(stream);
  return start_date;
}
