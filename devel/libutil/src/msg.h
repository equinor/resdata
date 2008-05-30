#ifndef __MSG_H__
#define __MSG_H__

#include <stdbool.h>


typedef struct msg_struct msg_type;



msg_type   * msg_alloc(const char *);
void         msg_show(msg_type * );
void         msg_free(msg_type *  , bool);
void         msg_update(msg_type * , const char * );
void         msg_update_int(msg_type * , const char * , int );
void         msg_hide(msg_type *);
void         msg_clear_msg(msg_type * msg);


#endif
