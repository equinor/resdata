#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <util.h>
#include <stdlib.h>


int main( int argc , char ** argv ) {
  char hostname[256];
  gethostname( hostname , 255 );
  printf("%s : " , hostname);
  for (int iarg = 1; iarg < argc; iarg++) {
    uid_t * uid_list;
    int     num_users;
    
    printf("%s :",argv[iarg]);
    uid_list = util_alloc_file_users( argv[iarg] , &num_users);
    for (int i = 0; i < num_users; i++) {
      struct passwd * pwd = getpwuid( uid_list[i] );
      if (pwd != NULL)
        printf(" %s", pwd->pw_name);
      else
        printf(" %d",uid_list[ i ]);
    }
    
    if (num_users == 0)
      printf(" <Not open>");
    printf("\n");
    
    util_safe_free( uid_list );
  }
}
