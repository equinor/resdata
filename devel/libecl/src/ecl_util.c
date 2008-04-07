#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ecl_util.h>
#include <dirent.h>
#include <util.h>
#include <hash.h>







char * ecl_util_alloc_base_guess(const char * path) {
  char *base = NULL;
  char *cwd  = NULL;
  int   data_count = 0;
  cwd  = getcwd(cwd , 0);
  if (chdir(path) != 0) {
    fprintf(stderr,"%s: failed to change to %s - aborting \n", __func__ , path);
    abort();
  } else {
    struct dirent *dentry;
    DIR *dirH;
    dirH = opendir( "./" );  /* Have already changed into this directory with chdir() */
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
    chdir(cwd);
    free(cwd);
  }

  if (data_count > 1)
    fprintf(stderr,"%s: found several files with extension DATA in:%s  can not guess ECLIPSE base - returning NULL\n",__func__ , path);
  else if (data_count == 0)
    fprintf(stderr,"%s: could not find any files ending with data / DATA in:%s - can not guess ECLIPSE base - returning NULL \n",__func__ , path);
  
  return base;
}



int ecl_util_filename_report_nr(const char *filename) {
  int report_nr;
  char *ext = strrchr(filename , '.');
  if (ext == NULL) {
    fprintf(stderr,"%s: can not determine timestep from filename:%s - aborting \n",__func__ , filename);
    abort();
  }
  
  if (ext[1] == 'X' || ext[1] == 'F' || ext[1] == 'S' || ext[1] == 'A') 
    report_nr = atoi(&ext[2]);
  else {
    fprintf(stderr,"%s: Filename:%s not recognized - valid extensions: Annnn / Xnnnn / Fnnnn / Snnnn - aborting \n",__func__ , filename);
    abort();
  } 
  
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
  
  if ( (file_type == ecl_other_file) && !ecl_other_ok) {
    fprintf(stderr,"%s: Can not determine type of:%s from filename - aborting \n",__func__ , filename);
    abort();
  }
  
}



char * ecl_util_alloc_filename_static(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr, bool must_exist) {
  char * filename;
  char * ext;
  int ext_length;
  switch (file_type) {
  case(ecl_restart_file):
    ext_length = 5;
    ext = malloc(ext_length + 1);
    if (fmt_file)
      sprintf(ext , "F%04d" , report_nr);
    else
      sprintf(ext , "X%04d" , report_nr);
    break;

  case(ecl_unified_restart_file):
    if (fmt_file) 
      ext_length = 6;
    else
      ext_length = 5;
    ext = malloc(ext_length + 1);
    if (fmt_file)
      strcpy(ext , "FUNRST");
    else
      strcpy(ext , "UNRST");
    break;

  case(ecl_summary_file):
    ext_length = 5;
    ext = malloc(ext_length + 1);
    if (fmt_file)
      sprintf(ext , "A%04d" , report_nr);
    else
      sprintf(ext , "S%04d" , report_nr);
    break;
    
  case(ecl_unified_summary_file):
    if (fmt_file)
      ext_length = 7;
    else
      ext_length = 6;
    ext = malloc(ext_length + 1);
    if (fmt_file)
      strcpy(ext , "FUNSMRY");
    else
      strcpy(ext , "UNSMRY");
    break;

  case(ecl_summary_header_file):
    if (fmt_file)
      ext_length = 7;
    else
      ext_length = 6;
    ext = malloc(ext_length + 1);
    if (fmt_file) 
      strcpy(ext , "FSMSPEC");
    else
      strcpy(ext , "SMSPEC");
    break;

  case(ecl_grid_file):
    if (fmt_file)
      ext_length = 5;
    else
      ext_length = 4;
    ext = malloc(ext_length + 1);
    if (fmt_file) 
      strcpy(ext , "FGRID");
    else
      strcpy(ext , "GRID");
    break;
    
  case(ecl_egrid_file):
    if (fmt_file)
      ext_length = 6;
    else
      ext_length = 5;
    ext = malloc(ext_length + 1);
    if (fmt_file) 
      strcpy(ext , "FEGRID");
    else
      strcpy(ext , "EGRID");
    break;

  case(ecl_init_file):
    if (fmt_file)
      ext_length = 5;
    else
      ext_length = 4;
    ext = malloc(ext_length + 1);
    if (fmt_file) 
      strcpy(ext , "FINIT");
    else
      strcpy(ext , "INIT");
    break;

  case(ecl_rft_file):
    if (fmt_file)
      ext_length = 4;
    else
      ext_length = 3;
    ext = malloc(ext_length + 1);
    if (fmt_file) 
      strcpy(ext , "FRFT");
    else
      strcpy(ext , "RFT");
    break;

  case(ecl_data_file):
    ext_length = 4;
    ext = util_alloc_string_copy("DATA");
    break;
    
  default:
    fprintf(stderr,"%s: Invalid input file_type to ecl_util_alloc_filename - aborting \n",__func__);
    abort();
  }
  if (path != NULL) {
    filename = malloc(strlen(path) + 1 + strlen(base) + 1 + ext_length + 1);
    sprintf(filename , "%s/%s.%s" , path , base , ext);
  } else {
    filename = malloc(strlen(base) + 1 + ext_length + 1);
    sprintf(filename , "%s.%s" , base , ext);
  }
  
  free(ext);

  if (must_exist) {
    if (!util_file_exists(filename)) {
      /*
	If aborting is not permissible you must first allocate the name
	in the normal way, and then check whether it existst in the calling
	unit. Tough luck ...
      */
      fprintf(stderr,"%s: file:%s does not exist - aborting \n",__func__ , filename);
      abort();
    }
  }
  
  return filename;
}


