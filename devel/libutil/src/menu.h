#ifndef __MENU_H__
#define __MENU_H__

typedef struct menu_struct menu_type;

typedef void (menu_func_type) (void *);

menu_type * menu_alloc(const char * , const char *);
void        menu_run(const menu_type * );
void        menu_free(menu_type * );
void        menu_add_item(menu_type *, const char * , const char * , menu_func_type * , void *);
void        menu_add_separator(menu_type * );

#endif
