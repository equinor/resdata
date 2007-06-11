#include <stdlib.h>
#include <util.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pathv.h>

struct pathv_struct {
  int size;
  char **path_vector;
  
  char *path;
};


/*****************************************************************/

static void pathv_iset__(pathv_type * pathv , int i , const char *path) {
  if (i > 0 || i < pathv->size) {
    if (path != NULL) {
      pathv->path_vector[i] = realloc(pathv->path_vector[i] , strlen(path) + 1);
      strcpy(pathv->path_vector[i] , path);
    } else {
      if (pathv->path_vector[i] != NULL)
	free(pathv->path_vector[i]);
      pathv->path_vector[i] = NULL;
    }
  } else {
    fprintf(stderr,"%s: i=%d invalid - aborting \n",__func__ , i);
    abort();
  }
}


static void pathv_makeit(pathv_type * pathv) {
  int i;
  int set_level = 0;
  bool all_set = true;

  for (i=0; i < pathv->size; i++) {
    if (pathv->path_vector[i] == NULL) 
      all_set = false;
    else {
      if (all_set) 
	set_level = i+1;
    }
  }
      

  if (set_level > 0) {  
    int len;
    len = 0;
    for (i = 0; i < set_level; i++)
      len += strlen(pathv->path_vector[i]);
    len += pathv->size;
    pathv->path = realloc(pathv->path , len);
    pathv->path[0] = '\0';
    for (i = 0; i < set_level; i++) {
      strcat(pathv->path , pathv->path_vector[i]);
      if (i < (pathv->size - 1))
	strcat(pathv->path , "/");
    }
    util_make_path(pathv->path);
  } else {
    if (pathv->path != NULL) {
      free(pathv->path);
      pathv->path = NULL;
    }
  }
}


/*****************************************************************/





pathv_type * pathv_alloc(int size , const char **path_vector) {
  int i;
  pathv_type *pathv = malloc(sizeof *pathv);
  pathv->size        = size;
  pathv->path_vector = calloc(size , sizeof * pathv->path_vector);
  pathv->path        = NULL;

  for (i=0; i < size; i++)
     pathv->path_vector[i] = NULL;
  
  if (path_vector != NULL) 
    pathv_vset(pathv , path_vector);
  
  return pathv;
}



void pathv_vset(pathv_type * pathv , const char **path_vector) {
  int i;
  for (i=0; i < pathv->size; i++)
    pathv_iset__(pathv , i , path_vector[i]);
  pathv_makeit(pathv);
}



void pathv_iset(pathv_type * pathv , int i, const char *path) {
  pathv_iset__(pathv , i , path);
  pathv_makeit(pathv);
}


void pathv_init(pathv_type *pathv , int i , const char * path) {
  pathv_iset__(pathv , i , path);
}


void pathv_free(pathv_type * pathv) {
  int i;
  if (pathv->path != NULL) free(pathv->path);
  for (i=0; i < pathv->size; i++)
    if (pathv->path_vector[i] != NULL) free(pathv->path_vector[i]);

  free(pathv->path_vector);
  free(pathv);
}

const char     * pathv_get_ref(const pathv_type *pathv) { return pathv->path; }