char * ecl_util_alloc_filename(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr) {
  return ecl_util_alloc_filename_static(path , base , file_type ,fmt_file , report_nr , false);
}


static char ** ecl_util_alloc_filelist_static(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr1 , int report_nr2, bool must_exist) {
  if (report_nr2 < report_nr1) {
    fprintf(stderr,"%s: Invalid input report_nr1:%d > report_nr:%d - aborting \n",__func__ , report_nr1 , report_nr2);
    abort();
  }
  {
    char ** file_list = util_malloc((report_nr2 - report_nr1 + 1) * sizeof * file_list , __func__);
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
    path = util_alloc_string_copy(".");
      
  {
    DIR * dirH = opendir(path);
    struct dirent *dentry;
    char **fileList;
    
    int files;
    if (dirH == NULL) {
      fprintf(stderr,"\n%s: opening directory:%s failed - aborting \n",__func__ , path);
      abort();
    }

    files = 0;
    while ((dentry = readdir (dirH)) != NULL) {
      if (ecl_util_filetype_p(dentry->d_name , file_type , fmt_file))
	files++;
    }
    rewinddir(dirH);
    
    if (files == 0) 
      fileList = NULL;
    else {
      fileList = calloc(files , sizeof *fileList);
      files = 0;
      while ((dentry = readdir (dirH)) != NULL) {
	if (ecl_util_filetype_p(dentry->d_name , file_type , fmt_file)) {
	  fileList[files] = malloc(strlen(path) + 1 + strlen(dentry->d_name) + 1);
	  sprintf(fileList[files] , "%s/%s" , path , dentry->d_name);
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
  char ** fileList = malloc((report_nr2 - report_nr1 + 1) * sizeof * fileList);
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
    fprintf(stderr,"%s: internal error - file_type:%d invalid input - aborting \n",__func__ , file_type);
    abort();
  }

  return unified;
}

/*****************************************************************/

int ecl_util_get_sizeof_ctype(ecl_type_enum ecl_type) {
  int sizeof_ctype;
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
    fprintf(stderr,"Internal error in %s - internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_type);
    abort();
  }
  return sizeof_ctype;
}


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
	  fprintf(stderr,"%s: double type can only load from int/float/double - aborting \n",__func__);
	  abort();
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
	  fprintf(stderr,"%s: float type can only load from int/float/double - aborting \n",__func__);
	  abort();
	}
	break;
      }
    default:
      fprintf(stderr,"%s con not convert %d -> %d \n",__func__ , src_type , target_type);
      abort();
    }
  }
}


ecl_type_enum ecl_util_guess_type(const char * key){ 
  hash_type * type_hash = hash_alloc(10);
  ecl_type_enum type;

  hash_insert_int(type_hash , "PERMX" , ecl_float_type);
  hash_insert_int(type_hash , "PERMZ" , ecl_float_type);
  hash_insert_int(type_hash , "PERMY" , ecl_float_type);
  hash_insert_int(type_hash , "PORO"  , ecl_float_type);
  hash_insert_int(type_hash , "COORD" , ecl_float_type);
  hash_insert_int(type_hash , "ZCORN" , ecl_float_type);
  hash_insert_int(type_hash , "ACTNUM" , ecl_int_type);
  
  if (hash_has_key(type_hash , key)) 
    type = hash_get_int(type_hash , key);
  else {
    fprintf(stderr,"could not guess type of keyword %s - update the table in %s/%s - aborting \n",key , __FILE__ , __func__);
    abort();
  }
  

  hash_free(type_hash);
  return type;
}


/**
This little function escapes eclipse keyword names so that they can be
safely used as filenames, i.e for instance the substitution:

   1/FVFGAS -> 1-FVFGAS

*/
void ecl_util_escape_kw(char * kw) {
  int index;
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
