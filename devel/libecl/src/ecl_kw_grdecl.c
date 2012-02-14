/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'ecl_kw_grdecl.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <string.h>
#include <ctype.h>
#include <util.h>
#include <ecl_kw.h>
#include <ecl_util.h>


/*
  This file is devoted to different routines for reading and writing
  GRDECL formatted files. These files are very weakly formatted; and
  generally a pain in the ass to work with. Things to consider
  include:

   1. The files have no proper header; only the name of the keyword.

      a) If you have a corresponding grid file it might be possible to
         infer the correct size from this, otherwise you must just
         load all the data you find.

      b) There is not type information; presented with a bunch of
         formatted numbers it is in general impossible to determine
         whether the underlying datatype should be integer, float or
         double. Therefor all the file-reading routines here expect an
         ecl_type_enum variable as input.

   2. The files can have comment sections; even in the data block.

   3. The * notation can be used to print repeated values in a compact
      form, i.e.  1000*0.25 to get 1000 conecutive 0.25 values.
      
*/



/** 
  This function will search through a GRDECL file to look for the
  'kw'; input variables and return vales are similar to
  ecl_kw_fseek_kw(). 
  
  Observe that the GRDECL files are exteremly weakly structured, it is
  therefor veeeery easy to fool this function with a malformed GRDECL
  file. The current implementation just does string-search for 'kw';
  i.e. comments is enough to wreak havock.
*/


static bool ecl_kw_grdecl_fseek_kw__(const char * kw , FILE * stream) {
  const int newline_char = '\n';
  long int init_pos = ftell(stream);
  if (util_fseek_string(stream , kw , false , true)) {
    /*
      OK the keyword is found in the file; now we must verify that:

      1. It is terminated with a blank, i.e. when searching for
         'COORD' we do not want a positiv on 'COORDSYS'.

      2. That the keyword indeed starts with a isspace() character; we
         are not interested in the 'SYS' in 'COORDSYS'. 

      3. That the current location is not a comment section.
    */
    long int kw_pos = ftell( stream );
    bool valid_kw = false;
    int c;

    fseek( stream , strlen(kw) , SEEK_CUR);    // Seek to end of kw
    c = fgetc( stream );                       // Read one character  
    fseek( stream , kw_pos , SEEK_SET );       // Seek back to beginning of kw

    if (isspace(c)) {
      if (kw_pos > 0) {
        fseek( stream , kw_pos - 1 , SEEK_SET);
        c = fgetc( stream );
        if (isspace(c))
          // OK - we have verifed that the kw string we have found both
          // starts and ends with a isspace() character.
          valid_kw = true;
      } else
        valid_kw = true;  // kw is at the very beginning of the file.
    } 
    

    if (valid_kw) {
      // OK - the kw is validly terminated with a space/tab/newline; now
      // we must verify that it is not in a comment section.
      if (kw_pos >= strlen(ECL_COMMENT_STRING) ) {  // Must have this check to avoid infinite spinning
                                                    // when the keyword is in the very beginning of the file.
        fseek( stream , 1 , SEEK_CUR );
        while (true) {
          fseek( stream , -2 , SEEK_CUR );
          c = fgetc( stream );
          if ((c == newline_char) || (ftell(stream) == 0)) 
            break;
        }
        {
          // We have gone as far back as necessary.
          int line_length = kw_pos - ftell( stream );
          char * line = util_malloc(line_length + 1  , __func__);
          
          fread( stream , sizeof * line , line_length , stream);
          line[line_length] = '\0';
          
          if (strstr( line , ECL_COMMENT_STRING) == NULL) 
            // We are not in a commen section.  
            valid_kw = true;
          else
            valid_kw = false;
          
          free( line );
        }
      }
    } else
      valid_kw = false;
    
    if (valid_kw) 
      return true;
    else {
      fseek( stream , strlen(kw) , SEEK_CUR );  // Skip over the kw so we don't find it again.
      if (ecl_kw_grdecl_fseek_kw__(kw , stream))
        return true;
      else {
        fseek( stream , init_pos , SEEK_SET );
        return false;
      }
    } 
  } else 
    return false;
}

bool ecl_kw_grdecl_fseek_kw(const char * kw , bool rewind , bool abort_on_error , FILE * stream) {
  if (ecl_kw_grdecl_fseek_kw__(kw , stream))
    return true;       /* OK - we found the kw between current file pos and EOF. */
  else if (rewind) {
    long int init_pos = ftell(stream);
    
    fseek(stream , 0L , SEEK_SET);
    if (ecl_kw_grdecl_fseek_kw__( kw , stream )) /* Try again from the beginning of the file. */
      return true;                              
    else
      fseek(stream , init_pos , SEEK_SET);       /* Could not find it - reposition to initial position. */
  }

  /* OK: If we are here - that means that we failed to find the kw. */
  if (abort_on_error) {
    char * filename = "????";
#ifdef HAVE_FORK
    filename = util_alloc_filename_from_stream( stream );
#endif
    util_abort("%s: failed to locate keyword:%s in file:%s - aborting \n",__func__ , kw , filename);
  }

  return false;
}




