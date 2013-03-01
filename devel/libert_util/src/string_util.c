/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'string_util.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ctype.h>

#include <ert/util/util.h>
#include <ert/util/int_vector.h>
#include <ert/util/bool_vector.h>
#include <ert/util/string_util.h>


/*****************************************************************/

/* 
   This functions parses an input string 'range_string' of the type:

     "0,1,8, 10 - 20 , 15,17-21"
 
   I.e. integers separated by "," and "-". The integer values are
   parsed out. 
*/

//#include <stringlist.h>
//#include <tokenizer.h>
//static int * util_sscanf_active_range__NEW(const char * range_string , int max_value , bool * active , int * _list_length) {
//  tokenizer_type * tokenizer = tokenizer_alloc( NULL  , /* No ordinary split characters. */
//                                                NULL  , /* No quoters. */
//                                                ",-"  , /* Special split on ',' and '-' */
//                                                " \t" , /* Removing ' ' and '\t' */
//                                                NULL  , /* No comment */
//                                                NULL  );
//  stringlist_type * tokens;
//  tokens = tokenize_buffer( tokenizer , range_string , true);
//  
//  stringlist_free( tokens );
//  tokenizer_free( tokenizer );
//} 
   


static bool valid_characters( const char * range_string ) {
  bool valid = false;
  if (range_string) {
    int offset = 0;
    valid = true;
    while (true) {
      char c = range_string[offset];
      if (isspace(c) || isdigit(c) || c == ',' || c == '-')
        offset++;
      else 
        valid = false;
      if (offset == strlen( range_string ) || !valid)
        break;
    }
  }
  return valid;
}



static int_vector_type * util_sscanf_active_range__(const char * range_string ) {
  int_vector_type *active_list = int_vector_alloc(0,0);
  bool valid = valid_characters( range_string );
    
  if (valid) {
    int  value1,value2;
    char  * start_ptr = (char *) range_string;
    char  * end_ptr;
    bool didnt_work = false;
    
    while (start_ptr != NULL) {
      value1 = strtol(start_ptr , &end_ptr , 10);
      
      if (end_ptr == start_ptr){
        printf("Returning to menu: %s \n" , start_ptr);
        didnt_work = true;
        break;
      }
      /* OK - we have found the first integer, now there are three possibilities:
         
         1. The string contains nothing more (except) possibly whitespace.
         2. The next characters are " , " - with more or less whitespace.
         3. The next characters are " - " - with more or less whitespace.
         
         Otherwise it is a an invalid string.
      */
      
      int_vector_append( active_list , value1);
      
      /* Skipping trailing whitespace. */
      start_ptr = end_ptr;
      while (start_ptr[0] != '\0' && isspace(start_ptr[0]))
        start_ptr++;
      
      
      if (start_ptr[0] == '\0') /* We have found the end */
        start_ptr = NULL;
      else {
        /* OK - now we can point at "," or "-" - else malformed string. */
        if (start_ptr[0] == ',' || start_ptr[0] == '-') {
          if (start_ptr[0] == '-') {  /* This is a range */
            start_ptr++; /* Skipping the "-" */
            while (start_ptr[0] != '\0' && isspace(start_ptr[0]))
              start_ptr++;
            
            if (start_ptr[0] == '\0') {
              /* The range just ended - without second value. */
              printf("%s[0]: malformed string: %s \n",__func__ , start_ptr);
              didnt_work = true; 
              break;
            }
            value2 = strtol(start_ptr , &end_ptr , 10);
            if (end_ptr == start_ptr) {
              printf("%s[1]: failed to parse integer from: %s \n",__func__ , start_ptr);
              didnt_work = true;
              break;
            }
            
            if (value2 < value1){
              printf("%s[2]: invalid interval - must have increasing range \n",__func__);
              didnt_work = true;
              break;
            }
            start_ptr = end_ptr;
            { 
              int value;
              for (value = value1 + 1; value <= value2; value++) 
                int_vector_append( active_list , value );
            }
            
            /* Skipping trailing whitespace. */
            while (start_ptr[0] != '\0' && isspace(start_ptr[0]))
              start_ptr++;
            
            
            if (start_ptr[0] == '\0')
              start_ptr = NULL; /* We are done */
            else {
              if (start_ptr[0] == ',')
                start_ptr++;
              else{
                printf("%s[3]: malformed string: %s \n",__func__ , start_ptr);
                didnt_work = true;
                break;
              }
            }
          } else 
            start_ptr++;  /* Skipping "," */
          
          /**
             When this loop is finished the start_ptr should point at a
             valid integer. I.e. for instance for the following input
             string:  "1-3 , 78"
             ^
             
             The start_ptr should point at "78".
          */
          
        } else{
          printf("%s[4]: malformed string: %s \n",__func__ , start_ptr);
          didnt_work = true;
          break;
        }
      }
    }
  }
  
  if (!valid) {
    int_vector_free( active_list );
    active_list = NULL;
  }
    
  return active_list;
}



static int_vector_type *  util_sscanf_alloc_active_list(const char * range_string ) {
  return util_sscanf_active_range__(range_string );
}



/*****************************************************************/

static bool_vector_type * alloc_mask( const int_vector_type * active_list ) {
  bool_vector_type * mask = bool_vector_alloc( 0 , false );
  int i;
  for (i=0; i < int_vector_size( active_list ); i++) 
    bool_vector_iset( mask , int_vector_iget( active_list , i) , true );

  return mask;
}



bool string_util_update_active_list( const char * range_string , int_vector_type * active_list ) {
  int_vector_sort( active_list );
  {
    bool_vector_type * mask = alloc_mask( active_list );
    bool valid = false;

    if (string_util_update_active_mask( range_string , mask )) {
      int_vector_reset( active_list );
      {
        int i;
        for (i=0; i < bool_vector_size(mask); i++) {
          bool active = bool_vector_iget( mask , i );
          if (active)
            int_vector_append( active_list , i );
        }
      }
      valid = true;
    }
    bool_vector_free( mask );
    return valid;
  }
}


bool string_util_init_active_list( const char * range_string , int_vector_type * active_list ) {
  int_vector_reset( active_list );
  return string_util_update_active_list( range_string , active_list );
}


int_vector_type *  string_util_alloc_active_list( const char * range_string ) {
  int_vector_type * active_list = int_vector_alloc( 0 , 0 );
  string_util_init_active_list( range_string , active_list );
  return active_list;
}

/*****************************************************************/

/*
  This is the only function which actually invokes the low level
  string parsing in util_sscanf_alloc_active_list().  
*/

bool string_util_update_active_mask( const char * range_string , bool_vector_type * active_mask) {
  int i;
  int_vector_type * sscanf_active = util_sscanf_alloc_active_list( range_string );
  if (sscanf_active) {
    for (i=0; i < int_vector_size( sscanf_active ); i++)
      bool_vector_iset( active_mask , int_vector_iget(sscanf_active , i) , true );
    
    int_vector_free( sscanf_active );
    return true;
  } else
    return false;
}


bool string_util_init_active_mask( const char * range_string , bool_vector_type * active_mask ) {
  bool_vector_reset( active_mask );
  return string_util_update_active_mask( range_string , active_mask );
}


bool_vector_type * string_util_alloc_active_mask( const char * range_string ) {
  bool_vector_type * mask  = bool_vector_alloc(0 , false );
  string_util_init_active_mask( range_string , mask );
  return mask;
}
