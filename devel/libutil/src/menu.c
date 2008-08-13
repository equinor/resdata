#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <menu.h>

/**
   This file implements a simple character based menu system. The menu
   consists of a list of items, where each item has a description, a
   function to call, an argument to send to this function, and a set
   of keys which will invoke this function.


   Example:

   menu_type * menu = menu_alloc("Tittel paa menyen" , "qQ");  
   menu_add_item(menu , "Alternativ 1" , "1aA" , func1 , arg1);
   menu_add_item(menu , "Alternativ 2" , 2Bb" , func2 , arg2);
   menu_run(menu);
   menu_free(menu);

   In this case we will get a menu looking like this:


      Tittel paa menyen
      1: Alternativ 1
      2: Alternativ 2
      q: Quit
      ==> __

   Now typing '1' will invoke the function func1() with argument
   arg1. In addition the characters 'a' and 'A' can also be used to
   invoke the same function. Function func2(arg2) is invoked with '2',
   'b' or 'B'; finally the menu is exited with 'q' or 'Q' (the
   arguments to the alloc call). 


   Observe that (in the current implementation) only single characters
   are allowed as activator keys for the various items. I.e. 'save' to
   save is not an option'
*/




typedef struct menu_item_struct menu_item_type;

struct menu_item_struct {
  bool             separator; /* If this is a separator - in that case all the following fields are moot. */ 
  char    	 * key_set;   /* The characters which will activate this item , e.g. "sS" - must be a \0 terminated string */
  char    	 * label;     /* The label/description of this menu item */
  menu_func_type * func;      /* The function called when this item is activated. */
  void           * arg;       /* The argument passed to func. */
};


struct menu_struct {
  int               size;              /* The number of items in the menu */
  menu_item_type ** items;             /* The vector of items */
  char            * quit_keys;         /* The keys which can be used to quit from this menu - typically "qQ" */
  char            * title;             /* The title of this menu */
  char            * complete_key_set;  /* A string containing all the allowed characters - to validata input */
  int               max_label_length;  /* The length of longest label - for a nice display only */
};


/**
   This function returns true if the first argument contains _any_ of the characters in
   the second argument. It is implemented by repeated calss to strchr();
*/

static bool __string_contains(const char * string , const char * char_set) {
  bool contains = false;
  int i = 0;
  do {
    if (strchr(string , char_set[i]) != NULL) 
      contains = true;
    i++;
  } while (!contains && i < strlen(char_set));
  return contains;
}



/**
   This function allocates an empty menu, with title 'title'. This
   particular menu will exit when one the keys in quit_keys is
   entered. */


menu_type * menu_alloc(const char * title , const char * quit_keys) {
  menu_type * menu = util_malloc(sizeof * menu , __func__);
  
  menu->title     = util_alloc_string_copy( title );
  menu->quit_keys = util_alloc_string_copy( quit_keys );
  menu->size      = 0;
  menu->items     = NULL;
  menu->complete_key_set = util_alloc_string_copy( quit_keys );
  menu->max_label_length = 0;

  return menu;
}

static menu_item_type * menu_item_alloc_empty() {
  menu_item_type * item = util_malloc(sizeof * item , __func__);

  item->label   = NULL; 
  item->key_set = NULL; 
  item->func    = NULL;
  item->arg     = NULL;
  item->separator = false;

  return item;
}


/**
   Low level function doing the actual append of a complete item.
*/
static void menu_append_item__(menu_type * menu ,  menu_item_type * item) {
  menu->size += 1;
  menu->items = util_realloc(menu->items , menu->size * sizeof * menu->items , __func__);
  menu->items[menu->size - 1] = item;
}



/**
  Adds (appends) an item to the menu. 


  Obsereve that several keys can be used to invoke a particular
  function, however *only* the first key in the key_set is displayed
  when the menu is printed on stdout.
*/