/**
   Observe that this function does not preserve the '*' structure
   which (might) have been used in the input.  
*/


static void iset_range( char * data , int data_offset , int sizeof_ctype , void * value_ptr , int multiplier) {
  for (int index =0; index < multiplier; index++) 
    memcpy( &data[ (index + data_offset) * sizeof_ctype ] , value_ptr , sizeof_ctype);
}


/**
   The @strict flag is used to indicate whether the loader will accept
   character strings embedded into a numerical grdecl keyword; this
   should of course in general not be allowed and @strict should be
   set to true. However the SPECGRID keyword used when specifying a
   grid is often given as:

     SPECGRID
         10 10 100 100 F /

   Whatever that 'F' is - it is discarded when the SPECGRID header is
   written to a GRID/EGRID file. For this reason we have the
   possibility of setting @strict to false; in which case the 'F' or
   other characters in the numerical input will be ignored.  

   If @strict is set to true the function will bomb when meeting a
   non-numeric character like the 'F' above.  
   
   ----------------------------------------------------------------

   The function supports multiplier keywords like:

   PERMX
      10000*0.15  0.16 0.17 0.18 0.19 10000*0.20
   /
   
   Observe that no-spaces-are-allowed-around-the-*
*/

static char * fscanf_alloc_grdecl_data( const char * header , bool strict , ecl_type_enum ecl_type , int * kw_size , FILE * stream ) {
  char newline        = '\n';
  bool atEOF          = false;
  int init_size       = 32;
  int buffer_size     = 64;
  int data_index      = 0;
  int sizeof_ctype    = ecl_util_get_sizeof_ctype( ecl_type );
  int data_size       = init_size;
  char * buffer       = util_malloc( (buffer_size + 1) * sizeof * buffer , __func__);
  char * data         = util_malloc( sizeof_ctype * data_size * sizeof * data , __func__);

  while (true) {
    if (fscanf(stream , "%32s" , buffer) == 1) {
      if (strcmp(buffer , ECL_COMMENT_STRING) == 0) {
        // We have read a comment marker - just read up to the end of line.
        char c;
        while (true) {
          c = fgetc( stream );
          if (c == newline)
            break;
          if (c == EOF) {
            atEOF = true;
            break;
          }
        }
      } else if (strcmp(buffer , ECL_DATA_TERMINATION) == 0) 
        break;
      else {
        // We have read a valid input string; scan numerical input values from it.
        // The multiplier algorithm will fail hard if there are spaces on either side
        // of the '*'.

        int multiplier;
        void * value_ptr = NULL;
        bool   char_input = false;
        
        if (ecl_type == ECL_INT_TYPE) {
          int value;

          if (sscanf(buffer , "%d*%d" , &multiplier , &value) == 2) 
            {}
          else if (sscanf( buffer , "%d" , &value) == 1) 
            multiplier = 1;
          else {
            char_input = true;
            if (strict)
              util_abort("%s: Malformed content:\"%s\" when reading keyword:%s \n",__func__ , buffer , header);
          }
          
          value_ptr = &value;
        } else if (ecl_type == ECL_FLOAT_TYPE) {
          float value;

          if (sscanf(buffer , "%d*%g" , &multiplier , &value) == 2) 
            {}
          else if (sscanf( buffer , "%g" , &value) == 1) 
            multiplier = 1;
          else {
            char_input = true;
            if (strict)
              util_abort("%s: Malformed content:\"%s\" when reading keyword:%s \n",__func__ , buffer , header);
          }

          value_ptr = &value;
        } else 
          util_abort("%s: sorry type:%s not supported \n",__func__ , ecl_util_get_type_name(ecl_type));
        
        if (char_input)
          fprintf(stderr,"Warning: character string: \'%s\' ignored when reading keyword:%s \n",buffer , header);
        else {
          if (data_index + multiplier >= data_size) {
            data_size  = 2*(data_index + multiplier);
            data       = util_realloc( data , sizeof_ctype * data_size * sizeof * data , __func__);
          }
          
          iset_range( data , data_index , sizeof_ctype , value_ptr , multiplier );
          data_index += multiplier;
        }

      }
      if (atEOF)
        break;
    } else 
      break;
  }
  free( buffer );
  *kw_size = data_index;
  data = util_realloc( data , sizeof_ctype * data_index * sizeof * data , __func__);
  return data;
}

