#ifndef __MENU_H__
#define __MENU_H__

typedef struct menu_struct menu_type;
typedef struct menu_item_struct menu_item_type;

typedef void (menu_func_type) (void *);
typedef void (arg_free_ftype) (void *);

menu_type      * menu_alloc(const char * , const char *);
void             menu_run(const menu_type * );
void             menu_free(menu_type * );
menu_item_type * menu_get_item(const menu_type * , char );
menu_item_type * menu_add_item(menu_type *, const char * , const char * , menu_func_type * , void * , arg_free_ftype * );
void             menu_add_separator(menu_type * );
menu_item_type * menu_get_item(const menu_type * , char );
void             menu_item_set_label( menu_item_type * , const char *);

#endif