void menu_add_item(menu_type * menu , const char * label , const char * key_set , menu_func_type * func, void * arg) {
  if (__string_contains(menu->complete_key_set , key_set)) 
    util_abort("%s:fatal error when building menu - key(s) in:%s already in use \n",__func__ , key_set);
  {
    menu_item_type * item = menu_item_alloc_empty();
    item->label   = util_alloc_string_copy(label);
    item->key_set = util_alloc_string_copy(key_set);
    item->func    = func;
    item->arg     = arg;
    item->separator = false;
    menu_append_item__(menu , item);
    menu->max_label_length = util_int_max(menu->max_label_length , strlen(label));
    menu->complete_key_set = util_strcat_realloc(menu->complete_key_set , key_set);
  }
}




/**
   Will add a '-------------' line to the menu.
*/
void menu_add_separator(menu_type * menu) {
  menu_item_type * item = menu_item_alloc_empty();
  item->separator = true;
  menu_append_item__(menu , item);
}






/**
   Free's the menu occupied by one menu item.
*/
static void menu_item_free(menu_item_type * item) {
  if (!item->separator) {
    free(item->key_set);
    free(item->label);
  }
  free(item);
}





static void __print_line(int c , int l) {
  int i;
  for (i=0; i < l; i++)
    fputc(c , stdout);
  fputc('\n' , stdout);
}
  
static void __print_sep(int l) {
  int i;
  printf("#");
  for (i=0; i < l; i++)
    fputc('-' , stdout);
  printf("#\n");
}


static void menu_display(const menu_type * menu) {
  int i;
  int length = util_int_max(menu->max_label_length , strlen(menu->title));
  
  __print_line('#' , length + 10);
  printf("# ");   util_fprintf_string(menu->title , length + 6 , center , stdout);  printf(" #\n");
  __print_line('#' , length + 10);
  for (i=0; i < menu->size; i++) {
    const menu_item_type * item = menu->items[i];
    if (item->separator) 
      __print_sep(length + 8);
    else {
      printf("# %c: ", item->key_set[0]);
      util_fprintf_string(item->label , length + 3 , right_pad , stdout);
      printf(" #\n");
    }
  }
  __print_sep(length + 8);
  printf("# %c: ",menu->quit_keys[0]);
  util_fprintf_string("Quit" , length + 3 , right_pad , stdout);
  printf(" #\n");
  __print_line('#' , length + 10);
  printf("\n");
}



/**
   Reads a string from stdin: If the string is longer than one single
   character it is discarded, if it is exactly one character long we
   check if it is in the menus set of available command characters,
   and return it *IF* it is a valid character. I.e. the return value
   from this function is *GURANTEED* to correspond to a menu item.
*/

static int menu_read_cmd(const menu_type * menu) { 
  char cmd[256]; 
  do { 
    printf("==> ");
    fflush(stdout); fscanf(stdin , "%s" , cmd); /* We read a full string -
						   but we only consider it if it is exactly *ONE* character long. */
  } while ((strchr(menu->complete_key_set , cmd[0]) == NULL) || strlen(cmd) > 1);
  return cmd[0];
}



void menu_run(const menu_type * menu) {
  while (1) {
    int cmd;
    

    menu_display(menu);
    cmd = menu_read_cmd(menu);
    if (strchr(menu->quit_keys , cmd) != NULL) /* We have recieved a quit command - leave the building. */
      break;

    /*
      OK - we start looking through all the available commands to see
      which this is. */
    {
      int item_index = 0;
      while (1) {
	const menu_item_type * item = menu->items[item_index];
	if (!item->separator) {
	  if (strchr(item->key_set , cmd) != NULL) {
	    /* Calling the function ... */
	    item->func(item->arg);
	    break;
	  }
	}
	item_index++;
      }
    }
  }
}



void menu_free(menu_type * menu) {
  free(menu->quit_keys);
  free(menu->title);
  free(menu->complete_key_set);
  {
    int i;
    for (i = 0; i < menu->size; i++) 
      menu_item_free(menu->items[i]);
    free(menu->items);
  }
}
