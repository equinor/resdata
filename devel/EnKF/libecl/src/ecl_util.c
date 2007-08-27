#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ecl_util.h>
#include <dirent.h>
#include <util.h>





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



void ecl_util_get_file_type(const char * filename, ecl_file_type * _file_type , bool *_fmt_file, int * _report_nr) {
  const bool ecl_other_ok = true;
  ecl_file_type file_type = -1;
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
      }
      if (file_type != ecl_other_file)
	report_nr = atoi(&ext[1]);
    }
  }

  *_file_type = file_type;
  *_fmt_file  = fmt_file;
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

char * ecl_util_alloc_exfilename(const char * path, const char * base , ecl_file_type file_type , bool fmt_file, int report_nr) {
  return ecl_util_alloc_filename_static(path , base , file_type ,fmt_file , report_nr , true);
}


static bool is_numeric(char c) {
  if (c >= 48 && c <= 57)
    return true;
  else
    return false;
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


static bool ecl_fstate_include_file(const char *filename , const char *base, const char *ext_match) {
  if (strstr(filename , base) == filename) {
    char *substring_ptr = strstr(filename , ext_match);
    if (substring_ptr == NULL) 
      return false;
    else {
      bool include = true;
      int i;
      substring_ptr += strlen(ext_match);
      if (strlen(substring_ptr) == 4) {
	for (i=0; i < 4; i++)
	  include = include && is_numeric(substring_ptr[i]);
      } else 
	include = false;
      return include;
    }
  }  else return false;
}


char ** ecl_util_alloc_filelist(const char *path , const char *base, ecl_file_type file_type , bool fmt_file , int report_nr1 , int report_nr2) {
  char ** fileList = malloc(report_nr2 - report_nr1 + 1 * sizeof * fileList);
  int report_nr;
  for (report_nr = report_nr1; report_nr <= report_nr2; report_nr++)
    fileList[report_nr - report_nr1] = ecl_util_alloc_filename_static(path , base , file_type , fmt_file , report_nr , false);
  
  return fileList;
}


char ** ecl_util_alloc_exfilelist(const char *_path , const char *base, ecl_file_type file_type , bool fmt_file , int *_files) {
  char *path , *ext_match;
  switch (file_type) {
  case (ecl_restart_file):
    ext_match = malloc(3);
    if (fmt_file)
      strcpy(ext_match , ".F");
    else
      strcpy(ext_match , ".X");
    break;
  case(ecl_summary_file):
    ext_match = malloc(3);
    if (fmt_file)
      strcpy(ext_match , ".A");
    else
      strcpy(ext_match , ".S");
    break;
  default:
    fprintf(stderr,"%s: can (currently) only scan for restart and summary files - aborting \n",__func__);
    abort();
  }

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
      if (ecl_fstate_include_file(dentry->d_name , base, ext_match))
	files++;
    } 
    rewinddir(dirH);
    
    if (files == 0) 
      fileList = NULL;
    else {
      fileList = calloc(files , sizeof *fileList);
      files = 0;
      while ((dentry = readdir (dirH)) != NULL) {
	if (ecl_fstate_include_file(dentry->d_name , base , ext_match)) {
	  fileList[files] = malloc(strlen(path) + 1 + strlen(dentry->d_name) + 1);
	  sprintf(fileList[files] , "%s/%s" , path , dentry->d_name);
	  files++;
	}
      }
    }
    closedir(dirH);
    free(ext_match);

    *_files = files;
    if (files > 0)
      qsort(fileList , files , sizeof *fileList , &ecl_util_fname_cmp);

    if (_path == NULL)
      free(path);
    return fileList;
  }
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
  default:
    fprintf(stderr,"%s: internal error - file_type:%d invalid input - aborting \n",__func__ , file_type);
    abort();
  }

  return unified;
}




