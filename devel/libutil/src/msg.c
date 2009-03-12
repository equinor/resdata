#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <stdbool.h>
#include <msg.h>


struct msg_struct {
  char * prompt;
  char * msg;
  int    msg_len;
  bool   visible;
};


static void __msg_assert_visible(const msg_type * msg) {
  if (!msg->visible) 
    util_abort("%s: you must call msg_show() first - aborting.\n",__func__);
}


static void __blank_string(int len) {
    int i;
  for (i = 0; i < len; i++)
    fputc('\b' , stdout);

  for (i = 0; i < len; i++)
    fputc(' ' , stdout);

  for (i = 0; i < len; i++)
    fputc('\b' , stdout);

}


void msg_clear_msg(msg_type * msg) {
  __msg_assert_visible(msg);
  __blank_string(msg->msg_len);
  if (msg->msg != NULL) {
    free(msg->msg);
    msg->msg_len = 0;
    msg->msg     = NULL;
  }
}
  


static void msg_clear_prompt(const msg_type * msg) {
  __blank_string(strlen(msg->prompt));
}


void msg_hide(msg_type * msg) {
  msg_clear_msg(msg);
  msg_clear_prompt(msg);
  msg->visible = false;
}


void msg_set_prompt(msg_type * msg , const char * prompt) {
  msg->prompt = util_realloc_string_copy(msg->prompt , prompt);
}


void msg_print_msg(const msg_type * msg) {
  if (msg->msg != NULL) 
    printf("%s" , msg->msg);
  fflush(stdout);
}


void msg_show(msg_type * msg) {
  if (!msg->visible) {
    printf("%s" , msg->prompt);
    msg_print_msg(msg);
    msg->visible = true;
  }
}


void msg_update(msg_type * msg , const char * new_msg) {
  __msg_assert_visible(msg);
  msg_clear_msg(msg);
  msg->msg = util_realloc_string_copy(msg->msg , new_msg);
  if (new_msg == NULL)
    msg->msg_len = 0;
  else
    msg->msg_len = strlen(new_msg);
  msg_print_msg(msg);
}


void msg_update_int(msg_type * msg , const char * fmt , int value) {
  char buffer[16];
  sprintf(buffer , fmt , value);
  msg_update(msg , buffer);
}


msg_type * msg_alloc(const char * prompt) {
  msg_type * msg = util_malloc(sizeof * msg , __func__);
  msg->prompt = util_alloc_string_copy(prompt);
  
  msg->msg     = NULL;
  msg->msg_len = 0;
  msg->visible = false;
  return msg;
}


void msg_free(msg_type * msg , bool clear) {
  if (clear) 
    msg_hide(msg);
  else 
    printf("\n");

  free(msg->prompt);
  if (msg->msg != NULL)
    free(msg->msg);
  
  free(msg);
}
  


