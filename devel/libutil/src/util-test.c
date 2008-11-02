#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <stringlist.h>
#include <menu.h>
#include <subst.h>





int main(int argc , char ** argv) {
  subst_list_type * subst_list = subst_list_alloc();

  subst_list_insert_ref(subst_list , "<Navn>"    , "Joakim Hove");
  subst_list_insert_ref(subst_list , "<Adresse>" , "Henrik Mohnsvei 6");
  subst_list_filter_file(subst_list , "PERSONER" , "Joakim");
  subst_list_free(subst_list);

}