/*
  
  This is the fundamental 'load a grdecl formatted keyword'
  function. The @ecl_type keyword must be specified to give the type
  of the keyword, currently only integer and float are supported. The
  @kw and @size argument are optional; if supplied the reliability
  will be improved:

    size: If the @size is set to <= 0 the function will just load all
      data it can find until a terminating '/' is found. If a size
      argument is given the function will check that there is
      agreement between the size input argument and the number of
      elements found on the file.

    header: If the @header argument is != NULL the function will start
      by seeking through the file to find the header string; if the
      header can not be found the function will return NULL - but not
      fail any more than that.

      If @kw == NULL on input the function will just fscanf() the
      first available string and use that as header for the
      keyword. This can lead to failure in a zillion different ways;
      it is highly recommended to supply a valid string for the
      @header argument.

*/

static ecl_kw_type * ecl_kw_fscanf_alloc_grdecl__(FILE * stream , const char * header , bool strict , int size , ecl_type_enum ecl_type) {
  if (!(ecl_type == ECL_FLOAT_TYPE || ecl_type == ECL_INT_TYPE))
    util_abort("%s: sorry only types FLOAT and INT supported\n",__func__);

  if (header != NULL)
    if (!ecl_kw_grdecl_fseek_kw( header , true , false , stream ))
      return NULL;  /* Could not find it. */

  {
    char file_header[9];
    if (fscanf(stream , "%s" , file_header) == 1) {
      int kw_size;
      char * data = fscanf_alloc_grdecl_data( file_header , strict , ecl_type , &kw_size , stream );
      
      // Verify size
      if (size > 0)
        if (size != kw_size) {
          util_safe_free( data );
          util_abort("%s: size mismatch when loading:%s. File:%d elements. Requested:%d elements \n",
                     __func__ , file_header , kw_size , size);
        }
      
      {
        ecl_kw_type * ecl_kw = ecl_kw_alloc_new( file_header , kw_size , ecl_type , NULL );
        ecl_kw_set_data_ptr( ecl_kw , data );
        return ecl_kw;
      }

    } else {
      util_abort("%s: failed to read header \n",__func__);
      return NULL;
    }
  }
}




/**
   These files are tricky to load - if there is something wrong
   it is nearly impossible to detect.
*/
ecl_kw_type * ecl_kw_fscanf_alloc_grdecl_data(FILE * stream , int size , ecl_type_enum ecl_type) {
  bool strict = true;
  return ecl_kw_fscanf_alloc_grdecl__( stream , NULL , strict , size , ecl_type );
}



/**
   This function will load a keyword from a grdecl file, and return
   it. If input argument @kw is NULL it will just try loading from the
   current position, otherwise it will start with seeking to find @kw
   first.

   Observe that the grdecl files are very weakly structured, so the
   loading of ecl_kw instances from a grdecl file can go wrong in many
   ways; if the loading fails the function returns NULL.

   The main loop is extremely simple - it is just repeated calls to
   fscanf() to read one-number-at-atime; when that reading fails that
   is interpreted as the end of the keyword.

   Currently ONLY integer and float types are supported in ecl_type -
   any other types will lead to a hard failure.

   The ecl_kw class has a quite deeply wired assumption that the
   header is a string of length 8 (I hope/think that is an ECLIPSE
   limitation), and the the class is not able to create ecl_kw
   instances with header length of more than 8 characters - code will
   abort hard if @kw is longer than 8 characters.
*/

ecl_kw_type * ecl_kw_fscanf_alloc_grdecl_dynamic__( FILE * stream , const char * kw , bool strict , ecl_type_enum ecl_type) {
  return ecl_kw_fscanf_alloc_grdecl__( stream , kw , strict , 0 , ecl_type );
}

ecl_kw_type * ecl_kw_fscanf_alloc_grdecl_dynamic( FILE * stream , const char * kw , ecl_type_enum ecl_type) {
  bool strict = true;
  return ecl_kw_fscanf_alloc_grdecl_dynamic__( stream , kw , strict , ecl_type );
}


/*****************************************************************/


void ecl_kw_fprintf_grdecl(const ecl_kw_type * ecl_kw , FILE * stream) {
  fortio_type * fortio = fortio_alloc_FILE_wrapper(NULL , false , true , stream);   /* Endian flip should *NOT* be used */
  fprintf(stream,"%s\n" , ecl_kw_get_header8(ecl_kw));
  ecl_kw_fwrite_data(ecl_kw , fortio);
  fprintf(stream,"/\n"); 
  fortio_free_FILE_wrapper( fortio );
}


