#include <util.h>
#include <subst.h>

int main(int argc, char ** argv)
{
  if(argc == 1 || argc % 2 != 0 ) 
    util_exit("Usage: replace.x from1 to1 from2 to2 ... fromN toN filename\n");
  
  subst_list_type * subst_list =  subst_list_alloc();
  for(int i=1; i < argc-1; i += 2)
    subst_list_insert_ref(subst_list, argv[i], argv[i+1]);

  subst_list_update_file(subst_list, argv[argc-1]);
  subst_list_free(subst_list);
  return 0;
}
