/*
   Copyright (C) 2018  Statoil ASA, Norway.

   The file 'util_portability.c' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#include <limits.h>
#include <errno.h>
#include <string.h>

#include <ert/util/util_portability.h>
#include <ert/util/ert_api_config.h>
#include "ert/util/build_config.h"



#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <Windows.h>
#endif

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#endif

#ifdef HAVE_SHLWAPI_H
#include <Shlwapi.h>
#endif

#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif

/**
   WIndows does not have the usleep() function, on the other hand
   Sleep() function in windows has millisecond resolution, instead of
   seconds as in linux.
*/

void util_usleep( unsigned long micro_seconds ) {
#ifdef HAVE__USLEEP
  usleep( micro_seconds );
#else
#ifdef ERT_WINDOWS
  {
    int milli_seconds = micro_seconds / 1000;
    Sleep( milli_seconds );
  }
#endif
#endif
}



static char * util_getcwd(char * buffer , int size) {
#ifdef HAVE_POSIX_GETCWD
  return getcwd( buffer , size );
#endif

#ifdef HAVE_WINDOWS_GETCWD
  return _getcwd( buffer , size );
#endif
}


char * util_alloc_cwd(void) {
  char * result_ptr;
  char * cwd;
  int buffer_size = 128;
  do {
    cwd = (char*)util_calloc(buffer_size , sizeof * cwd );
    result_ptr = util_getcwd(cwd , buffer_size - 1);
    if (result_ptr == NULL) {
      if (errno == ERANGE) {
        buffer_size *= 2;
        free(cwd);
      }
    }
  } while ( result_ptr == NULL );
  cwd = (char*)util_realloc(cwd , strlen(cwd) + 1 );
  return cwd;
}

/**
   Manual realpath() implementation to be used on platforms without
   realpath() support. Will remove /../, ./ and extra //. Will not
   handle symlinks.
*/


#define BACKREF ".."
#define CURRENT "."

char * util_alloc_realpath__(const char * input_path) {
  char * abs_path  = util_alloc_cwd_abs_path( input_path );
  char * real_path = (char*)util_malloc( strlen(abs_path) + 2 );
  real_path[0] = '\0';


  {
    char ** path_list;
    const char ** path_stack;
    int     path_len;

    util_path_split( abs_path , &path_len , &path_list );
    path_stack = (const char **) util_malloc( path_len * sizeof * path_stack );
    for (int i=0; i < path_len; i++)
      path_stack[i] = NULL;

    {
      int stack_size = 0;

      for (int path_index=0; path_index < path_len; path_index++) {
        const char * path_elm = path_list[path_index];

         if (strcmp( path_elm , CURRENT) == 0)
          continue;

        /* Backref - pop from stack. */
        if (strcmp(path_elm , BACKREF ) == 0) {
          if (stack_size > 0) {
            memmove(path_stack, &path_stack[1] , (stack_size - 1) * sizeof * path_stack);
            stack_size--;
          }
          continue;
        }

        /* Normal path element - push onto stack. */
        memmove(&path_stack[1], path_stack, stack_size * sizeof * path_stack);
        path_stack[0] = path_elm;
        stack_size++;
      }

      /* Build up the new string. */
      if (stack_size > 0) {
        for (int pos = stack_size - 1; pos >= 0; pos--) {
          const char * path_elm = path_stack[pos];
          if (pos == (stack_size- 1)) {
#ifdef ERT_WINDOWS
            // Windows:
            //   1) If the path starts with X: - just do nothing
            //   2) Else add \\ - for a UNC path.
            if (path_elm[1] != ':') {
              strcat(real_path, UTIL_PATH_SEP_STRING);
              strcat(real_path, UTIL_PATH_SEP_STRING);
            }
#else
            // Posix: just start with a leading '/'
            strcat(real_path, UTIL_PATH_SEP_STRING);
#endif
            strcat( real_path , path_elm);
          } else {
            strcat(real_path, UTIL_PATH_SEP_STRING);
            strcat(real_path , path_elm);
          }
        }
      }
    }
    free( path_stack );
    util_free_stringlist( path_list , path_len );
  }

  free(abs_path);
  return real_path;
}

#undef BACKREF
#undef CURRENT



int util_fnmatch( const char * pattern , const char * string ) {
#ifdef HAVE_FNMATCH
  return fnmatch( pattern , string , 0 );
#else
#pragma comment(lib , "shlwapi.lib")
  bool match = PathMatchSpec( string , pattern ); // shlwapi
  if (match)
    return 0;
  else
    return 1;

#endif
}



/**
   The util_alloc_realpath() will fail hard if the @input_path does
   not exist. If the path might-not-exist you should use
   util_alloc_abs_path() instead.
*/


char * util_alloc_realpath(const char * input_path) {
#ifdef HAVE_REALPATH
  char * buffer   = (char*)util_calloc(PATH_MAX + 1 , sizeof * buffer );
  char * new_path = NULL;

  new_path = realpath( input_path , buffer);
  if (new_path == NULL)
    util_abort("%s: input_path:%s - failed: %s(%d) \n",__func__ , input_path , strerror(errno) , errno);
  else
    new_path = (char*)util_realloc(new_path , strlen(new_path) + 1);

  return new_path;
#else
  /* We do not have the realpath() implementation. Must first check if
     the entry exists; and if not we abort. If the entry indeed exists
     we call the util_alloc_cwd_abs_path() function: */
#ifdef ERT_HAVE_SYMLINK
  ERROR - What the fuck; have symlinks and not realpath()?!
#endif
  if (!util_entry_exists( input_path ))
    util_abort("%s: input_path:%s does not exist - failed.\n",__func__ , input_path);

  return util_alloc_realpath__( input_path );
#endif
}


int util_chdir(const char * path) {
#ifdef HAVE_POSIX_CHDIR
  return chdir(path);
#endif

#ifdef HAVE_WINDOWS_CHDIR
  return _chdir( path );
#endif
}


int util_mkdir( const char * path ) {
#ifdef HAVE_POSIX_MKDIR
  return mkdir( path , UTIL_DEFAULT_MKDIR_MODE );
#endif

#ifdef HAVE_WINDOWS_MKDIR
  return _mkdir( path );
#endif
}


int util_getpid( ) {
#ifdef HAVE_POSIX_GETPID
  return getpid();
#endif

#ifdef HAVE_WINDOWS_MKDIR
  return _getpid();
#endif
}


void util_copy_mode(const char * src_file, const char * target_file) {
#ifdef HAVE_CHMOD
  stat_type stat_buffer;
  mode_t src_mode;

  stat( src_file , &stat_buffer );
  src_mode = stat_buffer.st_mode;
  chmod( target_file , src_mode );
#endif
}

/*
  Windows *might* have both the symbols _access() and access(), but we prefer
  the _access() symbol as that seems to be preferred by Windows. We therefor do
  the #HAVE_WINDOWS__ACCESS check first.
*/

#ifdef HAVE_WINDOWS__ACCESS

bool util_access(const char * entry, int mode) {
  return (_access(entry, mode) == 0);
}

#else

#ifdef HAVE_POSIX_ACCESS
bool util_access(const char * entry, mode_t mode) {
  return (access(entry, mode) == 0);
}
#endif
#endif

bool util_ftruncate(FILE * stream , long size) {
  int fd = fileno( stream );
  int int_return;

#ifdef HAVE_FTRUNCATE
  int_return = ftruncate( fd , size );
#elif HAVE__CHSIZE
  int_return = _chsize( fd , size );
#else
  BUG - no truncate implemantation available
#endif

  if (int_return == 0)
    return true;
  else
    return false;
}

